// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COVERHELPER_H
#define COVERHELPER_H

#include <QFileInfo>
#include <QFutureWatcher>
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

public:
    explicit CoverHelper(QObject *parent = nullptr);

    // 默认封面缓存目录（实例与「其它地方也要提取封面」的调用方共用同一处，避免两套路径）
    static QString defaultCacheDir();

    // 音频同目录查找同名/常见命名封面（cover/folder/AlbumArt），未命中返回空
    Q_INVOKABLE QString findLocalCover(const QString &sourcePath);

    // 读取音频文件内嵌封面（ID3v2 APIC / FLAC Picture / MP4 covr）
    Q_INVOKABLE QString findEmbeddedCover(const QString &sourcePath);

    // 工作线程提取内嵌封面（含图像解码与缓存落盘），经 localCoverReady 回传
    Q_INVOKABLE void findEmbeddedCoverAsync(const QString &sourcePath);

    Q_INVOKABLE void clearCache();

    // 自定义封面缓存目录（空串忽略；与当前目录相同则不变更）
    Q_INVOKABLE void setCacheDir(const QString &path);

    // 缓存超出上限时按最旧优先删除缓存文件
    Q_INVOKABLE void pruneCache(int maxMB);

signals:
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
    static QImage toImage(const QVariant &value);
    static Metadata metadataFromTag(TagLib::FileRef &ref);

    QString m_cacheDir;
};

#endif // COVERHELPER_H
