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

    if (action == ApiAction::SearchSongs) {
        onSearchSongs(ApiArg::stringValue(args, ApiArg::Keyword),
                      ApiArg::intValue(args, ApiArg::Type),
                      ApiArg::intValue(args, ApiArg::Page, 1),
                      ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::SearchLyrics) {
        onSearchLyrics(ApiArg::stringValue(args, ApiArg::Keyword),
                       ApiArg::intValue(args, ApiArg::Page, 1),
                       ApiArg::intValue(args, ApiArg::PageSize, 10));
    } else if (action == ApiAction::PlaylistMenu) {
        onPlaylistMenu(ApiArg::intValue(args, ApiArg::Type));
    } else if (action == ApiAction::MusicPlaylists) {
        onMusicPlaylists(ApiArg::stringValue(args, ApiArg::TagId),
                         ApiArg::intValue(args, ApiArg::Page, 1),
                         ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::CategoryPlaylists) {
        onCategoryPlaylists(ApiArg::stringValue(args, ApiArg::Id),
                            ApiArg::intValue(args, ApiArg::Page, 1),
                            ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::PlaylistSongs) {
        onPlaylistSongs(ApiArg::stringValue(args, ApiArg::ListId),
                        ApiArg::intValue(args, ApiArg::Page, 1),
                        ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::RecommendSongs) {
        onRecommendSongs(ApiArg::intValue(args, ApiArg::Page, 1),
                         ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::HotPlaylistMenu) {
        onHotPlaylistMenu(ApiArg::intValue(args, ApiArg::Type));
    } else if (action == ApiAction::HotPlaylists) {
        onHotPlaylists(ApiArg::intValue(args, ApiArg::Page, 1),
                       ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::NewSongs) {
        onNewSongs(ApiArg::intValue(args, ApiArg::Type),
                   ApiArg::intValue(args, ApiArg::Page, 1),
                   ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::AllToplist) {
        onAllToplist();
    } else if (action == ApiAction::MusicToplist) {
        onMusicToplist(ApiArg::intValue(args, ApiArg::Page, 1),
                       ApiArg::intValue(args, ApiArg::PageSize, 20),
                       ApiArg::intValue(args, ApiArg::RankId));
    } else if (action == ApiAction::HotSingers) {
        onHotSingers(ApiArg::intValue(args, ApiArg::Page, 1),
                     ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::SingerCategory) {
        onSingerCategory(ApiArg::intValue(args, ApiArg::Area),
                         ApiArg::intValue(args, ApiArg::Page, 1),
                         ApiArg::intValue(args, ApiArg::PageSize, 30));
    } else if (action == ApiAction::SingerSongs) {
        onSingerSongs(ApiArg::stringValue(args, ApiArg::SingerId),
                      ApiArg::intValue(args, ApiArg::Page, 1),
                      ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::MusicInfo) {
        onMusicInfo(ApiArg::stringValue(args, ApiArg::Hash),
                    ApiArg::intValue(args, ApiArg::Type));
    } else if (action == ApiAction::LyricInfo) {
        onLyricInfo(ApiArg::stringValue(args, ApiArg::Hash),
                    ApiArg::intValue(args, ApiArg::Duration));
    } else if (action == ApiAction::Comments) {
        onComments(ApiArg::stringValue(args, ApiArg::Hash),
                   ApiArg::intValue(args, ApiArg::Page, 1),
                   ApiArg::intValue(args, ApiArg::PageSize, 30));
    } else if (action == ApiAction::PersonalFm) {
        onPersonalFm(ApiArg::intValue(args, ApiArg::Page, 1),
                     ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else if (action == ApiAction::PersonalRadar) {
        onPersonalRadar(ApiArg::intValue(args, ApiArg::Page, 1),
                        ApiArg::intValue(args, ApiArg::PageSize, 20));
    } else {
        deliverError(action, m_sourceId, -1, QStringLiteral("unsupported action"));
    }
}

void MusicSource::unsupported(const QString &action)
{
    deliverError(action, m_sourceId, -1, QStringLiteral("unsupported action"));
}

void MusicSource::deliver(const QString &action, const QVariant &data, int source)
{
    emit resultReady(action, data, source);
}

void MusicSource::deliverError(const QString &action, int source, int code,
                               const QString &message)
{
    QVariantMap data{{QStringLiteral("error"), code}};
    if (!message.isEmpty())
        data.insert(QStringLiteral("message"), message);
    emit resultReady(action, data, source);
}

