// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "MvProxy.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUuid>

#include <memory>

namespace {
constexpr int kMaxEntries = 32;
constexpr int kMaxRequestBytes = 64 * 1024;
const char *kBrowserUa =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
} // namespace

MvProxy::MvProxy(QObject *parent)
    : QObject(parent), m_server(new QTcpServer(this)), m_nam(new QNetworkAccessManager(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &MvProxy::handleNewConnection);
    m_server->listen(QHostAddress::LocalHost, 0);
}

QString MvProxy::playableUrl(const QString &remoteUrl, const QString &referer)
{
    const QUrl url(remoteUrl);
    if (!m_server->isListening() || !url.isValid()
        || (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https")))
        return remoteUrl;

    const QString key = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_entries.insert(key, Entry{url, referer});
    if (!m_order.contains(key))
        m_order.append(key);
    while (m_order.size() > kMaxEntries)
        m_entries.remove(m_order.takeFirst());

    return QStringLiteral("http://127.0.0.1:%1/%2").arg(m_server->serverPort()).arg(key);
}

void MvProxy::handleNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *socket = m_server->nextPendingConnection();
        auto *buffer = new QByteArray;
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        connect(socket, &QObject::destroyed, [buffer] { delete buffer; });
        connect(socket, &QTcpSocket::readyRead, this, [this, socket, buffer] {
            buffer->append(socket->readAll());
            const int end = buffer->indexOf("\r\n\r\n");
            if (end < 0) {
                if (buffer->size() > kMaxRequestBytes)
                    socket->abort();
                return;
            }
            const QByteArray request = buffer->left(end);
            buffer->clear();
            disconnect(socket, &QTcpSocket::readyRead, this, nullptr);
            handleRequest(socket, request);
        });
    }
}

void MvProxy::handleRequest(QTcpSocket *socket, const QByteArray &request)
{
    const QList<QByteArray> lines = request.split('\n');
    if (lines.isEmpty()) {
        socket->abort();
        return;
    }

    const QList<QByteArray> parts = lines.first().trimmed().split(' ');
    if (parts.size() < 2) {
        sendSimple(socket, "400 Bad Request", "bad request");
        return;
    }

    QByteArray path = parts.at(1);
    const int query = path.indexOf('?');
    if (query >= 0)
        path = path.left(query);
    const Entry entry = m_entries.value(QString::fromLatin1(path.mid(1)));
    if (!entry.url.isValid()) {
        sendSimple(socket, "404 Not Found", "unknown stream");
        return;
    }

    QNetworkRequest req(entry.url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("User-Agent", kBrowserUa);
    req.setRawHeader("Accept", "*/*");
    if (!entry.referer.isEmpty())
        req.setRawHeader("Referer", entry.referer.toUtf8());

    for (const QByteArray &line : lines) {
        if (line.toLower().startsWith("range:")) {
            req.setRawHeader("Range", line.mid(6).trimmed());
            break;
        }
    }

    QNetworkReply *reply = m_nam->get(req);
    auto started = std::make_shared<bool>(false);
    const auto canWrite = [socket] {
        return socket->state() == QAbstractSocket::ConnectedState;
    };

    connect(reply, &QNetworkReply::readyRead, socket, [reply, socket, started, canWrite] {
        if (!canWrite())
            return;
        if (!*started) {
            sendHead(socket, reply);
            *started = true;
        }
        socket->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, socket,
            [reply, socket, started, canWrite] {
                if (!canWrite())
                    return;
                if (!*started) {
                    sendHead(socket, reply);
                    *started = true;
                }
                socket->write(reply->readAll());
                socket->disconnectFromHost();
            });
    connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);
    connect(socket, &QTcpSocket::disconnected, reply, [reply] { reply->abort(); });
}

void MvProxy::sendSimple(QTcpSocket *socket, const QByteArray &status, const QByteArray &body)
{
    const QByteArray response = "HTTP/1.1 " + status
        + "\r\nContent-Type: text/plain\r\nContent-Length: "
        + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
    socket->write(response);
    socket->disconnectFromHost();
}

void MvProxy::sendHead(QTcpSocket *socket, QNetworkReply *reply)
{
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const int code = status > 0 ? status : 200;
    const char *reason = "OK";
    if (code == 206)
        reason = "Partial Content";
    else if (code == 403)
        reason = "Forbidden";
    else if (code == 404)
        reason = "Not Found";
    else if (code >= 400)
        reason = "Error";

    QByteArray head = "HTTP/1.1 " + QByteArray::number(code) + ' ' + reason + "\r\n";
    const auto append = [&head, reply](const char *name) {
        const QByteArray value = reply->rawHeader(name);
        if (!value.isEmpty())
            head += QByteArray(name) + ": " + value + "\r\n";
    };
    append("Content-Type");
    append("Content-Length");
    append("Content-Range");
    head += "Accept-Ranges: bytes\r\nConnection: close\r\n\r\n";
    socket->write(head);
}
