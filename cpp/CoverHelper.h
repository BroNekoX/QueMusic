// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COVERHELPER_H
#define COVERHELPER_H

#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration/qqmlintegration.h>

namespace TagLib {
class FileRef;
}

// 内嵌封面像素转缓存文件
class CoverHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString currentCoverUrl READ currentCoverUrl NOTIFY currentCoverUrlChanged)

public:
    explicit CoverHelper(QObject *parent = nullptr);

    QString currentCoverUrl() const;

    Q_INVOKABLE QString convertVariantToUrl(const QVariant &imageVariant);

    // 音频同目录查找同名/常见命名封面（cover/folder/AlbumArt），未命中返回空
    Q_INVOKABLE QString findLocalCover(const QString &sourcePath);

    // 读取音频文件内嵌封面（ID3v2 APIC / FLAC Picture / MP4 covr）
    Q_INVOKABLE QString findEmbeddedCover(const QString &sourcePath);

    // 读取音频文件标题：同名 .json -> 内嵌 TAG，未命中返回空
    Q_INVOKABLE QString findTitle(const QString &sourcePath);

    // 读取音频文件歌手，未命中返回空
    Q_INVOKABLE QString findArtist(const QString &sourcePath);

    Q_INVOKABLE void clearCache();

signals:
    void currentCoverUrlChanged();

private:
    struct Metadata {
        QString title;
        QString artist;
    };

    void setCoverUrl(const QString &url);
    static QImage toImage(const QVariant &value);
    // 存储缓存文件路径：由「绝对路径 + 修改时间」派生，进程重启后仍然命中
    QString storageCachePath(const QString &localPath) const;
    // openRef 非空时复用已打开的文件，避免同一文件重复解析
    Metadata metadataOf(const QString &sourcePath);
    static Metadata readMetadata(const QFileInfo &fileInfo, TagLib::FileRef *openRef = nullptr);
    static Metadata metadataFromTag(TagLib::FileRef &ref);

    QString m_currentCoverUrl;
    QString m_cacheDir;
    QHash<QString, Metadata> m_metadataCache;
};

#endif // COVERHELPER_H
