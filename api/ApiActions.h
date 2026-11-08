// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
// 音源协议：action 名、参数键、内置 source id。

#ifndef APIACTIONS_H
#define APIACTIONS_H

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace ApiSource {

// 0-5 启用；插件从 100 开始。
inline constexpr int Kugou = 0;
inline constexpr int Netease = 1;
inline constexpr int Bilibili = 2;
inline constexpr int QQMusic = 3;
inline constexpr int Spotify = 4;
inline constexpr int AppleMusic = 5;
inline constexpr int PluginBase = 100;

inline bool isBuiltin(int source)
{
    return source >= Kugou && source <= AppleMusic;
}

} // namespace ApiSource

namespace ApiAction {

inline const QString SearchSongs = "searchSongs";
inline const QString SearchLyrics = "searchLyrics";
inline const QString PlaylistMenu = "getPlaylistMenu";
inline const QString MusicPlaylists = "getMusicPlaylists";
inline const QString CategoryPlaylists = "getCategoryPlaylists";
inline const QString PlaylistSongs = "getPlaylistSongs";
inline const QString RecommendSongs = "getRecommendSongs";
inline const QString HotPlaylistMenu = "getHotPlaylistMenu";
inline const QString HotPlaylists = "getHotPlaylists";
inline const QString NewSongs = "getNewSongs";
inline const QString AllToplist = "getAllToplist";
inline const QString MusicToplist = "getMusicToplist";
inline const QString HotSingers = "getHotSingers";
inline const QString SingerCategory = "getSingerCategory";
inline const QString SingerSongs = "getSingerSongs";
inline const QString MusicInfo = "getMusicInfo";
inline const QString LyricInfo = "getLyricInfo";
inline const QString Comments = "getComments";
inline const QString PersonalFm = "getPersonalFm";
inline const QString PersonalRadar = "getPersonalRadar";

// 返回 { "info": [...] } 的 action。
inline bool isListAction(const QString &action)
{
    return action == SearchSongs || action == SearchLyrics
        || action == PlaylistMenu || action == MusicPlaylists
        || action == CategoryPlaylists || action == PlaylistSongs
        || action == RecommendSongs || action == HotPlaylistMenu
        || action == HotPlaylists || action == NewSongs
        || action == AllToplist || action == MusicToplist
        || action == HotSingers || action == SingerCategory
        || action == SingerSongs || action == Comments
        || action == PersonalFm || action == PersonalRadar;
}

} // namespace ApiAction

namespace ApiArg {

inline const QString Keyword = "keyword";
inline const QString Type = "type";
inline const QString Page = "page";
inline const QString PageSize = "pageSize";
inline const QString Id = "id";
inline const QString TagId = "tagid";
inline const QString ListId = "listid";
inline const QString RankId = "rankid";
inline const QString SingerId = "singerid";
inline const QString Area = "area";
inline const QString Hash = "hash";
inline const QString Duration = "duration";

inline int intValue(const QVariantMap &args, const QString &key, int fallback = 0)
{
    const QVariant value = args.value(key);
    return value.isValid() ? value.toInt() : fallback;
}

inline QString stringValue(const QVariantMap &args, const QString &key,
                           const QString &fallback = QString())
{
    const QVariant value = args.value(key);
    if (!value.isValid())
        return fallback;
    const QString text = value.toString();
    return text.isEmpty() ? fallback : text;
}

} // namespace ApiArg

#endif // APIACTIONS_H
