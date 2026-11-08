// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "ApiResultRouter.h"

#include "ApiActions.h"
#include "ApiCommon.h"
#include "OnlineListModel.h"
#include "QualityHashCache.h"

#include <QVariant>

ApiResultRouter::ApiResultRouter(const OnlineModels &models, QualityHashCache *qualityCache,
                                 QObject *parent)
    : QObject(parent), m_models(models), m_qualityCache(qualityCache)
{
}

QVariantList ApiResultRouter::songList(const QVariant &value) const
{
    const QVariantList normalized = ApiCommon::normalizeList(value);
    if (m_songFilter == 0)
        return normalized;

    QVariantList filtered;
    filtered.reserve(normalized.size());
    for (const QVariant &entry : normalized) {
        const QVariantMap item = entry.toMap();
        const bool free = item.value(ApiCommon::kPaytype).toInt() != ApiCommon::kPaytypePaid;
        if ((m_songFilter == 1 && !free) || (m_songFilter == 2 && free))
            continue;
        filtered.append(item);
    }
    return filtered;
}

bool ApiResultRouter::route(const QString &action, const QVariant &data, int source)
{
    const QVariantMap message = data.toMap();
    const bool failed = message.contains("error");
    const QVariant info = message.contains("info") ? message.value("info") : data;

    if (info.typeId() == QMetaType::QVariantList && action != ApiAction::Comments)
        m_qualityCache->remember(info.toList());

    if (action == ApiAction::SearchSongs) {
        m_models.searchResults->append(ApiCommon::normalizeList(info));
    } else if (action == ApiAction::SearchLyrics) {
        emit lyricSearchReady(info, source, failed);
    } else if (action == ApiAction::PlaylistMenu) {
        QVariantList menu;
        const QVariantList items = info.toList();
        menu.reserve(items.size());
        for (const QVariant &value : items) {
            const QVariantMap item = ApiCommon::normalizeItem(value.toMap());
            menu << QVariantMap{{"title", item.value(ApiCommon::kTitle)}, {"category", item.value("category")}, {"id", item.value("id")}};
        }
        m_models.playlistMenu->clear();
        m_models.playlistMenu->append(menu);
    } else if (action == ApiAction::MusicPlaylists
               || action == ApiAction::CategoryPlaylists) {
        m_models.musicPlaylists->append(ApiCommon::normalizeList(info));
    } else if (action == ApiAction::PlaylistSongs
               || action == ApiAction::MusicToplist
               || action == ApiAction::SingerSongs) {
        m_models.playlistSongs->append(songList(info));
    } else if (action == ApiAction::RecommendSongs) {
        m_models.recommendSongs->append(ApiCommon::normalizeList(info));
    } else if (action == ApiAction::HotPlaylistMenu) {
        m_models.hotlistMenu->clear();
        m_models.hotlistMenu->append(ApiCommon::normalizeList(info));
    } else if (action == ApiAction::HotPlaylists) {
        m_models.hotPlaylists->append(ApiCommon::normalizeList(info));
    } else if (action == ApiAction::NewSongs) {
        m_models.newSongs->append(songList(info));
    } else if (action == ApiAction::AllToplist) {
        const QVariantList normalized = ApiCommon::normalizeList(info);
        m_models.musicToplist->clear();
        m_models.musicToplist->append(normalized);

        QVariantList withSource;
        withSource.reserve(normalized.size());
        for (const QVariant &value : normalized) {
            QVariantMap item = value.toMap();
            item.insert("source", source);
            withSource.append(item);
        }
        m_models.toplistList->append(withSource);
    } else if (action == ApiAction::HotSingers
               || action == ApiAction::SingerCategory) {
        m_models.singers->append(ApiCommon::normalizeList(info));
    } else if (action == ApiAction::PersonalFm
               || action == ApiAction::PersonalRadar) {
        const bool fm = action == ApiAction::PersonalFm;
        const QVariantList items = info.toList();
        if (items.isEmpty() && !failed) {
            emit personalFallbackWarning(fm ? QStringLiteral("私人漫游需要先在设置中登录账号")
                                           : QStringLiteral("私人雷达需要先在设置中登录账号"));
        } else {
            (fm ? m_models.personalFm : m_models.personalRadar)
                ->append(ApiCommon::normalizeList(items));
        }
    } else if (action == ApiAction::MusicInfo) {
        emit musicInfoReady(message, source);
    } else if (action == ApiAction::LyricInfo) {
        emit lyricInfoReady(message, source);
    } else if (action == ApiAction::Comments) {
        m_models.comments->append(failed ? QVariantList() : info.toList());
        emit commentsHasMoreChanged(!failed && message.value("hasMore").toBool());
    } else {
        return false;
    }
    return true;
}
