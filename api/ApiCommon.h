// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef APICOMMON_H
#define APICOMMON_H

#include <QVariantList>
#include <QVariantMap>

#include <utility>

namespace ApiCommon {

inline const QString kTitle = QStringLiteral("title");
inline const QString kArtist = QStringLiteral("artist");
inline const QString kCover = QStringLiteral("cover");
inline const QString kHash = QStringLiteral("hash");
inline const QString kHashHq = QStringLiteral("hashhq");
inline const QString kHashSq = QStringLiteral("hashsq");
inline const QString kPaytype = QStringLiteral("paytype");
inline const QString kDuration = QStringLiteral("duration");
inline const QString kAlbum = QStringLiteral("album");
inline const QString kPlaycount = QStringLiteral("playcount");

inline constexpr int kPaytypeFree = 0;
inline constexpr int kPaytypePaid = 3;

inline QVariantMap song(QString title, QString artist, QString cover, QString hash,
                        int duration = 0, QString album = QString(),
                        QString hashhq = QString(), QString hashsq = QString(),
                        int paytype = 0, qint64 playcount = 0)
{
    if (hashhq.isEmpty())
        hashhq = hash;
    if (hashsq.isEmpty())
        hashsq = hash;
    return {
        {kTitle, std::move(title)},
        {kArtist, std::move(artist)},
        {kCover, std::move(cover)},
        {kHash, std::move(hash)},
        {kHashHq, std::move(hashhq)},
        {kHashSq, std::move(hashsq)},
        {kPaytype, paytype},
        {kDuration, duration},
        {kAlbum, std::move(album)},
        {kPlaycount, playcount}
    };
}

inline QVariantMap listResult(QVariantList items)
{
    return {{QStringLiteral("info"), std::move(items)}};
}

inline QVariantMap comment(QString user, QString avatar, QString content,
                           qint64 time = 0, int liked = 0, int replies = 0)
{
    return {
        {QStringLiteral("user"), std::move(user)},
        {QStringLiteral("avatar"), std::move(avatar)},
        {QStringLiteral("content"), std::move(content)},
        {QStringLiteral("time"), time},
        {QStringLiteral("liked"), liked},
        {QStringLiteral("replies"), replies}
    };
}

}

#endif