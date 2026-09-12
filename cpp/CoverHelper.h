// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COVERHELPER_H
#define COVERHELPER_H

#include <QFileInfo>
#include <QFutureWatcher>
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

    // 工作线程提取内嵌封面（含图像解码与缓存落盘），经 localCoverReady 回传
    Q_INVOKABLE void findEmbeddedCoverAsync(const QString &sourcePath);

    // 读取音频文件标题：同名 .json -> 内嵌 TAG，未命中返回空
    Q_INVOKABLE QString findTitle(const QString &sourcePath);

    // 读取音频文件歌手，未命中返回空
    Q_INVOKABLE QString findArtist(const QString &sourcePath);

    // 一次性读出 {title, artist, coverUrl}，共享同一次 TagLib 打开，
    // 替代逐项多次 Q_INVOKABLE 调用，避免每行 delegate 重复打开音频文件。
    Q_INVOKABLE QVariantMap loadFullMetadata(const QString &sourcePath);

    Q_INVOKABLE void clearCache();

signals:
    void currentCoverUrlChanged();
    void localCoverReady(const QString &sourcePath, const QString &coverUrl);

public:
    struct Metadata {
        QString title;
        QString artist;
    };

    // 单次 TagLib 打开：提取内嵌封面缩略图写入 cacheDir 并返回 file:// URL；
    // metaOut 非空时顺带带回 title/artist（同名 .json 优先）。
    // 无共享可变状态，可在工作线程调用。
    static QString readCoverFromTag(const QString &sourcePath, const QString &cacheDir,
                                    Metadata *metaOut = nullptr);
    static Metadata readMetadata(const QFileInfo &fileInfo, TagLib::FileRef *openRef = nullptr);

private:
    void setCoverUrl(const QString &url);
    static QImage toImage(const QVariant &value);
    Metadata metadataOf(const QString &sourcePath);
    static Metadata metadataFromTag(TagLib::FileRef &ref);

    QString m_currentCoverUrl;
    QString m_cacheDir;
    QHash<QString, Metadata> m_metadataCache;
};

#endif // COVERHELPER_H
