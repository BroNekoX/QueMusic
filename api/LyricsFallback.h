// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// LRCLIB 歌词兜底：给没有原生歌词接口的音源（Spotify / Apple Music）按「歌手 + 曲名」反查。

#ifndef LYRICSFALLBACK_H
#define LYRICSFALLBACK_H

#include "ApiCommon.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantList>

#include <functional>
#include <utility>

namespace LyricsFallback {

inline constexpr const char *kUserAgent = "QueMusic/0.6 (https://github.com/BroNekoX/QueMusic)";


inline QVariantList lyricsOf(const QJsonObject &object)
{
    const QString synced = object.value("syncedLyrics").toString();
    return synced.isEmpty() ? QVariantList{} : ApiCommon::parseLrc(synced);
}

inline void request(QNetworkAccessManager *nam, QObject *context, const QUrl &url,
                    std::function<void(const QJsonDocument &)> done)
{
    QNetworkRequest networkRequest(url);
    networkRequest.setRawHeader("User-Agent", kUserAgent);
    networkRequest.setRawHeader("Accept", "application/json");
    networkRequest.setTransferTimeout(15000);
    networkRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                              QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = nam->get(networkRequest);
    QObject::connect(context, &QObject::destroyed, reply, &QNetworkReply::abort);
    QObject::connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    QObject::connect(reply, &QNetworkReply::finished, context,
                     [reply, done = std::move(done)] {
                         done(reply->error() == QNetworkReply::NoError
                                  ? QJsonDocument::fromJson(reply->readAll())
                                  : QJsonDocument());
                     });
}

// 先走 /api/get 精确匹配；没有同步歌词时再走 /api/search 取第一条带时间轴的。
inline void fetch(QNetworkAccessManager *nam, QObject *context, const QString &artist,
                  const QString &title, int durationSeconds,
                  std::function<void(const QVariantList &)> done)
{
    if (title.trimmed().isEmpty()) {
        done({});
        return;
    }

    QUrl url(QStringLiteral("https://lrclib.net/api/get"));
    QUrlQuery query;
    if (!artist.trimmed().isEmpty())
        query.addQueryItem(QStringLiteral("artist_name"), artist.trimmed());
    query.addQueryItem(QStringLiteral("track_name"), title.trimmed());
    if (durationSeconds > 0)
        query.addQueryItem(QStringLiteral("duration"), QString::number(durationSeconds));
    url.setQuery(query);

    request(nam, context, url,
            [nam, context, artist, title, done = std::move(done)](const QJsonDocument &doc) {
                QVariantList lyrics = lyricsOf(doc.object());
                if (!lyrics.isEmpty()) {
                    done(lyrics);
                    return;
                }

                QUrl search(QStringLiteral("https://lrclib.net/api/search"));
                QUrlQuery searchQuery;
                searchQuery.addQueryItem(QStringLiteral("track_name"), title.trimmed());
                if (!artist.trimmed().isEmpty())
                    searchQuery.addQueryItem(QStringLiteral("artist_name"), artist.trimmed());
                search.setQuery(searchQuery);

                request(nam, context, search,
                        [done = std::move(done)](const QJsonDocument &results) {
                            const QJsonArray array = results.array();
                            for (const QJsonValue &value : array) {
                                const QVariantList lyrics = lyricsOf(value.toObject());
                                if (!lyrics.isEmpty()) {
                                    done(lyrics);
                                    return;
                                }
                            }
                            done({});
                        });
            });
}

} // namespace LyricsFallback

#endif // LYRICSFALLBACK_H