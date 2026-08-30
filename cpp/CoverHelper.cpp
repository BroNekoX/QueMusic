// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "CoverHelper.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QHashFunctions>
#include <QStandardPaths>
#include <QUrl>

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
                if (entry.compare(target, Qt::CaseInsensitive) == 0)
                    return QUrl::fromLocalFile(dir.filePath(entry)).toString();
            }
        }
    }
    return QString();
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
