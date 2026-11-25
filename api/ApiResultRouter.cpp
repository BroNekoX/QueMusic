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
    const auto add = [this](const QString &action, OnlineListModel *model, bool songs = false,
                            bool replace = false, bool emptyOnFailure = false) {
        m_targets.insert(action, Target{model, songs, replace, emptyOnFailure});
    };

    add(ApiAction::SearchSongs, m_models.searchResults);
    add(ApiAction::MusicPlaylists, m_models.musicPlaylists);
    add(ApiAction::CategoryPlaylists, m_models.musicPlaylists);
    add(ApiAction::PlaylistSongs, m_models.playlistSongs, true);
    add(ApiAction::MusicToplist, m_models.playlistSongs, true);
    add(ApiAction::SingerSongs, m_models.playlistSongs, true);
    add(ApiAction::RecommendSongs, m_models.recommendSongs);
    add(ApiAction::HotPlaylistMenu, m_models.hotlistMenu, false, true);
    add(ApiAction::HotPlaylists, m_models.hotPlaylists);
    add(ApiAction::RecommendPlaylists, m_models.recommendPlaylists);
    add(ApiAction::NewSongs, m_models.newSongs, true);
    add(ApiAction::HotSingers, m_models.singers);
    add(ApiAction::SingerCategory, m_models.singers);
    add(ApiAction::UserPlaylists, m_models.userPlaylists, false, false, true);
    add(ApiAction::UserFavoriteSongs, m_models.userFavoriteSongs, true, false, true);
    add(ApiAction::FollowedArtists, m_models.followedArtists, false, false, true);
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

    if (info.typeId() == QMetaType::QVariantList && action != ApiAction::Comments
        && action != ApiAction::SearchMvs && action != ApiAction::SearchMvsCandidates)
        m_qualityCache->remember(info.toList());

    // 需要额外信号 / 改写的 action 单独处理，其余走 m_targets 表。
    if (action == ApiAction::SearchMvs || action == ApiAction::SearchMvsCandidates) {
        QVariantList items = ApiCommon::normalizeList(info);
        for (QVariant &entry : items) {
            QVariantMap item = entry.toMap();
            item.insert("source", source);
            entry = item;
        }
        if (action == ApiAction::SearchMvs) {
            if (m_models.searchResults)
                m_models.searchResults->append(items);
        } else {
            emit mvCandidatesReady(items, source);
        }
        return true;
    }
    if (action == ApiAction::MvInfo) {
        emit mvInfoReady(message, source);
        return true;
    }
    if (action == ApiAction::SearchLyrics) {
        emit lyricSearchReady(info, source, failed);
        return true;
    }
    if (action == ApiAction::MusicInfo) {
        emit musicInfoReady(message, source);
        return true;
    }
    if (action == ApiAction::LyricInfo) {
        emit lyricInfoReady(message, source);
        return true;
    }
    if (action == ApiAction::PlaylistMenu) {
        QVariantList menu;
        const QVariantList items = info.toList();
        menu.reserve(items.size());
        for (const QVariant &value : items) {
            const QVariantMap item = ApiCommon::normalizeItem(value.toMap());
            menu << QVariantMap{{"title", item.value(ApiCommon::kTitle)},
                                {"category", item.value("category")},
                                {"id", item.value("id")}};
        }
        m_models.playlistMenu->clear();
        m_models.playlistMenu->append(menu);
        return true;
    }
    if (action == ApiAction::AllToplist) {
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
        return true;
    }
    if (action == ApiAction::PersonalFm || action == ApiAction::PersonalRadar) {
        const bool fm = action == ApiAction::PersonalFm;
        const QVariantList items = info.toList();
        if (items.isEmpty() && !failed) {
            emit personalFallbackWarning(fm ? QStringLiteral("私人漫游需要先在设置中登录账号")
                                           : QStringLiteral("私人雷达需要先在设置中登录账号"));
        } else {
            (fm ? m_models.personalFm : m_models.personalRadar)
                ->append(ApiCommon::normalizeList(items));
        }
        return true;
    }
    if (action == ApiAction::Comments) {
        m_models.comments->append(failed ? QVariantList() : info.toList());
        emit commentsHasMoreChanged(!failed && message.value("hasMore").toBool());
        return true;
    }

    const auto it = m_targets.constFind(action);
    if (it == m_targets.constEnd())
        return false;

    const Target &target = it.value();
    QVariantList items = target.songs ? songList(info) : ApiCommon::normalizeList(info);
    if (target.emptyOnFailure && failed)
        items.clear();
    if (target.replace)
        target.model->clear();
    target.model->append(items);
    return true;
}
