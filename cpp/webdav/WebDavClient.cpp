// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "WebDavClient.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QXmlStreamReader>

#include <algorithm>

namespace {

// 只看元素本地名，兼容各家服务器的命名空间前缀（<D:response> / <response>）
QList<WebDavClient::Entry> parseMultiStatus(const QByteArray &xml, const QUrl &base)
{
    QList<WebDavClient::Entry> out;
    QXmlStreamReader reader(xml);
    WebDavClient::Entry entry;
    QString href;
    bool inResponse = false;

    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement: {
            const QStringView name = reader.name();
            if (name == u"response") {
                inResponse = true;
                entry = WebDavClient::Entry();
                href.clear();
            } else if (!inResponse) {
                break;
            } else if (name == u"href") {
                href = reader.readElementText();
            } else if (name == u"collection") {
                entry.isDir = true;
            } else if (name == u"getcontentlength") {
                entry.size = reader.readElementText().toLongLong();
            } else if (name == u"getlastmodified") {
                entry.modified = QDateTime::fromString(reader.readElementText(), Qt::RFC2822Date);
            }
            break;
        }
        case QXmlStreamReader::EndElement:
            if (inResponse && reader.name() == u"response") {
                inResponse = false;
                const QUrl url = base.resolved(href);
                QString path = url.path();
                while (path.endsWith(QLatin1Char('/')))
                    path.chop(1);
                const QString name =
                    QUrl::fromPercentEncoding(path.section(QLatin1Char('/'), -1).toUtf8());
                if (name.isEmpty() || url == base)
                    break;                       // 结果里含自身，跳过
                entry.name = name;
                entry.url = url.toString();
                out.append(entry);
            }
            break;
        default:
            break;
        }
    }
    if (reader.hasError())
        out.clear();
    return out;
}

} // namespace

WebDavClient::WebDavClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

QString WebDavClient::basicAuth(const QString &user, const QString &password)
{
    if (user.isEmpty())
        return {};
    const QString token = user + QLatin1Char(':') + password;
    return QStringLiteral("Basic ") + QString::fromLatin1(token.toUtf8().toBase64());
}

void WebDavClient::listDir(const QUrl &url, const QString &authHeader, ListCallback done)
{
    QNetworkRequest request(url);
    request.setRawHeader("Depth", "1");
    request.setTransferTimeout(15000);      // 服务器不回包时快速失败，避免目录一直转圈
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/xml; charset=utf-8"));
    if (!authHeader.isEmpty())
        request.setRawHeader("Authorization", authHeader.toUtf8());

    static const QByteArray body = "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                                   "<D:propfind xmlns:D=\"DAV:\"><D:prop>"
                                   "<D:resourcetype/><D:getcontentlength/><D:getlastmodified/>"
                                   "</D:prop></D:propfind>";

    QNetworkReply *reply = m_nam->sendCustomRequest(request, "PROPFIND", body);
    connect(reply, &QNetworkReply::finished, this, [reply, url, done = std::move(done)] {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError) {
            QString message = reply->errorString();
            if (status == 401)
                message = QStringLiteral("认证失败（账号或密码不对）");
            else if (status == 403)
                message = QStringLiteral("没有访问权限");
            else if (status == 404)
                message = QStringLiteral("路径不存在");
            done({}, message);
            return;
        }
        QList<Entry> entries = parseMultiStatus(reply->readAll(), url);
        // 目录在前、名称升序（服务器返回顺序各家不一）
        std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
            if (a.isDir != b.isDir)
                return a.isDir;
            return QString::localeAwareCompare(a.name, b.name) < 0;
        });
        done(entries, QString());
    });
}
