// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef APICOMMON_H
#define APICOMMON_H

#include <QJsonObject>
#include <QList>
#include <QRegularExpression>
#include <QVariantList>
#include <QVariantMap>

#include <algorithm>
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

// 平台头像/封面常见 //host 和 http:// 两种写法，统一成 https。
inline QString httpsUrl(QString url)
{
    if (url.startsWith(QLatin1String("//")))
        return QLatin1String("https:") + url;
    if (url.startsWith(QLatin1String("http://")))
        url.replace(0, 7, QLatin1String("https://"));
    return url;
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

// 标准 LRC：一行可以带多个时间戳，[mm:ss.xx] 与 [mm:ss.xxx] 都接受。
inline QVariantList parseLrc(const QString &text)
{
    static const QRegularExpression timestamp(
        "\\[(\\d{1,3}):(\\d{1,2})(?:[.:](\\d{1,3}))?\\]");

    QVariantList result;
    const QStringList lines = text.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;

        QList<qlonglong> times;
        int end = 0;
        QRegularExpressionMatchIterator it = timestamp.globalMatch(line);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            int ms = 0;
            const QString fraction = match.captured(3);
            if (fraction.size() == 1)
                ms = fraction.toInt() * 100;
            else if (fraction.size() == 2)
                ms = fraction.toInt() * 10;
            else if (!fraction.isEmpty())
                ms = fraction.left(3).toInt();
            times << (match.captured(1).toLongLong() * 60 + match.captured(2).toLongLong()) * 1000 + ms;
            end = match.capturedEnd();
        }
        if (times.isEmpty())
            continue;

        const QString lyric = line.mid(end).trimmed();
        if (lyric.isEmpty())
            continue;
        for (qlonglong time : std::as_const(times))
            result << QVariantMap{{"time", time}, {"text", lyric}};
    }

    std::sort(result.begin(), result.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value("time").toLongLong() < b.toMap().value("time").toLongLong();
    });
    return result;
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

// 平台返回的“试听片段”：有地址但只有 30 秒，视同无地址走音源链
inline bool isTrial(const QVariantMap &map)
{
    if (map.contains("freeTrialInfo") && !map.value("freeTrialInfo").isNull())
        return true;
    for (const char *key : {"trial", "is_trial", "try_begin"}) {
        const QVariant value = map.value(QLatin1String(key));
        if (value.isValid() && value.toLongLong() > 0)
            return true;
    }
    return map.value("size_try").toLongLong() > 0;
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