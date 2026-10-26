// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef MUSICAPISERVICE_H
#define MUSICAPISERVICE_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>
#include <QtQmlIntegration/qqmlintegration.h>

#include "BilibiliApi.h"
#include "KugouApi.h"
#include "NeteaseCloudApi.h"
#include "OnlineListModel.h"
#include "sources/ScriptSourceHost.h"
#include "../cpp/DownloadManager.h"

class AccountManager;
class QQmlEngine;
class QJSEngine;

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

    Q_PROPERTY(OnlineListModel* searchSongsResults READ searchSongsResults CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* newSongs READ newSongs CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* recommendSongs READ recommendSongs CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* musicPlaylists READ musicPlaylists CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* playlistSong READ playlistSong CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* hotPlayLists READ hotPlayLists CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* getHotlistMenu READ getHotlistMenu CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* musicToplist READ musicToplist CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* toplistList READ toplistList CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* singerList READ singerList CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* personalFm READ personalFm CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* personalRadar READ personalRadar CONSTANT FINAL)
    Q_PROPERTY(OnlineListModel* comments READ comments CONSTANT FINAL)
    Q_PROPERTY(bool commentsHasMore READ commentsHasMore NOTIFY commentsHasMoreChanged FINAL)

    Q_PROPERTY(QVariant allPlaylistMenu READ allPlaylistMenu WRITE setAllPlaylistMenu NOTIFY allPlaylistMenuChanged FINAL)
    Q_PROPERTY(QVariantList lyricsData READ lyricsData WRITE setLyricsData NOTIFY lyricsDataChanged FINAL)
    Q_PROPERTY(QVariantList lyricsTranslate READ lyricsTranslate WRITE setLyricsTranslate NOTIFY lyricsTranslateChanged FINAL)

    Q_PROPERTY(bool lyricFollowActive READ lyricFollowActive WRITE setLyricFollowActive NOTIFY lyricFollowActiveChanged FINAL)
    Q_PROPERTY(qlonglong lyricPositionMs READ lyricPositionMs WRITE setLyricPositionMs NOTIFY lyricPositionMsChanged FINAL)
    Q_PROPERTY(int lyricOffsetMs READ lyricOffsetMs WRITE setLyricOffsetMs NOTIFY lyricOffsetMsChanged FINAL)
    Q_PROPERTY(int lyricIndex READ lyricIndex NOTIFY lyricIndexChanged FINAL)

    Q_PROPERTY(bool loadState READ loadState WRITE setLoadState NOTIFY loadStateChanged FINAL)
    Q_PROPERTY(QVariant globalid READ globalid WRITE setGlobalid NOTIFY globalidChanged FINAL)
    Q_PROPERTY(QVariant globaltagid READ globaltagid WRITE setGlobaltagid NOTIFY globaltagidChanged FINAL)
    Q_PROPERTY(QVariant globalinfo READ globalinfo WRITE setGlobalinfo NOTIFY globalinfoChanged FINAL)
    Q_PROPERTY(int nowIndex READ nowIndex WRITE setNowIndex NOTIFY nowIndexChanged FINAL)
    Q_PROPERTY(int songFilter MEMBER m_songFilter NOTIFY songFilterChanged FINAL)
    Q_PROPERTY(DownloadManager* downloader READ downloader CONSTANT FINAL)
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged FINAL)

public:
    explicit MusicApiService(QObject *parent);
    ~MusicApiService() override;

    static MusicApiService *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    static void setSharedAccountManager(AccountManager *am);

    void setAccountManager(AccountManager *am);

    int songSource() const;
    void setSongSource(int source);

    int soundQuality() const;
    void setSoundQuality(int q);

    QVariantList sources() const;
    QStringList sourceNames() const;
    QVariantList sourceIds() const;
    Q_INVOKABLE int sourceIdAt(int index) const;
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

    QVariant allPlaylistMenu() const { return m_allPlaylistMenu; }
    void setAllPlaylistMenu(const QVariant &v);
    QVariantList lyricsData() const { return m_lyricsData; }
    void setLyricsData(const QVariantList &v);
    QVariantList lyricsTranslate() const { return m_lyricsTranslate; }
    void setLyricsTranslate(const QVariantList &v);

    bool lyricFollowActive() const { return m_lyricFollowActive; }
    void setLyricFollowActive(bool active);
    qlonglong lyricPositionMs() const { return m_lyricPositionMs; }
    void setLyricPositionMs(qlonglong ms);
    int lyricOffsetMs() const { return m_lyricOffsetMs; }
    void setLyricOffsetMs(int ms);
    int lyricIndex() const { return m_lyricIndex; }

    bool loadState() const { return m_loadState; }
    void setLoadState(bool state);
    QVariant globalid() const { return m_globalid; }
    void setGlobalid(const QVariant &v);
    QVariant globaltagid() const { return m_globaltagid; }
    void setGlobaltagid(const QVariant &v);
    QVariant globalinfo() const { return m_globalinfo; }
    void setGlobalinfo(const QVariant &v);
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
    Q_INVOKABLE void getAllToplists();
    Q_INVOKABLE void getMusicToplist(int page, int pageSize, int rankid, int source = -1);
    Q_INVOKABLE void getHotSingers(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getSingerCategory(int area, int page = 1, int pageSize = 30,
                                       int source = -1);
    Q_INVOKABLE void getSingerSongs(const QString &singerid, int page = 1, int pageSize = 20,
                                    int source = -1);
    Q_INVOKABLE void getMusicInfo(const QString &hash, int type = 0, int source = -1);
    Q_INVOKABLE void getLyricInfo(const QString &hash, int duration, int source = -1);
    Q_INVOKABLE void getPersonalFm(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getPersonalRadar(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getComments(const QString &hash, int page = 1, int pageSize = 30,
                                 int source = -1);
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
    void findOnlineLyrics(const QString &title, const QString &artist, int duration,
                          int source = 0);

signals:
    void loaded();
    void finished();
    void urlplay(const QString &playurl, const QString &title, const QString &artist,
                 const QString &cover, const QString &solve, const QString &hash, int source);
    void warned(const QString &text, int type);
    void songSourceChanged();
    void soundQualityChanged();
    void sourcesChanged();
    void allPlaylistMenuChanged();
    void lyricsDataChanged();
    void lyricsTranslateChanged();
    void lyricFollowActiveChanged();
    void lyricPositionMsChanged();
    void lyricOffsetMsChanged();
    void lyricIndexChanged();
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
    void rebuildSources();
    QVariantList songList(const QVariant &data);
    int m_songFilter = 0;

    int resolve(int source) const;
    static QVariantMap readLocalMetadataBlocking(const QString &filePath);
    static bool writeLocalMetadataBlocking(const QString &filePath, const QVariantMap &meta);
    void beginRequest();
    void endRequest();
    QTimer m_requestWatchdog;
    bool isStalePlayResponse(const QString &playHash, int source) const;
    void rememberHashes(const QVariantList &items);
    QString resolveQualityHash(const QString &hash) const;
    QString qualityCachePath() const;
    void loadQualityCache();
    void scheduleSaveQualityCache();
    void saveQualityCache();
    void syncSource(int source);
    QVariantMap normalizeItem(const QVariantMap &raw);
    QVariantList normalizeList(const QVariant &value);
    void handleMusicInfo(const QVariantMap &data, int source);

    struct LocalLyricsRequest {
        QString filePath;
        QString title;
        QString artist;
        int duration = 0;
    };

    struct QualityAlts {
        QString hq;
        QString sq;
    };

    struct OnlineTrack {
        QString hash;
        QString title;
        QString artist;
        int duration = 0;
    };

    int m_source = 0;
    int m_soundQuality = 1;
    int m_localLyricsGeneration = 0;
    QHash<QString, QString> m_coverHintCache;
    QHash<QString, QualityAlts> m_alts;
    QHash<QString, QString> m_baseOf;
    bool m_altsDirty = false;
    QTimer m_altsSaveTimer;
    QVariantList m_sourceList;
    NeteaseCloudApi m_netease;
    KugouApi m_kugou;
    BilibiliApi m_bilibili;
    ScriptSourceHost m_scripts;
    AccountManager *m_account = nullptr;
    static AccountManager *s_accountManager;

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

    QVariant m_allPlaylistMenu;
    QVariantList m_lyricsData;
    QVariantList m_lyricsTranslate;

    QMap<QString, QVariantMap> m_pendingDownloads;
    QMap<int, LocalLyricsRequest> m_localLyricsSearches;
    QMap<QString, LocalLyricsRequest> m_pendingLocalLyrics;
    OnlineTrack m_biliTrack;
    QMap<int, LocalLyricsRequest> m_biliFallbackSearches;

    bool m_loadState = false;
    int m_pendingRequests = 0;
    int m_playGeneration = 0;
    QString m_playHash;
    int m_playSource = -1;
    QHash<QString, int> m_playLyricGenerations;
    QVariant m_globalid;
    QVariant m_globaltagid;
    QVariant m_globalinfo;
    int m_nowIndex = 0;

    void setLyricIndex(int index);
    void rebuildLyricTimes(const QVariantList &list);
    void advanceLyricIndex();
    QTimer m_lyricTimer;
    QList<qlonglong> m_lyricTimes;
    qlonglong m_lyricPositionMs = 0;
    int m_lyricOffsetMs = 0;
    int m_lyricIndex = 0;
    bool m_lyricFollowActive = false;

    DownloadManager m_downloader;
};

#endif