// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "CoverHelper.h"
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

CoverHelper::CoverHelper(QObject *parent)
    : QObject(parent)
{
    m_cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/cache";
    QDir dir;
    if (!dir.exists(m_cacheDir)) {
        dir.mkpath(m_cacheDir);
    }
}

QString CoverHelper::convertVariantToUrl(const QVariant &imageVariant)
{
    if (!imageVariant.isValid() || imageVariant.isNull()) {
        m_currentCoverUrl.clear();
        emit currentCoverUrlChanged();
        return QString();
    }

    // 从 QVariant 提取 QImage
    QImage img;
    if (imageVariant.userType() == QMetaType::QImage) {
        img = imageVariant.value<QImage>();
    } else if (imageVariant.canConvert<QByteArray>()) {
        img.loadFromData(imageVariant.toByteArray());
    }

    if (img.isNull()) {
        m_currentCoverUrl.clear();
        emit currentCoverUrlChanged();
        return QString();
    }

    // 保存为临时 PNG 文件供 QML Image 加载
    QTemporaryFile tempFile(m_cacheDir + "/cover_XXXXXX.png");
    if (tempFile.open() && img.save(&tempFile, "PNG")) {
        tempFile.setAutoRemove(false); // 保留文件，避免 QML 加载时被清理
        m_currentCoverUrl = "file:///" + tempFile.fileName();
        emit currentCoverUrlChanged();
        return m_currentCoverUrl;
    }

    m_currentCoverUrl.clear();
    emit currentCoverUrlChanged();
    return QString();
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
    if (!fi.exists() || !fi.isFile())
        return QString();

    const QDir dir = fi.absoluteDir();
    const QString baseName = fi.completeBaseName();

    // 探测顺序：与音频文件同名 -> 常见通用命名（cover/folder/AlbumArt）
    const QStringList names = { baseName, QStringLiteral("cover"),
                                QStringLiteral("folder"), QStringLiteral("AlbumArt") };
    const QStringList extensions = { QStringLiteral("jpg"), QStringLiteral("jpeg"),
                                     QStringLiteral("png"), QStringLiteral("webp"),
                                     QStringLiteral("bmp"), QStringLiteral("gif") };

    for (const QString &name : names) {
        for (const QString &ext : extensions) {
            const QString candidate = dir.filePath(name + QLatin1Char('.') + ext);
            if (QFile::exists(candidate))
                return QUrl::fromLocalFile(candidate).toString();
        }
    }
    return QString();
}

QString CoverHelper::currentCoverUrl() const
{
    return m_currentCoverUrl;
}

void CoverHelper::clearCache()
{
    QDir dir(m_cacheDir);

    // 如果目录存在，递归删除整个目录及其全部内容
    if (dir.exists()) {
        if (dir.removeRecursively()) {
            qDebug() << "Cache directory removed:" << m_cacheDir;
        } else {
            qWarning() << "Failed to remove cache directory:" << m_cacheDir;
        }
    }

    // 重新创建空目录（确保后续临时文件能正常写入）
    if (!dir.mkpath(m_cacheDir)) {
        qWarning() << "Failed to recreate cache directory:" << m_cacheDir;
    }

    // 清空记录列表（无用）
    m_createdFiles.clear();

    // 重置当前封面 URL 并发射信号
    m_currentCoverUrl.clear();
    emit currentCoverUrlChanged();
}