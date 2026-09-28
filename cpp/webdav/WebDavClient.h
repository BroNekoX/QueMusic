// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 极简 WebDAV 客户端：只做列目录（PROPFIND Depth:1）
#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;

class WebDavClient : public QObject
{
    Q_OBJECT
public:
    struct Entry {
        QString name;      // 显示名（已解码，不含路径）
        QString url;       // 绝对 URL
        bool isDir = false;
        qint64 size = 0;
        QDateTime modified;
    };
    using ListCallback = std::function<void(const QList<Entry> &entries, const QString &error)>;

    explicit WebDavClient(QObject *parent = nullptr);

    void listDir(const QUrl &url, const QString &authHeader, ListCallback done);

    static QString basicAuth(const QString &user, const QString &password);

private:
    QNetworkAccessManager *m_nam = nullptr;
};
