// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QTcpServer;
class QTcpSocket;

// 本地视频流代理：给 MediaPlayer 的请求补 UA/Referer，并透传 Range。
class MvProxy : public QObject
{
    Q_OBJECT
public:
    explicit MvProxy(QObject *parent = nullptr);

    QString playableUrl(const QString &remoteUrl, const QString &referer = QString());

private:
    struct Entry
    {
        QUrl url;
        QString referer;
    };

    void handleNewConnection();
    void handleRequest(QTcpSocket *socket, const QByteArray &request);
    static void sendSimple(QTcpSocket *socket, const QByteArray &status, const QByteArray &body);
    static void sendHead(QTcpSocket *socket, QNetworkReply *reply);

    QTcpServer *m_server = nullptr;
    QNetworkAccessManager *m_nam = nullptr;
    QHash<QString, Entry> m_entries;
    QStringList m_order;
};
