// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COVERHELPER_H
#define COVERHELPER_H

#include <QObject>
#include <QImage>
#include <QVariant>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class CoverHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString currentCoverUrl READ currentCoverUrl NOTIFY currentCoverUrlChanged)

public:
    explicit CoverHelper(QObject *parent = nullptr);

    Q_INVOKABLE QString convertVariantToUrl(const QVariant &imageVariant);
    // 元数据未提供内嵌封面时，
    // 尝试在音频文件同目录查找同名/常见命名的本地封面图片（如 base.jpg/png、cover.jpg/folder.png 等）；
    // 命中返回 file:// URL，未命中返回空字符串。
    // sourcePath 为本地文件路径或 file:// URL。
    Q_INVOKABLE QString findLocalCover(const QString &sourcePath);
    QString currentCoverUrl() const;
    Q_INVOKABLE void clearCache();

signals:
    void currentCoverUrlChanged();

private:
    QString m_currentCoverUrl;
    QStringList m_createdFiles;          // 记录生成的文件路径
    QString m_cacheDir;                  // 缓存目录路径
};

#endif // COVERHELPER_H