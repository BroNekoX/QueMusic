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
#include <QStandardPaths>
#include <QUrl>

#include <fileref.h>
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
    const QFileInfo fi(localPath);
    const QByteArray seed = (fi.absoluteFilePath() + QLatin1Char('@')
                           + QString::number(fi.lastModified().toMSecsSinceEpoch()))
                                .toUtf8();
    // 用内容固定的哈希，跨系统 / 跨进程结果一致，不依赖 Qt 的散列种子
    const QString digest = QString::fromLatin1(
        QCryptographicHash::hash(seed, QCryptographicHash::Sha1).toHex());
    return m_cacheDir + QStringLiteral("/cover-") + digest + QStringLiteral(".png");
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

    // 存储缓存：文件存在就直接返回，跨进程、跨重启有效，不用再拆包解析
    const QString cacheFilePath = storageCachePath(localPath);
    if (QFileInfo::exists(cacheFilePath))
        return QUrl::fromLocalFile(cacheFilePath).toString();

    const QByteArray encodedPath = QFile::encodeName(localPath);
    TagLib::FileRef ref(encodedPath.constData(), false);
    QString coverUrl;
    if (!ref.isNull() && ref.file() != nullptr) {
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

void CoverHelper::clearCache()
{
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
