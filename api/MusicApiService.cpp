// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "MusicApiService.h"

#include "ApiActions.h"
#include "ApiCommon.h"
#include "ApiResultRouter.h"
#include "platform/BilibiliApi.h"
#include "platform/KugouApi.h"
#include "LocalMedia.h"
#include "LyricTimeline.h"
#include "platform/NeteaseCloudApi.h"
#include "platform/QQMusicApi.h"
#include "PlaybackCoordinator.h"
#include "QualityHashCache.h"
#include "SelfCheck.h"
#include "playback/Playback.h"
#include "sources/ScriptSourceHost.h"

#include "../cpp/AccountManager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJSEngine>
#include <QQmlEngine>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>

#include <algorithm>

namespace {
constexpr int kRequestWatchdogMs = 20000;
constexpr int kLyricTickMs = 320;

QString appConfigPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QStringLiteral("/BroNekoX");
}
} // namespace

AccountManager *MusicApiService::s_accountManager = nullptr;
MusicApiService *MusicApiService::s_instance = nullptr;

MusicApiService *MusicApiService::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    if (s_instance)
        return s_instance;

    auto *service = new MusicApiService(QCoreApplication::instance());
    QQmlEngine::setObjectOwnership(service, QQmlEngine::CppOwnership);
    service->setAccountManager(s_accountManager);
    s_instance = service;
    Playback::notifyApiReady(service);
    return service;
}

MusicApiService *MusicApiService::instance()
{
    return s_instance;
}

void MusicApiService::setSharedAccountManager(AccountManager *am)
{
    s_accountManager = am;
}

MusicApiService::MusicApiService(QObject *parent)
    : QObject(parent)
{
    m_qualityCache = std::make_unique<QualityHashCache>(this);
    m_localMedia = std::make_unique<LocalMedia>(this);
    m_kugou = std::make_unique<KugouApi>(this);
    m_netease = std::make_unique<NeteaseCloudApi>(this);
    m_bilibili = std::make_unique<BilibiliApi>(this);
    m_qq = std::make_unique<QQMusicApi>(this);
    m_scripts = std::make_unique<ScriptSourceHost>(this);

    OnlineModels models;
    models.searchResults = &m_searchSongsResults;
    models.newSongs = &m_newSongs;
    models.recommendSongs = &m_recommendSongs;
    models.musicPlaylists = &m_musicPlaylists;
    models.playlistMenu = &m_allPlaylistMenu;
    models.playlistSongs = &m_playlistSong;
    models.hotPlaylists = &m_hotPlayLists;
    models.hotlistMenu = &m_getHotlistMenu;
    models.musicToplist = &m_musicToplist;
    models.toplistList = &m_toplistList;
    models.singers = &m_singerList;
    models.personalFm = &m_personalFm;
    models.personalRadar = &m_personalRadar;
    models.comments = &m_comments;

    m_router = std::make_unique<ApiResultRouter>(models, m_qualityCache.get(), this);
    m_playback = std::make_unique<PlaybackCoordinator>(m_qualityCache.get(), this);

    m_registry.map(ApiSource::Kugou, m_kugou.get());
    m_registry.map(ApiSource::Netease, m_netease.get());
    m_registry.map(ApiSource::Bilibili, m_bilibili.get());
    m_registry.map(ApiSource::QQMusic, m_qq.get());

    connect(m_kugou.get(), &KugouApi::resultReady, this, &MusicApiService::handleResult);
    connect(m_netease.get(), &NeteaseCloudApi::resultReady, this, &MusicApiService::handleResult);
    connect(m_bilibili.get(), &BilibiliApi::resultReady, this, &MusicApiService::handleResult);
    connect(m_qq.get(), &QQMusicApi::resultReady, this, &MusicApiService::handleResult);
    connect(m_scripts.get(), &ScriptSourceHost::resultReady, this, &MusicApiService::handleResult);
    connect(m_scripts.get(), &ScriptSourceHost::pluginsChanged, this,
            &MusicApiService::rebuildSources);

    connect(m_router.get(), &ApiResultRouter::commentsHasMoreChanged, this,
            &MusicApiService::setCommentsHasMore);
    connect(m_router.get(), &ApiResultRouter::personalFallbackWarning, this,
            [this](const QString &text) { emit warned(text, 2); });
    connect(m_router.get(), &ApiResultRouter::musicInfoReady, m_playback.get(),
            &PlaybackCoordinator::handleMusicInfo);
    connect(m_router.get(), &ApiResultRouter::lyricInfoReady, m_playback.get(),
            &PlaybackCoordinator::handleLyricInfo);
    connect(m_router.get(), &ApiResultRouter::lyricSearchReady, m_playback.get(),
            &PlaybackCoordinator::handleLyricSearchResult);

    connect(m_playback.get(), &PlaybackCoordinator::urlplay, this, &MusicApiService::urlplay);
    connect(m_playback.get(), &PlaybackCoordinator::playUrlReady, this,
            &MusicApiService::playUrlReady);
    connect(m_playback.get(), &PlaybackCoordinator::playFailed, this,
            &MusicApiService::playFailed);
    connect(m_playback.get(), &PlaybackCoordinator::warned, this, &MusicApiService::warned);
    connect(m_playback.get(), &PlaybackCoordinator::localLyricsReady, this,
            &MusicApiService::localLyricsReady);
    connect(m_playback.get(), &PlaybackCoordinator::localLyricsFailed, this,
            &MusicApiService::localLyricsFailed);
    connect(m_playback.get(), &PlaybackCoordinator::lyricsReady, this,
            [this](const QVariantList &lyrics, const QVariantList &translate) {
                setLyricsData(lyrics);
                setLyricsTranslate(translate);
            });
    connect(m_playback.get(), &PlaybackCoordinator::lyricRequested, this,
            [this](const QString &hash, int duration, int source) {
                send(ApiAction::LyricInfo,
                     QVariantMap{{ApiArg::Hash, hash}, {ApiArg::Duration, duration}},
                     source, false);
            });
    connect(m_playback.get(), &PlaybackCoordinator::searchRequested, this,
            [this](const QString &action, const QVariantMap &args, int source) {
                send(action, args, source, true);
            });
    connect(m_playback.get(), &PlaybackCoordinator::downloadRequested, this,
            [this](const QString &url, const QString &fileName, const QVariantMap &meta) {
                m_downloader.addDownload(url, fileName, meta);
            });

    connect(m_localMedia.get(), &LocalMedia::metadataReady, this,
            &MusicApiService::localMetadataReady);
    connect(m_localMedia.get(), &LocalMedia::metadataSaved, this,
            &MusicApiService::localMetadataSaved);
    connect(&m_downloader, &DownloadManager::downloadPathChanged, this,
            &MusicApiService::downloadPathChanged);
    connect(this, &MusicApiService::songFilterChanged, this,
            [this] { m_router->setSongFilter(m_songFilter); });

    const QString sourcesDir = appConfigPath() + QStringLiteral("/QueMusic/sources");
    QDir().mkpath(sourcesDir);
    m_scripts->setPluginDir(sourcesDir);
    rebuildSources();
    m_scripts->reload();

    QSettings options(appConfigPath() + QStringLiteral("/QueMusic.ini"), QSettings::IniFormat);
    m_soundQuality = options.value(QStringLiteral("Options/soundQuality"), 1).toInt();
    refreshCredentials();

    m_requestWatchdog.setSingleShot(true);
    m_requestWatchdog.setInterval(kRequestWatchdogMs);
    connect(&m_requestWatchdog, &QTimer::timeout, this, [this] {
        if (m_pendingRequests == 0)
            return;
        qWarning() << "[api]" << m_pendingRequests
                   << "个请求超过" << kRequestWatchdogMs / 1000 << "秒没有回结果，强制结束加载态";
        m_pendingRequests = 0;
        m_pendingByAction.clear();
        setLoadState(false);
    });

    m_lyricTimer.setInterval(kLyricTickMs);
    m_lyricTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_lyricTimer, &QTimer::timeout, this, &MusicApiService::advanceLyricIndex);
}

MusicApiService::~MusicApiService()
{
    if (s_instance == this)
        s_instance = nullptr;
}

void MusicApiService::setAccountManager(AccountManager *am)
{
    if (m_account == am)
        return;
    if (m_account)
        disconnect(m_account, nullptr, this, nullptr);

    m_account = am;
    if (m_account) {
        connect(m_account, &AccountManager::neteaseLoginChanged, this,
                &MusicApiService::refreshAccountState);
        connect(m_account, &AccountManager::kugouLoginChanged, this,
                &MusicApiService::refreshAccountState);
    }
    refreshAccountState();
}

void MusicApiService::refreshAccountState()
{
    refreshCredentials();
    if (m_playback)
        m_playback->setKugouLoggedIn(m_account && m_account->isKugouLoggedIn());
}

void MusicApiService::refreshCredentials()
{
    m_credentials.clear();

    if (m_account) {
        SourceCredentials kugou;
        kugou.cookie = m_account->kugouCookie();
        kugou.deviceMid = m_account->kugouMid();
        kugou.deviceDfid = m_account->kugouDfid();
        kugou.loggedIn = m_account->isKugouLoggedIn();
        kugou.quality = m_soundQuality;
        m_credentials.insert(ApiSource::Kugou, kugou);

        SourceCredentials netease;
        netease.cookie = m_account->neteaseApiCookie();
        netease.loggedIn = m_account->isNeteaseLoggedIn();
        netease.quality = m_soundQuality;
        m_credentials.insert(ApiSource::Netease, netease);
    }

    SourceCredentials fallback;
    fallback.quality = m_soundQuality;

    QSet<MusicSource *> applied;
    for (int id : m_registry.ids()) {
        MusicSource *source = m_registry.source(id);
        if (!source || applied.contains(source))
            continue;
        applied.insert(source);
        source->applyCredentials(m_credentials.value(id, fallback));
    }
}

void MusicApiService::setSongSource(int source)
{
    if (source < ApiSource::PluginBase && !m_registry.contains(source)) {
        qWarning() << "[api] 无效音源，回退酷狗:" << source;
        source = ApiSource::Kugou;
    }
    if (m_source == source)
        return;
    m_source = source;
    if (m_playback)
        m_playback->setDefaultSource(source);
    emit songSourceChanged();
}

void MusicApiService::setSoundQuality(int quality)
{
    const int bounded = qBound(0, quality, 3);
    if (m_soundQuality == bounded)
        return;
    m_soundQuality = bounded;
    refreshCredentials();
    emit soundQualityChanged();
}

int MusicApiService::resolveSource(int source) const
{
    return source < 0 ? m_source : source;
}

int MusicApiService::sourceIdAt(int index) const
{
    if (index < 0 || index >= m_sourceIds.size())
        return m_source;
    return m_sourceIds.at(index).toInt();
}

QString MusicApiService::sourceNameOf(int source) const
{
    const int index = m_sourceIds.indexOf(QVariant(source));
    return index >= 0 && index < m_sourceNames.size() ? m_sourceNames.at(index) : QString();
}

QVariantMap MusicApiService::selfCheck() const
{
    SelfCheckContext context;
    context.sources = m_sourceList;
    context.sourceIds = m_sourceIds;
    context.sourceNames = m_sourceNames;
    context.plugins = m_scripts ? m_scripts->plugins() : QVariantList();
    context.registry = &m_registry;
    context.qualityCache = m_qualityCache.get();
    context.currentSource = m_source;
    context.pendingRequests = m_pendingRequests;
    context.lyricTimesSorted = std::is_sorted(m_lyricTimes.cbegin(), m_lyricTimes.cend());
    return ApiSelfCheck::run(context);
}

void MusicApiService::rebuildSources()
{
    rebuildPluginMappings();

    QVariantList list{
        QVariantMap{{QStringLiteral("id"), ApiSource::Kugou},
                    {QStringLiteral("name"), QStringLiteral("酷狗音乐")},
                    {QStringLiteral("plugin"), false}},
        QVariantMap{{QStringLiteral("id"), ApiSource::Netease},
                    {QStringLiteral("name"), QStringLiteral("网易云音乐")},
                    {QStringLiteral("plugin"), false}},
        QVariantMap{{QStringLiteral("id"), ApiSource::Bilibili},
                    {QStringLiteral("name"), QStringLiteral("哔哩哔哩")},
                    {QStringLiteral("plugin"), false}},
        QVariantMap{{QStringLiteral("id"), ApiSource::QQMusic},
                    {QStringLiteral("name"), QStringLiteral("QQ音乐")},
                    {QStringLiteral("plugin"), false}}
    };
    list += m_scripts->plugins();

    m_sourceList = list;
    m_sourceNames.clear();
    m_sourceIds.clear();
    m_sourceNames.reserve(list.size());
    m_sourceIds.reserve(list.size());
    for (const QVariant &entry : list) {
        const QVariantMap info = entry.toMap();
        m_sourceNames.append(info.value(QStringLiteral("name")).toString());
        m_sourceIds.append(info.value(QStringLiteral("id")).toInt());
    }

    refreshCredentials();
    emit sourcesChanged();
}

void MusicApiService::rebuildPluginMappings()
{
    for (int id : std::as_const(m_pluginSourceIds))
        m_registry.unmap(id);
    m_pluginSourceIds.clear();

    const QVariantList plugins = m_scripts->plugins();
    m_pluginSourceIds.reserve(plugins.size());
    for (const QVariant &entry : plugins) {
        const int id = entry.toMap().value(QStringLiteral("id")).toInt();
        if (id <= 0)
            continue;
        m_registry.map(id, m_scripts.get());
        m_pluginSourceIds.append(id);
    }
}

void MusicApiService::reloadSources()
{
    m_scripts->reload();
}

void MusicApiService::send(const QString &action, const QVariantMap &args, int source,
                           bool tracked)
{
    sendResolved(action, args, resolveSource(source), tracked);
}

void MusicApiService::sendResolved(const QString &action, const QVariantMap &args,
                                   int resolvedSource, bool tracked)
{
    if (tracked)
        beginRequest(action);

    MusicSource *provider = m_registry.source(resolvedSource);
    if (!provider) {
        qWarning() << "[api] 音源不可用:" << resolvedSource << "action:" << action;
        if (tracked)
            endRequest(action);
        return;
    }

    provider->request(resolvedSource, action, args);
}

void MusicApiService::searchSongs(const QString &keyword, int type, int page, int pageSize,
                                  int source)
{
    send(ApiAction::SearchSongs,
         QVariantMap{{ApiArg::Keyword, keyword},
                     {ApiArg::Type, type},
                     {ApiArg::Page, page},
                     {ApiArg::PageSize, pageSize}},
         source);
}

void MusicApiService::getPlaylistMenu(int type, int source)
{
    m_allPlaylistMenu.clear();
    m_musicPlaylists.clear();
    send(ApiAction::PlaylistMenu, QVariantMap{{ApiArg::Type, type}}, source);
}

void MusicApiService::getMusicPlaylists(const QString &tagid, int page, int pageSize, int source)
{
    send(ApiAction::MusicPlaylists,
         QVariantMap{{ApiArg::TagId, tagid},
                     {ApiArg::Page, page},
                     {ApiArg::PageSize, pageSize}},
         source);
}

void MusicApiService::getCategoryPlaylists(const QString &id, int page, int pageSize, int source)
{
    send(ApiAction::CategoryPlaylists,
         QVariantMap{{ApiArg::Id, id},
                     {ApiArg::Page, page},
                     {ApiArg::PageSize, pageSize}},
         source);
}

void MusicApiService::getPlaylistSongs(const QString &listid, int page, int pageSize, int source)
{
    send(ApiAction::PlaylistSongs,
         QVariantMap{{ApiArg::ListId, listid},
                     {ApiArg::Page, page},
                     {ApiArg::PageSize, pageSize}},
         source);
}

void MusicApiService::getRecommendSongs(int page, int pageSize, int source)
{
    send(ApiAction::RecommendSongs,
         QVariantMap{{ApiArg::Page, page}, {ApiArg::PageSize, pageSize}}, source);
}

void MusicApiService::getHotPlaylistMenu(int type, int source)
{
    send(ApiAction::HotPlaylistMenu, QVariantMap{{ApiArg::Type, type}}, source);
}

void MusicApiService::getHotPlaylists(int page, int pageSize, int source)
{
    send(ApiAction::HotPlaylists,
         QVariantMap{{ApiArg::Page, page}, {ApiArg::PageSize, pageSize}}, source);
}

void MusicApiService::getNewSongs(int type, int page, int pageSize, int source)
{
    send(ApiAction::NewSongs,
         QVariantMap{{ApiArg::Type, type},
                     {ApiArg::Page, page},
                     {ApiArg::PageSize, pageSize}},
         source);
}

void MusicApiService::getAllToplist(int source)
{
    m_toplistList.clear();
    send(ApiAction::AllToplist, {}, source);
}

void MusicApiService::getMusicToplist(int page, int pageSize, int rankid, int source)
{
    send(ApiAction::MusicToplist,
         QVariantMap{{ApiArg::Page, page},
                     {ApiArg::PageSize, pageSize},
                     {ApiArg::RankId, rankid}},
         source);
}

void MusicApiService::getHotSingers(int page, int pageSize, int source)
{
    send(ApiAction::HotSingers,
         QVariantMap{{ApiArg::Page, page}, {ApiArg::PageSize, pageSize}}, source);
}

void MusicApiService::getSingerCategory(int area, int page, int pageSize, int source)
{
    send(ApiAction::SingerCategory,
         QVariantMap{{ApiArg::Area, area},
                     {ApiArg::Page, page},
                     {ApiArg::PageSize, pageSize}},
         source);
}

void MusicApiService::getSingerSongs(const QString &singerid, int page, int pageSize, int source)
{
    send(ApiAction::SingerSongs,
         QVariantMap{{ApiArg::SingerId, singerid},
                     {ApiArg::Page, page},
                     {ApiArg::PageSize, pageSize}},
         source);
}

void MusicApiService::getMusicInfo(const QString &hash, int type, int source)
{
    const int resolved = resolveSource(source);
    const QString playHash = resolved == ApiSource::Kugou
                                 ? m_qualityCache->resolve(hash, m_soundQuality)
                                 : hash;

    if (type == 0)
        m_playback->beginPlay(playHash, resolved);

    sendResolved(ApiAction::MusicInfo,
                 QVariantMap{{ApiArg::Hash, playHash},
                             {ApiArg::Id, playHash},
                             {ApiArg::Type, type}},
                 resolved, type != 2);
}

void MusicApiService::prefetchMusicInfo(const QString &hash, int source)
{
    if (hash.isEmpty())
        return;

    const int resolved = resolveSource(source);
    MusicSource *provider = m_registry.source(resolved);
    if (!provider || provider->isPlugin())
        return;

    const QString playHash = resolved == ApiSource::Kugou
                                 ? m_qualityCache->resolve(hash, m_soundQuality)
                                 : hash;
    sendResolved(ApiAction::MusicInfo,
                 QVariantMap{{ApiArg::Hash, playHash},
                             {ApiArg::Id, playHash},
                             {ApiArg::Type, 2}},
                 resolved, false);
}

void MusicApiService::invalidatePlayRequest()
{
    m_playback->invalidate();
}

void MusicApiService::getLyricInfo(const QString &hash, int duration, int source)
{
    send(ApiAction::LyricInfo,
         QVariantMap{{ApiArg::Hash, hash}, {ApiArg::Duration, duration}}, source, false);
}

void MusicApiService::getPersonalFm(int page, int pageSize, int source)
{
    send(ApiAction::PersonalFm,
         QVariantMap{{ApiArg::Page, page}, {ApiArg::PageSize, pageSize}}, source);
}

void MusicApiService::getPersonalRadar(int page, int pageSize, int source)
{
    send(ApiAction::PersonalRadar,
         QVariantMap{{ApiArg::Page, page}, {ApiArg::PageSize, pageSize}}, source);
}

void MusicApiService::getComments(const QString &hash, int page, int pageSize, int source)
{
    if (hash.isEmpty())
        return;
    send(ApiAction::Comments,
         QVariantMap{{ApiArg::Hash, hash},
                     {ApiArg::Page, qMax(1, page)},
                     {ApiArg::PageSize, qBound(1, pageSize, 50)}},
         source);
}

QString MusicApiService::qualityHashFor(const QVariantMap &song) const
{
    QString base = song.value(ApiCommon::kHash).toString();
    if (base.isEmpty())
        base = song.value(QStringLiteral("favId")).toString();
    if (base.isEmpty())
        base = song.value(QStringLiteral("id")).toString();
    if (base.isEmpty())
        return QString();

    if (m_soundQuality >= 2) {
        const QString sq = song.value(ApiCommon::kHashSq).toString();
        if (!sq.isEmpty())
            return sq;
    } else if (m_soundQuality == 1) {
        const QString hq = song.value(ApiCommon::kHashHq).toString();
        if (!hq.isEmpty())
            return hq;
    }
    return base;
}

bool MusicApiService::playSong(const QVariantMap &song, int source)
{
    int resolved = source;
    if (resolved < 0) {
        const QVariant itemSource = song.value(QStringLiteral("source"));
        resolved = itemSource.isValid() ? itemSource.toInt() : m_source;
    }
    if (resolved < 0) resolved = m_source;
    if (!m_registry.contains(resolved))
        return false;

    const QString hash = qualityHashFor(song);
    if (hash.isEmpty())
        return false;
    getMusicInfo(hash, 0, resolved);
    return true;
}

bool MusicApiService::downloadSong(const QVariantMap &song, int source)
{
    int resolved = source;
    if (resolved < 0) {
        const QVariant itemSource = song.value(QStringLiteral("source"));
        resolved = itemSource.isValid() ? itemSource.toInt() : m_source;
    }
    if (resolved < 0)
        resolved = m_source;
    if (!m_registry.contains(resolved))
        return false;

    const QString hash = qualityHashFor(song);
    if (hash.isEmpty())
        return false;
    getMusicInfo(hash, 1, resolved);
    return true;
}

void MusicApiService::handleResult(const QString &action, const QVariant &data, int source)
{
    const QVariantMap message = data.toMap();
    const bool prefetch = action == ApiAction::MusicInfo
                          && message.value(ApiArg::Type).toInt() == 2;
    if (!prefetch)
        endRequest(action);

    if (message.contains(QStringLiteral("error"))) {
        qWarning() << "[api] 请求失败:" << action
                   << "error:" << message.value(QStringLiteral("error")).toInt()
                   << "source:" << source;
    }

    if (!m_router->route(action, data, source))
        qWarning() << "MusicApiService: 未知消息类型:" << action;
}

void MusicApiService::setLocalLyrics()
{
    setLyricsData({QVariantMap{{QStringLiteral("time"), 0},
                               {QStringLiteral("text"), QStringLiteral("纯音乐，请欣赏")}}});
    setLyricsTranslate({});
}

void MusicApiService::readLocalMetadataAsync(const QString &filePath)
{
    m_localMedia->readMetadataAsync(filePath);
}

QString MusicApiService::readLocalCoverHint(const QString &filePath)
{
    return m_localMedia->coverHint(filePath);
}

QVariantMap MusicApiService::readLocalAudioInfo(const QString &filePath)
{
    return LocalMedia::readAudioInfo(filePath);
}

QString MusicApiService::readLocalLyricsText(const QString &filePath)
{
    return LocalMedia::readLyricsText(filePath);
}

void MusicApiService::writeLocalMetadata(const QString &filePath, const QVariantMap &meta)
{
    m_localMedia->writeMetadataAsync(filePath, meta);
}

void MusicApiService::readLocalLyricsAsync(const QString &filePath, const QString &title,
                                           const QString &artist, int duration,
                                           bool allowOnlineSearch)
{
    m_playback->readLocalLyricsAsync(filePath, title, artist, duration, allowOnlineSearch);
}

void MusicApiService::findLocalLyrics(const QString &filePath, const QString &title,
                                      const QString &artist, int duration, int source)
{
    m_playback->findLocalLyrics(filePath, title, artist, duration, source);
}

void MusicApiService::setLyricsData(const QVariantList &data)
{
    if (m_lyricsData == data)
        return;
    m_lyricsData = data;
    rebuildLyricTimes(data);
    setLyricIndex(0);
    emit lyricsDataChanged();
}

void MusicApiService::setLyricsTranslate(const QVariantList &translate)
{
    if (m_lyricsTranslate == translate)
        return;
    m_lyricsTranslate = translate;
    emit lyricsTranslateChanged();
}

void MusicApiService::rebuildLyricTimes(const QVariantList &list)
{
    m_lyricTimes.clear();
    m_lyricTimes.reserve(list.size());
    for (const QVariant &item : list)
        m_lyricTimes.append(item.toMap().value(QStringLiteral("time")).toLongLong());
}

void MusicApiService::setLyricIndex(int index)
{
    if (m_lyricIndex != index) {
        m_lyricIndex = index;
        emit lyricIndexChanged();
    }
    emit lyricFuncted();
}

void MusicApiService::setCommentsHasMore(bool hasMore)
{
    if (m_commentsHasMore != hasMore) {
        m_commentsHasMore = hasMore;
        emit commentsHasMoreChanged();
    }
}

void MusicApiService::advanceLyricIndex()
{
    if (m_lyricTimes.isEmpty()) {
        setLyricIndex(0);
        return;
    }

    const qlonglong position = m_lyricPositionMs + m_lyricOffsetMs + kLyricTickMs;
    setLyricIndex(LyricTimeline::indexAt(m_lyricTimes, position));
}

void MusicApiService::setLyricFollowActive(bool active)
{
    if (m_lyricFollowActive == active)
        return;
    m_lyricFollowActive = active;
    if (active) {
        m_lyricTimer.start();
        advanceLyricIndex();
    } else {
        m_lyricTimer.stop();
    }
    emit lyricFollowActiveChanged();
}

void MusicApiService::setLyricPositionMs(qlonglong ms)
{
    if (m_lyricPositionMs == ms)
        return;
    m_lyricPositionMs = ms;
    emit lyricPositionMsChanged();
}

void MusicApiService::setLyricOffsetMs(int ms)
{
    if (m_lyricOffsetMs == ms)
        return;
    m_lyricOffsetMs = ms;
    emit lyricOffsetMsChanged();
}

void MusicApiService::setLoadState(bool state)
{
    if (m_loadState == state)
        return;
    m_loadState = state;
    emit loadStateChanged();
    if (state)
        emit loaded();
    else
        emit finished();
}

void MusicApiService::beginRequest(const QString &action)
{
    ++m_pendingRequests;
    ++m_pendingByAction[action];
    m_requestWatchdog.start();
    setLoadState(true);
}

void MusicApiService::endRequest(const QString &action)
{
    auto it = m_pendingByAction.find(action);
    if (it == m_pendingByAction.end())
        return;
    if (--it.value() <= 0)
        m_pendingByAction.erase(it);

    if (m_pendingRequests > 0)
        --m_pendingRequests;
    if (m_pendingRequests == 0) {
        m_requestWatchdog.stop();
        setLoadState(false);
    } else {
        m_requestWatchdog.start();
    }
}

void MusicApiService::setGlobalid(const QVariant &value)
{
    if (m_globalid == value)
        return;
    m_globalid = value;
    emit globalidChanged();
}

void MusicApiService::setGlobaltagid(const QVariant &value)
{
    if (m_globaltagid == value)
        return;
    m_globaltagid = value;
    emit globaltagidChanged();
}

void MusicApiService::setGlobalinfo(const QVariant &value)
{
    if (m_globalinfo == value)
        return;
    m_globalinfo = value;
    emit globalinfoChanged();
}

void MusicApiService::setNowIndex(int index)
{
    if (m_nowIndex == index)
        return;
    m_nowIndex = index;
    emit nowIndexChanged();
}
