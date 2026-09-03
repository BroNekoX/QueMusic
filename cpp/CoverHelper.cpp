// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "CoverHelper.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHashFunctions>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>

#include <fileref.h>
#include <tag.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <attachedpictureframe.h>
#include <flacfile.h>
#include <flacpicture.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mp4coverart.h>
#include <tpropertymap.h>

namespace {

// 封面限制边长，降低编码耗时与磁盘占用
constexpr int kMaxCoverSize = 512;

// 元数据缓存键：由「绝对路径 + 修改时间」派生，文件被改写后自动失效
QString metadataCacheKey(const QFileInfo &fi)
{
    const QByteArray seed = (fi.absoluteFilePath() + QLatin1Char('@')
                             + QString::number(fi.lastModified().toMSecsSinceEpoch()))
                                .toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(seed, QCryptographicHash::Sha1).toHex());
}

// TagLib 字符串统一转 UTF-8，避免中文乱码
QString tagToQString(const TagLib::String &value)
{
    return value.isEmpty() ? QString() : QString::fromUtf8(value.toCString(true));
}

QString jsonFirst(const QJsonObject &object, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QString value = object.value(QLatin1String(key)).toString().trimmed();
        if (!value.isEmpty())
            return value;
    }
    return QString();
}

QString propertyFirst(const TagLib::PropertyMap &properties, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        for (const TagLib::String &value : properties.value(key)) {
            const QString text = tagToQString(value);
            if (!text.isEmpty())
                return text;
        }
    }
    return QString();
}

} // namespace

CoverHelper::CoverHelper(QObject *parent)
    : QObject(parent)
{
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
               + QStringLiteral("/cache");
    QDir().mkpath(m_cacheDir);
}

QString CoverHelper::currentCoverUrl() const
{
    return m_currentCoverUrl;
}

QString CoverHelper::convertVariantToUrl(const QVariant &imageVariant)
{
    QImage image = toImage(imageVariant);
    if (image.isNull()) {
        setCoverUrl(QString());
        return QString();
    }

    if (qMax(image.width(), image.height()) > kMaxCoverSize)
        image = image.scaled(kMaxCoverSize, kMaxCoverSize, Qt::KeepAspectRatio,
                             Qt::FastTransformation);

    // 按像素内容命名，重复播放直接命中缓存
    const QImage fingerprint = image.convertToFormat(QImage::Format_ARGB32);
    const QString fileName = QString::number(
        qHashBits(fingerprint.constBits(), size_t(fingerprint.sizeInBytes())), 16)
        + QStringLiteral(".png");
    const QString path = m_cacheDir + QLatin1Char('/') + fileName;

    if (!QFileInfo::exists(path) && !image.save(path, "PNG")) {
        setCoverUrl(QString());
        return QString();
    }

    const QString url = QUrl::fromLocalFile(path).toString();
    setCoverUrl(url);
    return url;
}

QString CoverHelper::storageCachePath(const QString &localPath) const
{
    // 用内容固定的哈希，跨系统 / 跨进程结果一致，不依赖 Qt 的散列种子
    return m_cacheDir + QStringLiteral("/cover-") + metadataCacheKey(QFileInfo(localPath))
           + QStringLiteral(".png");
}

QString CoverHelper::findLocalCover(const QString &sourcePath)
{
    if (sourcePath.isEmpty())
        return QString();

    QString localPath = sourcePath;
    const QUrl asUrl(sourcePath);
    if (asUrl.isLocalFile())
        localPath = asUrl.toLocalFile();

    const QFileInfo fi(localPath);
    if (!fi.isFile())
        return QString();

    QString found;
    const QDir dir = fi.absoluteDir();
    const QStringList entries = dir.entryList(QDir::Files);

    // 探测顺序：同名 -> cover/folder/AlbumArt
    const QStringList names = { fi.completeBaseName(), QStringLiteral("cover"),
                                QStringLiteral("folder"), QStringLiteral("AlbumArt") };
    const QStringList extensions = { QStringLiteral("jpg"), QStringLiteral("jpeg"),
                                     QStringLiteral("png"), QStringLiteral("webp"),
                                     QStringLiteral("bmp"), QStringLiteral("gif") };

    for (const QString &name : names) {
        for (const QString &ext : extensions) {
            const QString target = name + QLatin1Char('.') + ext;
            for (const QString &entry : entries) {
                if (entry.compare(target, Qt::CaseInsensitive) == 0) {
                    found = QUrl::fromLocalFile(dir.filePath(entry)).toString();
                    break;
                }
            }
            if (!found.isEmpty())
                break;
        }
        if (!found.isEmpty())
            break;
    }

    return found;
}

QString CoverHelper::findEmbeddedCover(const QString &sourcePath)
{
    QString localPath = sourcePath;
    const QUrl asUrl(sourcePath);
    if (asUrl.isLocalFile())
        localPath = asUrl.toLocalFile();
    const QFileInfo fi(localPath);
    if (!fi.isFile())
        return QString();

    // 存储缓存
    const QString cacheFilePath = storageCachePath(localPath);
    if (QFileInfo::exists(cacheFilePath))
        return QUrl::fromLocalFile(cacheFilePath).toString();

    const QByteArray encodedPath = QFile::encodeName(localPath);
    TagLib::FileRef ref(encodedPath.constData(), false);
    QString coverUrl;
    if (!ref.isNull() && ref.file() != nullptr) {
        // 顺带读出标题 / 歌手并缓存，与封面共用同一次解析
        const QString key = metadataCacheKey(fi);
        if (!m_metadataCache.contains(key))
            m_metadataCache.insert(key, readMetadata(fi, &ref));

        TagLib::ByteVector coverData;

        if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(ref.file())) {
            if (auto *id3v2 = mpeg->ID3v2Tag()) {
                const auto frames = id3v2->frameList("APIC");
                for (auto *frame : frames) {
                    auto *pic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame *>(frame);
                    if (!pic || pic->picture().isEmpty())
                        continue;
                    if (pic->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                        coverData = pic->picture();
                        break;
                    }
                    if (coverData.isEmpty())
                        coverData = pic->picture();
                }
            }
        } else if (auto *flac = dynamic_cast<TagLib::FLAC::File *>(ref.file())) {
            const auto pictures = flac->pictureList();
            for (auto *pic : pictures) {
                if (!pic || pic->data().isEmpty())
                    continue;
                if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
                    coverData = pic->data();
                    break;
                }
                if (coverData.isEmpty())
                    coverData = pic->data();
            }
        } else if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(ref.file())) {
            if (mp4->tag()) {
                const TagLib::MP4::CoverArtList covers =
                    mp4->tag()->item("covr").toCoverArtList();
                for (const auto &cover : covers) {
                    if (!cover.data().isEmpty()) {
                        coverData = cover.data();
                        break;
                    }
                }
            }
        }

        if (!coverData.isEmpty()) {
            QImage image;
            image.loadFromData(QByteArray(coverData.data(), coverData.size()));
            if (!image.isNull()) {
                if (qMax(image.width(), image.height()) > kMaxCoverSize)
                    image = image.scaled(kMaxCoverSize, kMaxCoverSize, Qt::KeepAspectRatio,
                                         Qt::FastTransformation);
                if (image.save(cacheFilePath, "PNG"))
                    coverUrl = QUrl::fromLocalFile(cacheFilePath).toString();
            }
        }
    }

    return coverUrl;
}

QString CoverHelper::findTitle(const QString &sourcePath)
{
    return metadataOf(sourcePath).title;
}

QString CoverHelper::findArtist(const QString &sourcePath)
{
    return metadataOf(sourcePath).artist;
}

CoverHelper::Metadata CoverHelper::metadataOf(const QString &sourcePath)
{
    if (sourcePath.isEmpty())
        return {};

    QString localPath = sourcePath;
    const QUrl asUrl(sourcePath);
    if (asUrl.isLocalFile())
        localPath = asUrl.toLocalFile();

    const QFileInfo fi(localPath);
    if (!fi.isFile())
        return {};

    const QString key = metadataCacheKey(fi);
    const auto cached = m_metadataCache.constFind(key);
    if (cached != m_metadataCache.constEnd())
        return cached.value();

    const Metadata meta = readMetadata(fi);
    m_metadataCache.insert(key, meta);
    return meta;
}

CoverHelper::Metadata CoverHelper::readMetadata(const QFileInfo &fileInfo, TagLib::FileRef *openRef)
{
    Metadata meta;

    // 同名 .json 是人工补全，优先于音频内嵌 TAG
    QFile json(fileInfo.absolutePath() + QLatin1Char('/') + fileInfo.completeBaseName()
               + QStringLiteral(".json"));
    if (json.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(json.readAll());
        if (doc.isObject()) {
            const QJsonObject object = doc.object();
            meta.title = jsonFirst(object, { "title", "name", "TITLE" });
            meta.artist = jsonFirst(object, { "artist", "singer", "songer", "ARTIST" });
        }
    }

    if (!meta.title.isEmpty() && !meta.artist.isEmpty())
        return meta;

    Metadata fromTag;
    if (openRef) {
        fromTag = metadataFromTag(*openRef);
    } else {
        const QByteArray encodedPath = QFile::encodeName(fileInfo.absoluteFilePath());
        TagLib::FileRef ref(encodedPath.constData(), false);
        fromTag = metadataFromTag(ref);
    }

    if (meta.title.isEmpty())
        meta.title = fromTag.title;
    if (meta.artist.isEmpty())
        meta.artist = fromTag.artist;
    return meta;
}

CoverHelper::Metadata CoverHelper::metadataFromTag(TagLib::FileRef &ref)
{
    Metadata meta;
    if (ref.isNull() || ref.file() == nullptr)
        return meta;

    if (const TagLib::Tag *tag = ref.tag()) {
        meta.title = tagToQString(tag->title());
        meta.artist = tagToQString(tag->artist());
    }

    const TagLib::PropertyMap properties = ref.file()->properties();
    if (meta.title.isEmpty())
        meta.title = propertyFirst(properties, { "TITLE" });
    if (meta.artist.isEmpty())
        meta.artist = propertyFirst(properties, { "ARTIST", "ALBUMARTIST" });
    return meta;
}

void CoverHelper::clearCache()
{
    m_metadataCache.clear();

    QDir dir(m_cacheDir);
    if (dir.exists())
        dir.removeRecursively();
    if (!dir.mkpath(m_cacheDir))
        qWarning() << "Failed to recreate cache directory:" << m_cacheDir;

    setCoverUrl(QString());
}

void CoverHelper::setCoverUrl(const QString &url)
{
    if (m_currentCoverUrl == url)
        return;
    m_currentCoverUrl = url;
    emit currentCoverUrlChanged();
}

QImage CoverHelper::toImage(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return QImage();

    if (value.userType() == QMetaType::QImage)
        return value.value<QImage>();

    if (value.canConvert<QByteArray>()) {
        QImage image;
        image.loadFromData(value.toByteArray());
        return image;
    }
    return QImage();
}
