// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 在线音源协议层：action 名、参数键、内置音源编号。
//
// 这一层是「宿主 <-> 音源」之间唯一的字符串协议。新增一个平台时，
// 只需要实现 MusicSource::request() 里对应的 action，不需要改调度层。

#ifndef APIACTIONS_H
#define APIACTIONS_H

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace ApiSource {

// 内置音源编号。0/1/2 是历史值，QML 侧与用户配置里都按这些数字持久化，
// 不要修改。脚本音源从 100 开始分配（见 ScriptSourceHost）。
inline constexpr int Kugou = 0;
inline constexpr int Netease = 1;
inline constexpr int Bilibili = 2;
inline constexpr int QQMusic = 3;
inline constexpr int Spotify = 4;     // 预留
inline constexpr int AppleMusic = 5;  // 预留
inline constexpr int PluginBase = 100;

inline bool isBuiltin(int source)
{
    return source >= Kugou && source <= AppleMusic;
}

} // namespace ApiSource

namespace ApiAction {

inline const QString SearchSongs = QStringLiteral("searchSongs");
inline const QString SearchLyrics = QStringLiteral("searchLyrics");
inline const QString PlaylistMenu = QStringLiteral("getPlaylistMenu");
inline const QString MusicPlaylists = QStringLiteral("getMusicPlaylists");
inline const QString CategoryPlaylists = QStringLiteral("getCategoryPlaylists");
inline const QString PlaylistSongs = QStringLiteral("getPlaylistSongs");
inline const QString RecommendSongs = QStringLiteral("getRecommendSongs");
inline const QString HotPlaylistMenu = QStringLiteral("getHotPlaylistMenu");
inline const QString HotPlaylists = QStringLiteral("getHotPlaylists");
inline const QString NewSongs = QStringLiteral("getNewSongs");
inline const QString AllToplist = QStringLiteral("getAllToplist");
inline const QString MusicToplist = QStringLiteral("getMusicToplist");
inline const QString HotSingers = QStringLiteral("getHotSingers");
inline const QString SingerCategory = QStringLiteral("getSingerCategory");
inline const QString SingerSongs = QStringLiteral("getSingerSongs");
inline const QString MusicInfo = QStringLiteral("getMusicInfo");
inline const QString LyricInfo = QStringLiteral("getLyricInfo");
inline const QString Comments = QStringLiteral("getComments");
inline const QString PersonalFm = QStringLiteral("getPersonalFm");
inline const QString PersonalRadar = QStringLiteral("getPersonalRadar");

// 所有返回 { "info": [...] } 的 action。脚本音源与结果路由共用，
// 避免两边各维护一份容易漂移的清单。
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

inline const QString Keyword = QStringLiteral("keyword");
inline const QString Type = QStringLiteral("type");
inline const QString Page = QStringLiteral("page");
inline const QString PageSize = QStringLiteral("pageSize");
inline const QString Id = QStringLiteral("id");
inline const QString TagId = QStringLiteral("tagid");
inline const QString ListId = QStringLiteral("listid");
inline const QString RankId = QStringLiteral("rankid");
inline const QString SingerId = QStringLiteral("singerid");
inline const QString Area = QStringLiteral("area");
inline const QString Hash = QStringLiteral("hash");
inline const QString Duration = QStringLiteral("duration");

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
