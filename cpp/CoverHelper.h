// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COVERHELPER_H
#define COVERHELPER_H

#include <QFileInfo>
#include <QObject>
#include <QString>
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

    // 音频同目录查找同名/常见命名封面（cover/folder/AlbumArt），未命中返回空
    Q_INVOKABLE QString findLocalCover(const QString &sourcePath);

    // 读取音频文件内嵌封面（ID3v2 APIC / FLAC Picture / MP4 covr）
    Q_INVOKABLE QString findEmbeddedCover(const QString &sourcePath);

    // 工作线程取内嵌封面（命中缓存直接回传），经 localCoverReady 回传
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

    // 内嵌封面统一按这一档缓存成 cover-<key>.jpg，列表与播放页共用，解码尺寸交给 QML 的 sourceSize
    static constexpr int kCoverSize = 512;

    // 旧版缓存名带尺寸后缀（cover-<key>-64.jpg），命中说明是过期缓存、需要重提
    static bool isLegacyCover(const QString &coverUrl);

    // TagLib 打开音频取内嵌封面：命中缓存直接返回，否则缩到 kCoverSize 写入 cacheDir（JPEG）。
    // metaOut 非空时顺带带回 title/artist（同名 .json 优先）；failedOut 区分「没读到」与「确实没有封面」。
    static QString readCoverFromTag(const QString &sourcePath, const QString &cacheDir,
                                    Metadata *metaOut = nullptr, bool *failedOut = nullptr);
    static Metadata readMetadata(const QFileInfo &fileInfo, TagLib::FileRef *openRef = nullptr);

private:
    static Metadata metadataFromTag(TagLib::FileRef &ref);

    QString m_cacheDir;
};

#endif // COVERHELPER_H
