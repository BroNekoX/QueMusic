// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

#pragma once

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <functional>
#include <utility>

namespace ApiHttp {

inline void get(QNetworkAccessManager *nam, QObject *context, const QUrl &url,
                const QByteArray &userAgent, const QByteArray &referer,
                const QByteArray &cookie,
                std::function<void(const QByteArray &, QNetworkReply::NetworkError)> done,
                const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {})
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", userAgent);
    request.setRawHeader("Referer", referer);
    request.setRawHeader("Accept-Encoding", "identity");
    request.setTransferTimeout(15000);
    if (!cookie.isEmpty() && url.scheme() == QLatin1String("https"))
        request.setRawHeader("Cookie", cookie);
    for (const auto &header : extraHeaders)
        request.setRawHeader(header.first, header.second);

    QNetworkReply *reply = nam->get(request);
    QObject::connect(context, &QObject::destroyed, reply, &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    QObject::connect(reply, &QNetworkReply::finished, context,
                     [reply, done = std::move(done)] {
        const QNetworkReply::NetworkError error = reply->error();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        const bool accepted = error == QNetworkReply::NoError
                              || (httpStatus >= 200 && httpStatus < 300 && !body.isEmpty());
        if (!accepted) {
            QUrl clean = reply->url();
            clean.setQuery(QString());
            qWarning() << "[api] 请求失败:" << reply->errorString() << clean.toString();
        }
        done(body, accepted ? QNetworkReply::NoError : error);
    });
}

inline void getJson(QNetworkAccessManager *nam, QObject *context, const QUrl &url,
                    const QByteArray &userAgent, const QByteArray &referer,
                    const QByteArray &cookie, std::function<void(const QJsonObject &)> done)
{
    get(nam, context, url, userAgent, referer, cookie,
        [done = std::move(done)](const QByteArray &body, QNetworkReply::NetworkError error) {
            QJsonObject object = QJsonDocument::fromJson(body).object();
            if (error != QNetworkReply::NoError)
                object.insert(QStringLiteral("error"), int(error));
            done(object);
        });
}

}