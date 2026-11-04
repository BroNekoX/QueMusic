// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef APICOMMON_H
#define APICOMMON_H

#include <QVariantList>
#include <QVariantMap>

#include <initializer_list>
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

// 统一字段归一化：把平台差异字段（name/singer/pic/id）补齐成 QML 使用的
// 标准字段，并给缺省字段填上稳定类型。所有音源（含脚本插件）共用这一份。
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

    fill(kTitle, QStringLiteral("name"));
    fill(kArtist, QStringLiteral("singer"));
    fill(kCover, QStringLiteral("pic"));
    fill(kHash, QStringLiteral("id"));

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