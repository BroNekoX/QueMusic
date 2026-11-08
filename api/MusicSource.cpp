// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "MusicSource.h"

MusicSource::MusicSource(int sourceId, QObject *parent)
    : QObject(parent), m_sourceId(sourceId)
{
}

void MusicSource::request(int sourceId, const QString &action, const QVariantMap &args)
{
    Q_UNUSED(sourceId)

    auto argInt = [&args](const QString &key, int fallback = 0) { return ApiArg::intValue(args, key, fallback); };
    auto argStr = [&args](const QString &key) { return ApiArg::stringValue(args, key); };

    if (action == ApiAction::SearchSongs)
        onSearchSongs(argStr(ApiArg::Keyword), argInt(ApiArg::Type), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::SearchLyrics)
        onSearchLyrics(argStr(ApiArg::Keyword), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 10));
    else if (action == ApiAction::PlaylistMenu)
        onPlaylistMenu(argInt(ApiArg::Type));
    else if (action == ApiAction::MusicPlaylists)
        onMusicPlaylists(argStr(ApiArg::TagId), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::CategoryPlaylists)
        onCategoryPlaylists(argStr(ApiArg::Id), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::PlaylistSongs)
        onPlaylistSongs(argStr(ApiArg::ListId), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::RecommendSongs)
        onRecommendSongs(argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::HotPlaylistMenu)
        onHotPlaylistMenu(argInt(ApiArg::Type));
    else if (action == ApiAction::HotPlaylists)
        onHotPlaylists(argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::NewSongs)
        onNewSongs(argInt(ApiArg::Type), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::AllToplist)
        onAllToplist();
    else if (action == ApiAction::MusicToplist)
        onMusicToplist(argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20), argInt(ApiArg::RankId));
    else if (action == ApiAction::HotSingers)
        onHotSingers(argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::SingerCategory)
        onSingerCategory(argInt(ApiArg::Area), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 30));
    else if (action == ApiAction::SingerSongs)
        onSingerSongs(argStr(ApiArg::SingerId), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::MusicInfo)
        onMusicInfo(argStr(ApiArg::Hash), argInt(ApiArg::Type));
    else if (action == ApiAction::LyricInfo)
        onLyricInfo(argStr(ApiArg::Hash), argInt(ApiArg::Duration));
    else if (action == ApiAction::Comments)
        onComments(argStr(ApiArg::Hash), argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 30));
    else if (action == ApiAction::PersonalFm)
        onPersonalFm(argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else if (action == ApiAction::PersonalRadar)
        onPersonalRadar(argInt(ApiArg::Page, 1), argInt(ApiArg::PageSize, 20));
    else
        deliverError(action, m_sourceId, -1, "unsupported action");
}

void MusicSource::unsupported(const QString &action)
{
    deliverError(action, m_sourceId, -1, "unsupported action");
}

void MusicSource::deliver(const QString &action, const QVariant &data, int source)
{
    emit resultReady(action, data, source);
}

void MusicSource::deliverError(const QString &action, int source, int code,
                               const QString &message)
{
    QVariantMap data{{"error", code}};
    if (!message.isEmpty())
        data.insert("message", message);
    emit resultReady(action, data, source);
}

