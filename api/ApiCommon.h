// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef APICOMMON_H
#define APICOMMON_H

#include <QJsonObject>
#include <QVariantList>
#include <QVariantMap>

#include <initializer_list>
#include <utility>

namespace ApiCommon {

inline const QString kTitle = "title";
inline const QString kArtist = "artist";
inline const QString kCover = "cover";
inline const QString kHash = "hash";
inline const QString kHashHq = "hashhq";
inline const QString kHashSq = "hashsq";
inline const QString kPaytype = "paytype";
inline const QString kDuration = "duration";
inline const QString kAlbum = "album";
inline const QString kPlaycount = "playcount";

inline constexpr int kPaytypeFree = 0;
inline constexpr int kPaytypePaid = 3;

inline QVariantMap song(QString title, QString artist, QString cover, QString hash,
                        int duration = 0, QString album = QString(),
                        QString hashhq = QString(), QString hashsq = QString(),
                        int paytype = 0, qint64 playcount = 0)
{
    hashhq = hashhq.isEmpty() ? hash : hashhq;
    hashsq = hashsq.isEmpty() ? hash : hashsq;
    return {{kTitle, std::move(title)}, {kArtist, std::move(artist)}, {kCover, std::move(cover)}, {kHash, std::move(hash)},
            {kHashHq, std::move(hashhq)}, {kHashSq, std::move(hashsq)}, {kPaytype, paytype}, {kDuration, duration},
            {kAlbum, std::move(album)}, {kPlaycount, playcount}};
}

inline QVariantMap listResult(QVariantList items)
{
    return {{"info", std::move(items)}};
}

inline QVariantMap listResult(QVariantList items, bool hasMore)
{
    return {{"info", std::move(items)}, {"hasMore", hasMore}};
}

inline QVariantMap listResult(QVariantList items, const QJsonObject &json)
{
    QVariantMap result = listResult(std::move(items));
    if (json.contains("error")) result.insert("error", json.value("error"));
    return result;
}

inline QVariantMap comment(QString user, QString avatar, QString content, qint64 time = 0, int liked = 0, int replies = 0)
{
    return {{"user", std::move(user)}, {"avatar", std::move(avatar)}, {"content", std::move(content)},
            {"time", time}, {"liked", liked}, {"replies", replies}};
}

inline bool hasValue(const QVariantMap &map, const QString &key)
{
    const QVariant value = map.value(key);
    return value.isValid() && !value.isNull();
}

inline QVariantMap normalizeItem(QVariantMap item)
{
    if (item.isEmpty())
        return item;

    const auto fill = [&item](const QString &key, const QString &fallback) {
        if (!hasValue(item, key) && hasValue(item, fallback))
            item.insert(key, item.value(fallback));
    };

    fill(kTitle, "name");
    fill(kArtist, "singer");
    fill(kCover, "pic");
    fill(kHash, "id");

    if (!hasValue(item, kTitle))     item.insert(kTitle, QString());
    if (!hasValue(item, kArtist))    item.insert(kArtist, QString());
    if (!hasValue(item, kCover))     item.insert(kCover, QString());
    if (!hasValue(item, kHash))      item.insert(kHash, QString());
    if (!hasValue(item, kAlbum))     item.insert(kAlbum, QString());
    if (!hasValue(item, kDuration))  item.insert(kDuration, 0);
    if (!hasValue(item, kPlaycount)) item.insert(kPlaycount, 0);
    if (!hasValue(item, kPaytype))   item.insert(kPaytype, kPaytypeFree);
    if (!hasValue(item, kHashHq))    item.insert(kHashHq, item.value(kHash));
    if (!hasValue(item, kHashSq))    item.insert(kHashSq, item.value(kHash));
    return item;
}

inline QVariantList normalizeList(const QVariant &value)
{
    const QVariantList list = value.toList();
    QVariantList result;
    result.reserve(list.size());
    for (const QVariant &entry : list)
        result.append(normalizeItem(entry.toMap()));
    return result;
}

inline QString firstNonEmpty(const QVariantMap &map, std::initializer_list<const char *> keys)
{
    for (const char *key : keys) {
        const QString text = map.value(QLatin1String(key)).toString();
        if (!text.isEmpty())
            return text;
    }
    return QString();
}

}

#endif