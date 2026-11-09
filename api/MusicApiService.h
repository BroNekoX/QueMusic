// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef MUSICAPISERVICE_H
#define MUSICAPISERVICE_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

#include <memory>

#include "MusicSource.h"
#include "OnlineListModel.h"
#include "../cpp/DownloadManager.h"

class AccountManager;
class ApiResultRouter;
class BilibiliApi;
class KugouApi;
class LocalMedia;
class NeteaseCloudApi;
class AppleMusicApi;
class PlaybackCoordinator;
class QQMusicApi;
class SpotifyApi;
class QualityHashCache;
class QQmlEngine;
class QJSEngine;
class ScriptSourceHost;

class MusicApiService : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MusicApi)
    QML_SINGLETON

    Q_PROPERTY(int songSource READ songSource WRITE setSongSource NOTIFY songSourceChanged FINAL)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged FINAL)
    Q_PROPERTY(QStringList sourceNames READ sourceNames NOTIFY sourcesChanged FINAL)
    Q_PROPERTY(QVariantList sourceIds READ sourceIds NOTIFY sourcesChanged FINAL)
    Q_PROPERTY(int soundQuality READ soundQuality WRITE setSoundQuality NOTIFY soundQualityChanged FINAL)

    Q_PROPERTY(OnlineListModel *searchSongsResults READ searchSongsResults CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *newSongs READ newSongs CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *recommendSongs READ recommendSongs CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *musicPlaylists READ musicPlaylists CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *allPlaylistMenu READ allPlaylistMenu CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *playlistSong READ playlistSong CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *hotPlayLists READ hotPlayLists CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *getHotlistMenu READ getHotlistMenu CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *musicToplist READ musicToplist CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *toplistList READ toplistList CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *singerList READ singerList CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *personalFm READ personalFm CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *personalRadar READ personalRadar CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel *comments READ comments CONSTANT FINAL)
    Q_PROPERTY(bool commentsHasMore READ commentsHasMore NOTIFY commentsHasMoreChanged FINAL)

    Q_PROPERTY(QVariantList lyricsData READ lyricsData WRITE setLyricsData NOTIFY lyricsDataChanged FINAL)
    Q_PROPERTY(QVariantList lyricsTranslate READ lyricsTranslate WRITE setLyricsTranslate NOTIFY lyricsTranslateChanged FINAL)

    Q_PROPERTY(bool loadState READ loadState WRITE setLoadState NOTIFY loadStateChanged FINAL)
    Q_PROPERTY(QVariant globalid READ globalid WRITE setGlobalid NOTIFY globalidChanged FINAL)
    Q_PROPERTY(QVariant globaltagid READ globaltagid WRITE setGlobaltagid NOTIFY globaltagidChanged FINAL)
    Q_PROPERTY(QVariant globalinfo READ globalinfo WRITE setGlobalinfo NOTIFY globalinfoChanged FINAL)
    Q_PROPERTY(int nowIndex READ nowIndex WRITE setNowIndex NOTIFY nowIndexChanged FINAL)
    Q_PROPERTY(int songFilter MEMBER m_songFilter NOTIFY songFilterChanged FINAL)
    Q_PROPERTY(DownloadManager *downloader READ downloader CONSTANT FINAL)
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged FINAL)

public:
    explicit MusicApiService(QObject *parent);
    ~MusicApiService() override;

    static MusicApiService *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static MusicApiService *instance();
    static void setSharedAccountManager(AccountManager *am);

    void setAccountManager(AccountManager *am);

    int songSource() const { return m_source; }
    void setSongSource(int source);

    int soundQuality() const { return m_soundQuality; }
    void setSoundQuality(int quality);

    QVariantList sources() const { return m_sourceList; }
    QStringList sourceNames() const { return m_sourceNames; }
    QVariantList sourceIds() const { return m_sourceIds; }
    Q_INVOKABLE int sourceIdAt(int index) const;
    Q_INVOKABLE QString sourceNameOf(int source) const;
    Q_INVOKABLE QVariantMap selfCheck() const;
    Q_INVOKABLE void reloadSources();

    OnlineListModel *searchSongsResults() { return &m_searchSongsResults; }
    OnlineListModel *newSongs() { return &m_newSongs; }
    OnlineListModel *recommendSongs() { return &m_recommendSongs; }
    OnlineListModel *musicPlaylists() { return &m_musicPlaylists; }
    OnlineListModel *playlistSong() { return &m_playlistSong; }
    OnlineListModel *hotPlayLists() { return &m_hotPlayLists; }
    OnlineListModel *getHotlistMenu() { return &m_getHotlistMenu; }
    OnlineListModel *musicToplist() { return &m_musicToplist; }
    OnlineListModel *toplistList() { return &m_toplistList; }
    OnlineListModel *singerList() { return &m_singerList; }
    OnlineListModel *personalFm() { return &m_personalFm; }
    OnlineListModel *personalRadar() { return &m_personalRadar; }
    OnlineListModel *comments() { return &m_comments; }
    bool commentsHasMore() const { return m_commentsHasMore; }
    void setCommentsHasMore(bool hasMore);

    // 轻量模式（引擎销毁）前调用：清空全部浏览型/结果型在线列表。
    // 本单例跨引擎销毁常驻，而这些列表只是 QML 页面的浏览缓存，不跨轻量
    // 保留（搜索结果同样不保留）；不清则会构成轻量模式内存的地板。
    // 保留项（见 .cpp 实现）：lyricsData/lyricsTranslate（桌面歌词部件与
    // SMTC 仍需当前歌词）及登录态、音源配置等会话态。
    // 只能在引擎销毁路径（QML 页面即将随引擎一起销毁）调用，此时 model
    // reset 不会触发存活视图的异常刷新。
    void clearTransientModels();

    OnlineListModel *allPlaylistMenu() { return &m_allPlaylistMenu; }
    QVariantList lyricsData() const { return m_lyricsData; }
    void setLyricsData(const QVariantList &data);
    QVariantList lyricsTranslate() const { return m_lyricsTranslate; }
    void setLyricsTranslate(const QVariantList &translate);

    bool loadState() const { return m_loadState; }
    void setLoadState(bool state);
    QVariant globalid() const { return m_globalid; }
    void setGlobalid(const QVariant &value);
    QVariant globaltagid() const { return m_globaltagid; }
    void setGlobaltagid(const QVariant &value);
    QVariant globalinfo() const { return m_globalinfo; }
    void setGlobalinfo(const QVariant &value);
    int nowIndex() const { return m_nowIndex; }
    void setNowIndex(int index);

    DownloadManager *downloader() { return &m_downloader; }
    QString downloadPath() const { return m_downloader.downloadPath(); }
    void setDownloadPath(const QString &path) { m_downloader.setDownloadPath(path); }

    Q_INVOKABLE void searchSongs(const QString &keyword, int type, int page, int pageSize,
                                 int source = -1);
    Q_INVOKABLE void getPlaylistMenu(int type, int source = -1);
    Q_INVOKABLE void getMusicPlaylists(const QString &tagid, int page = 1, int pageSize = 20,
                                       int source = -1);
    Q_INVOKABLE void getCategoryPlaylists(const QString &id, int page = 1, int pageSize = 20,
                                          int source = -1);
    Q_INVOKABLE void getPlaylistSongs(const QString &listid, int page = 1, int pageSize = 20,
                                      int source = -1);
    Q_INVOKABLE void getRecommendSongs(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getHotPlaylistMenu(int type, int source = -1);
    Q_INVOKABLE void getHotPlaylists(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getNewSongs(int type, int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getAllToplist(int source = -1);
    Q_INVOKABLE void getMusicToplist(int page, int pageSize, int rankid, int source = -1);
    Q_INVOKABLE void getHotSingers(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getSingerCategory(int area, int page = 1, int pageSize = 30,
                                       int source = -1);
    Q_INVOKABLE void getSingerSongs(const QString &singerid, int page = 1, int pageSize = 20,
                                    int source = -1);
    Q_INVOKABLE void getMusicInfo(const QString &hash, int type = 0, int source = -1);
    Q_INVOKABLE void prefetchMusicInfo(const QString &hash, int source = -1);
    void invalidatePlayRequest();
    Q_INVOKABLE void getLyricInfo(const QString &hash, int duration, int source = -1);
    Q_INVOKABLE void getPersonalFm(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getPersonalRadar(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getComments(const QString &hash, int page = 1, int pageSize = 30,
                                 int source = -1);

    Q_INVOKABLE bool playSong(const QVariantMap &song, int source = -1);
    Q_INVOKABLE bool downloadSong(const QVariantMap &song, int source = -1);

    Q_INVOKABLE void setLocalLyrics();
    Q_INVOKABLE void readLocalMetadataAsync(const QString &filePath);
    Q_INVOKABLE QString readLocalCoverHint(const QString &filePath);
    Q_INVOKABLE QVariantMap readLocalAudioInfo(const QString &filePath);
    Q_INVOKABLE QString readLocalLyricsText(const QString &filePath);
    Q_INVOKABLE void writeLocalMetadata(const QString &filePath, const QVariantMap &meta);
    Q_INVOKABLE void readLocalLyricsAsync(const QString &filePath, const QString &title,
                                          const QString &artist, int duration = 0,
                                          bool allowOnlineSearch = true);
    Q_INVOKABLE void findLocalLyrics(const QString &filePath, const QString &title,
                                     const QString &artist, int duration = 0,
                                     int source = -1);

signals:
    void loaded();
    void finished();
    void urlplay(const QString &playurl, const QString &title, const QString &artist,
                 const QString &cover, const QString &solve, const QString &hash, int source);
    void warned(const QString &text, int type);
    void playUrlReady(const QString &hash, const QString &url, const QString &title,
                      const QString &artist, const QString &cover, int duration, int source);
    void playFailed(const QString &hash, int source);
    void songSourceChanged();
    void soundQualityChanged();
    void sourcesChanged();
    void lyricsDataChanged();
    void lyricsTranslateChanged();
    void loadStateChanged();
    void globalidChanged();
    void globaltagidChanged();
    void globalinfoChanged();
    void nowIndexChanged();
    void songFilterChanged();
    void commentsHasMoreChanged();
    void downloadPathChanged();
    void localLyricsReady(const QString &filePath, const QVariantList &lyrics,
                          const QVariantList &translate);
    void localLyricsFailed(const QString &filePath);
    void localMetadataReady(const QString &filePath, const QVariantMap &meta);
    void localMetadataSaved(const QString &filePath, bool ok);

private slots:
    void handleResult(const QString &action, const QVariant &data, int source);

private:
    int resolveSource(int source) const;
    void send(const QString &action, const QVariantMap &args, int source, bool tracked = true);
    void sendResolved(const QString &action, const QVariantMap &args, int resolvedSource,
                      bool tracked);
    void refreshCredentials();
    QString qualityHashFor(const QVariantMap &song) const;

    void rebuildSources();
    void rebuildPluginMappings();
    void refreshAccountState();

    void beginRequest(const QString &action);
    void endRequest(const QString &action);

    static AccountManager *s_accountManager;
    static MusicApiService *s_instance;

    int m_source = 0;
    int m_soundQuality = 1;
    int m_songFilter = 0;

    std::unique_ptr<KugouApi> m_kugou;
    std::unique_ptr<NeteaseCloudApi> m_netease;
    std::unique_ptr<BilibiliApi> m_bilibili;
    std::unique_ptr<QQMusicApi> m_qq;
    std::unique_ptr<SpotifyApi> m_spotify;
    std::unique_ptr<AppleMusicApi> m_apple;
    std::unique_ptr<ScriptSourceHost> m_scripts;
    std::unique_ptr<QualityHashCache> m_qualityCache;
    std::unique_ptr<LocalMedia> m_localMedia;
    std::unique_ptr<ApiResultRouter> m_router;
    std::unique_ptr<PlaybackCoordinator> m_playback;
    MusicSourceRegistry m_registry;

    AccountManager *m_account = nullptr;
    QHash<int, SourceCredentials> m_credentials;
    QList<int> m_pluginSourceIds;

    QVariantList m_sourceList;
    QStringList m_sourceNames;
    QVariantList m_sourceIds;
    OnlineListModel m_allPlaylistMenu;
    QVariantList m_lyricsData;
    QVariantList m_lyricsTranslate;

    OnlineListModel m_searchSongsResults;
    OnlineListModel m_newSongs;
    OnlineListModel m_recommendSongs;
    OnlineListModel m_musicPlaylists;
    OnlineListModel m_playlistSong;
    OnlineListModel m_hotPlayLists;
    OnlineListModel m_getHotlistMenu;
    OnlineListModel m_musicToplist;
    OnlineListModel m_toplistList;
    OnlineListModel m_singerList;
    OnlineListModel m_personalFm;
    OnlineListModel m_personalRadar;
    OnlineListModel m_comments;
    bool m_commentsHasMore = false;

    bool m_loadState = false;
    QHash<QString, int> m_pendingByAction;
    int m_pendingRequests = 0;
    QTimer m_requestWatchdog;

    QVariant m_globalid;
    QVariant m_globaltagid;
    QVariant m_globalinfo;
    int m_nowIndex = 0;

    DownloadManager m_downloader;
};

#endif // MUSICAPISERVICE_H
