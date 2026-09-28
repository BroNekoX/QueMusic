// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 平台自建请求的公共部分：统一 UA/Referer/Cookie、15s 超时、失败也回调（保证 QML 侧复位）
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
                const QByteArray &cookie, std::function<void(const QByteArray &)> done)
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", userAgent);
    request.setRawHeader("Referer", referer);
    request.setRawHeader("Accept-Encoding", "identity");
    request.setTransferTimeout(15000);
    if (!cookie.isEmpty())
        request.setRawHeader("Cookie", cookie);

    QNetworkReply *reply = nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, context, [reply, done = std::move(done)] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            qWarning() << "[api] 请求失败:" << reply->errorString() << reply->url().toString();
        done(reply->error() != QNetworkReply::NoError ? QByteArray() : reply->readAll());
    });
}

inline void getJson(QNetworkAccessManager *nam, QObject *context, const QUrl &url,
                    const QByteArray &userAgent, const QByteArray &referer,
                    const QByteArray &cookie, std::function<void(const QJsonObject &)> done)
{
    get(nam, context, url, userAgent, referer, cookie,
        [done = std::move(done)](const QByteArray &body) {
            done(QJsonDocument::fromJson(body).object());
        });
}

} // namespace ApiHttp
