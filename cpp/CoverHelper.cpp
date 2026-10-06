// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "CoverHelper.h"
#include "DbService.h"

#include <QtConcurrent/QtConcurrentRun>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMultiMap>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
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

QString metadataCacheKey(const QFileInfo &fi)
{
    const QByteArray seed = (fi.absoluteFilePath() + QLatin1Char('@')
                             + QString::number(fi.lastModified().toMSecsSinceEpoch()))
                                .toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(seed, QCryptographicHash::Sha1).toHex());
}

// 旧版带尺寸后缀的缓存名，如 cover-<sha1>-64.jpg
const QRegularExpression kSizedCoverName(
    QStringLiteral("^cover-[0-9a-f]{40}-\\d+\\.(?:jpg|png)$"));

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

// file:/// 形式的 URL 还原成本地路径；本身已是路径时原样返回
QString localPathFromSource(const QString &sourcePath)
{
    const QUrl url(sourcePath);
    return url.isLocalFile() ? url.toLocalFile() : sourcePath;
}

} // namespace

CoverHelper::CoverHelper(QObject *parent)
    : QObject(parent)
    , m_cacheDir(DbService::cacheDir())
{
    QDir().mkpath(m_cacheDir);
}

QString CoverHelper::findLocalCover(const QString &sourcePath)
{
    if (sourcePath.isEmpty())
        return QString();

    const QString localPath = localPathFromSource(sourcePath);

    const QFileInfo fi(localPath);
    if (!fi.isFile())
        return QString();

    const QDir dir = fi.absoluteDir();
    const QStringList entries = dir.entryList(QDir::Files);

    // 探测顺序：同名 -> cover/folder/AlbumArt
    const QStringList names = { fi.completeBaseName(), QStringLiteral("cover"),
                                QStringLiteral("folder"), QStringLiteral("AlbumArt") };
    const QStringList extensions = { QStringLiteral("jpg"), QStringLiteral("jpeg"),
                                     QStringLiteral("png"), QStringLiteral("webp"),
                                     QStringLiteral("bmp"), QStringLiteral("gif") };

    QHash<QString, QString> byLowerName;
    byLowerName.reserve(entries.size());
    for (const QString &entry : entries)
        byLowerName.insert(entry.toLower(), entry);

    for (const QString &name : names) {
        for (const QString &ext : extensions) {
            const auto it = byLowerName.constFind((name + QLatin1Char('.') + ext).toLower());
            if (it != byLowerName.constEnd())
                return QUrl::fromLocalFile(dir.filePath(it.value())).toString();
        }
    }

    return QString();
}

QString CoverHelper::readCoverFromTag(const QString &sourcePath, const QString &cacheDir,
                                      Metadata *metaOut, bool *failedOut)
{
    if (failedOut)
        *failedOut = false;
    const QString localPath = localPathFromSource(sourcePath);
    const QFileInfo fi(localPath);
    if (metaOut)
        *metaOut = Metadata();
    if (!fi.isFile()) {
        if (failedOut)
            *failedOut = true;   // 文件暂时不可用，留待重试
        return QString();
    }

    // 一首歌只有一个封面缓存文件（JPEG，写不出去才退 PNG），与用途无关
    const QString base = cacheDir + QStringLiteral("/cover-") + metadataCacheKey(fi);
    const QString cachedJpg = base + QStringLiteral(".jpg");
    const QString cachedPng = base + QStringLiteral(".png");
    const bool hasJpg = QFileInfo::exists(cachedJpg);
    if (hasJpg || QFileInfo::exists(cachedPng)) {
        if (metaOut)
            *metaOut = readMetadata(fi);
        return QUrl::fromLocalFile(hasJpg ? cachedJpg : cachedPng).toString();
    }

    if (!QDir().mkpath(cacheDir)) {
        qWarning() << "[cover] 缓存目录不可用:" << cacheDir;
        if (failedOut)
            *failedOut = true;
        return QString();
    }

    const QByteArray encodedPath = QFile::encodeName(localPath);
    TagLib::FileRef ref(encodedPath.constData(), false);
    if (ref.isNull() || ref.file() == nullptr) {
        // 典型情形是文件正被播放占用：等空闲时（列表扫描）再提取
        qWarning() << "[cover] 无法打开音频文件读取封面:" << localPath;
        if (failedOut)
            *failedOut = true;
        return QString();
    }
    if (metaOut)
        *metaOut = readMetadata(fi, &ref);

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
            const TagLib::MP4::CoverArtList covers = mp4->tag()->item("covr").toCoverArtList();
            for (const auto &cover : covers) {
                if (!cover.data().isEmpty()) {
                    coverData = cover.data();
                    break;
                }
            }
        }
    }

    if (coverData.isEmpty())   // 没有内嵌封面是常见情况，不值得刷日志，也不需要重试
        return QString();

    QImage image;
    image.loadFromData(QByteArray(coverData.data(), coverData.size()));
    if (image.isNull()) {
        qWarning() << "[cover] 内嵌封面解码失败:" << localPath;
        if (failedOut)
            *failedOut = true;
        return QString();
    }

    // 覆盖式缩放：短边不小于目标值，配合 QML 的 PreserveAspectCrop 不会被拉糊
    if (qMax(image.width(), image.height()) > kCoverSize)
        image = image.scaled(kCoverSize, kCoverSize, Qt::KeepAspectRatioByExpanding,
                             Qt::SmoothTransformation);
    // JPEG 没有 alpha 通道，带透明通道的封面先转 RGB，免得存出来发黑
    if (image.hasAlphaChannel())
        image = image.convertToFormat(QImage::Format_RGB32);

    QSaveFile out(cachedJpg);
    if (out.open(QIODevice::WriteOnly) && image.save(&out, "JPEG", 88) && out.commit())
        return QUrl::fromLocalFile(cachedJpg).toString();

    QSaveFile png(cachedPng);   // 构建里万一没有 JPEG 支持
    if (png.open(QIODevice::WriteOnly) && image.save(&png, "PNG") && png.commit())
        return QUrl::fromLocalFile(cachedPng).toString();

    qWarning() << "[cover] 封面缓存写入失败:" << cachedJpg;
    if (failedOut)
        *failedOut = true;
    return QString();
}

bool CoverHelper::isLegacyCover(const QString &coverUrl)
{
    if (coverUrl.isEmpty())
        return false;
    return kSizedCoverName.match(QFileInfo(QUrl(coverUrl).toLocalFile()).fileName()).hasMatch();
}

QString CoverHelper::findEmbeddedCover(const QString &sourcePath)
{
    const QString localPath = localPathFromSource(sourcePath);
    if (!QFileInfo(localPath).isFile())
        return QString();
    return readCoverFromTag(localPath, m_cacheDir);
}

void CoverHelper::findEmbeddedCoverAsync(const QString &sourcePath)
{
    const QString localPath = localPathFromSource(sourcePath);
    const QString cacheDir = m_cacheDir;

    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, sourcePath]() {
        const QString coverUrl = watcher->result();
        watcher->deleteLater();
        // 空结果也要回传：QML 据此决定保留 json 封面还是回退占位图
        emit localCoverReady(sourcePath, coverUrl);
    });
    watcher->setFuture(QtConcurrent::run([localPath, cacheDir]() {
        return readCoverFromTag(localPath, cacheDir);
    }));
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
    QDir dir(m_cacheDir);
    if (dir.exists())
        dir.removeRecursively();
    if (!dir.mkpath(m_cacheDir))
        qWarning() << "Failed to recreate cache directory:" << m_cacheDir;

    // 文件删了，DB 里存的封面路径就成了死链：清掉引用并标记未提取，下次打开列表时重新提取
    DbService::instance()->submit([](QSqlDatabase &db) {
        QSqlQuery query(db);
        if (!query.exec(QStringLiteral("UPDATE songs SET tagged = 0, tag_cover = ''")))
            qWarning() << "Failed to reset cover cache in db:" << query.lastError().text();
    });
}

void CoverHelper::setCacheDir(const QString &path)
{
    if (path.isEmpty() || path == m_cacheDir)
        return;
    m_cacheDir = path;
    if (!QDir().mkpath(m_cacheDir))
        qWarning() << "Failed to create cache directory:" << m_cacheDir;
}

// 放到线程池：启动时会调用一次，不能占着 UI 线程。
// 按 mtime 从旧到新删，新写入的缓存排在最末，不会被误删。
void CoverHelper::pruneCache(int maxMB)
{
    if (maxMB <= 0)
        return;
    const QString cacheDir = m_cacheDir;
    // 丢弃 QFuture：只是把清理丢给线程池，不阻塞 UI
    (void)QtConcurrent::run([cacheDir, maxMB] {
        QDir dir(cacheDir);
        if (!dir.exists())
            return;

        const qint64 limit = qint64(maxMB) * 1024 * 1024;
        QMultiMap<qint64, QFileInfo> entries;
        qint64 total = 0;
        const QFileInfoList files = dir.entryInfoList(QDir::Files);
        for (const QFileInfo &fi : files) {
            total += fi.size();
            entries.insert(fi.lastModified().toMSecsSinceEpoch(), fi);
        }

        while (total > limit && !entries.isEmpty()) {
            auto it = entries.begin();
            const QFileInfo fi = it.value();
            entries.erase(it);
            total -= fi.size();
            QFile::remove(fi.absoluteFilePath());
        }
    });
}
