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
                const QByteArray &cookie,
                std::function<void(const QByteArray &, QNetworkReply::NetworkError)> done,
                const QList<QPair<QByteArray, QByteArray>> &extraHeaders = {})
{
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", userAgent);
    request.setRawHeader("Referer", referer);
    request.setRawHeader("Accept-Encoding", "identity");
    request.setTransferTimeout(15000);
    // 明文端点不携带凭证：酷狗等仍有个别 http:// 域名，凭证不能随明文出网
    if (!cookie.isEmpty() && url.scheme() == QLatin1String("https"))
        request.setRawHeader("Cookie", cookie);
    for (const auto &header : extraHeaders)
        request.setRawHeader(header.first, header.second);

    QNetworkReply *reply = nam->get(request);
    QObject::connect(reply, &QNetworkReply::finished, context, [reply, done = std::move(done)] {
        const QNetworkReply::NetworkError error = reply->error();
        const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QByteArray body = reply->readAll();
        // 服务端在完整响应后立刻断链时，Qt 会报 RemoteHostClosedError，但数据其实是完整的
        const bool accepted = error == QNetworkReply::NoError
                              || (httpStatus >= 200 && httpStatus < 300 && !body.isEmpty());
        if (!accepted) {
            // 只记 scheme://host/path：查询串里可能带 token
            QUrl clean = reply->url();
            clean.setQuery(QString());
            qWarning() << "[api] 请求失败:" << reply->errorString() << clean.toString();
        }
        reply->deleteLater();
        done(body, accepted ? QNetworkReply::NoError : error);
    });
}

// JSON 版：传输失败时在对象里补一个 "error" 键，让上层与「业务成功但空数据」区分开。
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

} // namespace ApiHttp
