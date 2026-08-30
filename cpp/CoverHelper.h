// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COVERHELPER_H
#define COVERHELPER_H

#include <QImage>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtQmlIntegration/qqmlintegration.h>

// 内嵌封面像素转缓存文件（按内容哈希复用，重复播放不再重新编码）
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

    Q_INVOKABLE void clearCache();

signals:
    void currentCoverUrlChanged();

private:
    void setCoverUrl(const QString &url);
    static QImage toImage(const QVariant &value);

    QString m_currentCoverUrl;
    QString m_cacheDir;
};

#endif // COVERHELPER_H
