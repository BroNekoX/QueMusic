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

inline void request(QNetworkAccessManager *nam, QObject *context, const QUrl &url,
                    const QByteArray &userAgent, const QByteArray &referer,
                    const QByteArray &cookie, QNetworkAccessManager::Operation operation,
                    const QByteArray &body,
                    std::function<void(const QByteArray &, QNetworkReply::NetworkError)> done,
                    const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {})
{
    QNetworkRequest networkRequest(url);
    networkRequest.setRawHeader("User-Agent", userAgent);
    networkRequest.setRawHeader("Referer", referer);
    networkRequest.setRawHeader("Accept-Encoding", "identity");
    if (operation == QNetworkAccessManager::PostOperation) {
        networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                                 QStringLiteral("application/json;charset=UTF-8"));
        networkRequest.setRawHeader("Accept", "application/json");
    }
    networkRequest.setTransferTimeout(15000);
    if (!cookie.isEmpty() && url.scheme() == QLatin1String("https"))
        networkRequest.setRawHeader("Cookie", cookie);
    for (const auto &header : extraHeaders)
        networkRequest.setRawHeader(header.first, header.second);

    QNetworkReply *reply = operation == QNetworkAccessManager::PostOperation
                               ? nam->post(networkRequest, body)
                               : nam->get(networkRequest);
    QObject::connect(context, &QObject::destroyed, reply, &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    QObject::connect(reply, &QNetworkReply::finished, context,
                     [reply, done = std::move(done)] {
        const QNetworkReply::NetworkError error = reply->error();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray response = reply->readAll();
        const bool accepted = error == QNetworkReply::NoError
                              || (httpStatus >= 200 && httpStatus < 300 && !response.isEmpty());
        if (!accepted) {
            QUrl clean = reply->url();
            clean.setQuery(QString());
            qWarning() << "[api] 请求失败:" << reply->errorString() << clean.toString();
        }
        done(response, accepted ? QNetworkReply::NoError : error);
    });
}

inline void get(QNetworkAccessManager *nam, QObject *context, const QUrl &url,
                const QByteArray &userAgent, const QByteArray &referer,
                const QByteArray &cookie,
                std::function<void(const QByteArray &, QNetworkReply::NetworkError)> done,
                const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {})
{
    request(nam, context, url, userAgent, referer, cookie,
            QNetworkAccessManager::GetOperation, {}, std::move(done), extraHeaders);
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

inline void postJson(QNetworkAccessManager *nam, QObject *context, const QUrl &url,
                     const QByteArray &body, const QByteArray &userAgent,
                     const QByteArray &referer, const QByteArray &cookie,
                     std::function<void(const QJsonObject &)> done,
                     const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {})
{
    request(nam, context, url, userAgent, referer, cookie,
            QNetworkAccessManager::PostOperation, body,
            [done = std::move(done)](const QByteArray &response, QNetworkReply::NetworkError error) {
                QJsonObject object = QJsonDocument::fromJson(response).object();
                if (error != QNetworkReply::NoError)
                    object.insert(QStringLiteral("error"), int(error));
                done(object);
            },
            extraHeaders);
}

}