// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef MUSICAPISERVICE_H
#define MUSICAPISERVICE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QtQmlIntegration/qqmlintegration.h>

#include "KugouApi.h"
#include "NeteaseCloudApi.h"   // 基于 QCloudMusicApi 的网易云实现（替代旧 NeteaseApi）
#include "OnlineListModel.h"
#include "../cpp/DownloadManager.h"

class AccountManager;
class QQmlEngine;
class QJSEngine;

class MusicApiService : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MusicApi)
    QML_SINGLETON

    // 默认歌曲源（0 酷狗 / 1 网易云），source<0 时使用
    Q_PROPERTY(int songSource READ songSource WRITE setSongSource NOTIFY songSourceChanged)

    // 在线数据列表模型（QML 侧 .count/.get()/.clear() 与旧 ListModel 一致）
    Q_PROPERTY(OnlineListModel* searchSongsResults READ searchSongsResults CONSTANT)
    Q_PROPERTY(OnlineListModel* newSongs READ newSongs CONSTANT)
    Q_PROPERTY(OnlineListModel* recommendSongs READ recommendSongs CONSTANT)
    Q_PROPERTY(OnlineListModel* musicPlaylists READ musicPlaylists CONSTANT)
    Q_PROPERTY(OnlineListModel* playlistSong READ playlistSong CONSTANT)
    Q_PROPERTY(OnlineListModel* hotPlayLists READ hotPlayLists CONSTANT)
    Q_PROPERTY(OnlineListModel* getHotlistMenu READ getHotlistMenu CONSTANT)
    Q_PROPERTY(OnlineListModel* musicToplist READ musicToplist CONSTANT)
    Q_PROPERTY(OnlineListModel* toplistList READ toplistList CONSTANT)
    Q_PROPERTY(OnlineListModel* singerList READ singerList CONSTANT)
    Q_PROPERTY(OnlineListModel* personalFm READ personalFm CONSTANT)
    Q_PROPERTY(OnlineListModel* personalRadar READ personalRadar CONSTANT)

    // 非模型数据
    Q_PROPERTY(QVariant allPlaylistMenu READ allPlaylistMenu WRITE setAllPlaylistMenu NOTIFY allPlaylistMenuChanged)
    Q_PROPERTY(QVariant playlistmenuInfo READ playlistmenuInfo WRITE setPlaylistmenuInfo NOTIFY playlistmenuInfoChanged)
    Q_PROPERTY(QVariant lyricsData READ lyricsData WRITE setLyricsData NOTIFY lyricsDataChanged)
    Q_PROPERTY(QVariant lyricsTranslate READ lyricsTranslate WRITE setLyricsTranslate NOTIFY lyricsTranslateChanged)

    // 状态与全局变量
    Q_PROPERTY(bool loadState READ loadState WRITE setLoadState NOTIFY loadStateChanged)
    Q_PROPERTY(QVariant globalid READ globalid WRITE setGlobalid NOTIFY globalidChanged)
    Q_PROPERTY(QVariant globaltagid READ globaltagid WRITE setGlobaltagid NOTIFY globaltagidChanged)
    Q_PROPERTY(QVariant globalinfo READ globalinfo WRITE setGlobalinfo NOTIFY globalinfoChanged)
    Q_PROPERTY(int nowIndex READ nowIndex WRITE setNowIndex NOTIFY nowIndexChanged)

    // 下载管理器（DownloadPage 直接绑定 MusicApi.downloader）
    Q_PROPERTY(DownloadManager* downloader READ downloader CONSTANT)
    Q_PROPERTY(QString downloadPath READ downloadPath WRITE setDownloadPath NOTIFY downloadPathChanged)

public:
    explicit MusicApiService(QObject *parent = nullptr);

    // QML_SINGLETON 工厂：首次访问 MusicApi 时由 QML 引擎调用
    static MusicApiService *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    // main.cpp 注入共享的 AccountManager，供 create() 使用
    static void setSharedAccountManager(AccountManager *am);

    void setAccountManager(AccountManager *am); // 请求自动携带登录态 Cookie

    int songSource() const;
    void setSongSource(int source);

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

    QVariant allPlaylistMenu() const { return m_allPlaylistMenu; }
    void setAllPlaylistMenu(const QVariant &v);
    QVariant playlistmenuInfo() const { return m_playlistmenuInfo; }
    void setPlaylistmenuInfo(const QVariant &v);
    QVariant lyricsData() const { return m_lyricsData; }
    void setLyricsData(const QVariant &v);
    QVariant lyricsTranslate() const { return m_lyricsTranslate; }
    void setLyricsTranslate(const QVariant &v);

    bool loadState() const { return m_loadState; }
    void setLoadState(bool s);
    QVariant globalid() const { return m_globalid; }
    void setGlobalid(const QVariant &v);
    QVariant globaltagid() const { return m_globaltagid; }
    void setGlobaltagid(const QVariant &v);
    QVariant globalinfo() const { return m_globalinfo; }
    void setGlobalinfo(const QVariant &v);
    int nowIndex() const { return m_nowIndex; }
    void setNowIndex(int v);

    DownloadManager *downloader() { return &m_downloader; }
    QString downloadPath() const { return m_downloader.downloadPath(); }
    void setDownloadPath(const QString &p) { m_downloader.setDownloadPath(p); }

    // 统一接口（缺省 source 时用当前默认源）
    Q_INVOKABLE void searchSongs(const QString &keyword, int type, int page, int pageSize,
                                 int source = -1);
    Q_INVOKABLE void getPlaylistMenu(int type, int source = -1);
    Q_INVOKABLE void getMenuInfo(const QString &id, int source = -1);
    Q_INVOKABLE void getMusicPlaylists(const QString &tagid, int page = 1, int pageSize = 20,
                                       int source = -1);
    Q_INVOKABLE void getPlaylistSongs(const QString &listid, int page = 1, int pageSize = 20,
                                      int source = -1);
    Q_INVOKABLE void getRecommendSongs(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getHotPlaylistMenu(int type, int source = -1);
    Q_INVOKABLE void getHotPlaylists(int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getNewSongs(int type, int page = 1, int pageSize = 20, int source = -1);
    Q_INVOKABLE void getAllToplist(int source = -1);   // 榜单列表（单平台）
    Q_INVOKABLE void getAllToplists();           // 酷狗 + 网易云榜单
    Q_INVOKABLE void getMusicToplist(int page, int pageSize, int rankid, int source = -1); // 榜单歌曲
    Q_INVOKABLE void getHotSingers(int page = 1, int pageSize = 20, int source = -1);
    // 歌手分类：area 由 QML 按平台映射（酷狗 1 华语/2 欧美/3 日本/4 韩国；网易云 7/96/8/16）
    Q_INVOKABLE void getSingerCategory(int area, int page = 1, int pageSize = 30,
                                       int source = -1);
    Q_INVOKABLE void getSingerSongs(const QString &singerid, int page = 1, int pageSize = 20,
                                    int source = -1);
    Q_INVOKABLE void getMusicInfo(const QString &hash, int type = 0, int source = -1);
    Q_INVOKABLE void getLyricInfo(const QString &hash, int duration, int source = -1);
    // 私人漫游（每日推荐式流媒体，网易云 personal_fm / 酷狗推荐榜）
    Q_INVOKABLE void getPersonalFm(int page = 1, int pageSize = 20, int source = -1);
    // 私人雷达（基于用户口味的推荐，网易云 recommend_songs / 酷狗新歌榜）
    Q_INVOKABLE void getPersonalRadar(int page = 1, int pageSize = 20, int source = -1);
    // 本地音乐（无歌词）时调用：清掉在线歌词残留，显示占位歌词 [{time:0, text:"纯音乐，请欣赏"}]
    Q_INVOKABLE void setLocalLyrics();
    // 读取本地音频同目录同名 .json 元数据（不存在返回空 map）
    Q_INVOKABLE QVariantMap readLocalMetadata(const QString &filePath);
    // 读取本地歌词：同名 .lrc 优先，其次读取音频内嵌歌词。
    Q_INVOKABLE QVariantMap readLocalLyrics(const QString &filePath);
    // 本地歌词不存在时，按标题/歌手搜索在线歌词；结果通过信号返回。
    Q_INVOKABLE void findLocalLyrics(const QString &filePath, const QString &title,
                                     const QString &artist, int duration = 0,
                                     int source = -1);

signals:
    void loaded();   // loadState 置 true（QLoadSign 显示加载动画）
    void finished(); // loadState 置 false（QLoadSign 结束动画）
    void urlplay(const QString &playurl, const QString &title, const QString &artist,
                 const QString &cover, const QString &solve, const QString &hash, int source);
    void warned(const QString &text, int type); // 下载等提示
    void songSourceChanged();
    void allPlaylistMenuChanged();
    void playlistmenuInfoChanged();
    void lyricsDataChanged();
    void lyricsTranslateChanged();
    void loadStateChanged();
    void globalidChanged();
    void globaltagidChanged();
    void globalinfoChanged();
    void nowIndexChanged();
    void downloadPathChanged();
    void localLyricsReady(const QString &filePath, const QVariantList &lyrics);
    void localLyricsFailed(const QString &filePath);

private slots:
    void handleResult(const QString &action, const QVariant &data, int source);

private:
    int resolve(int source) const; // source<0 → 默认源
    void syncCookie(int source);   // 同步 AccountManager 登录态 Cookie
    QVariantMap normalizeItem(const QVariantMap &raw); // 字段归一化 + 旧字段别名
    QVariantList normalizeList(const QVariant &v);
    void handleMusicInfo(const QVariantMap &d, int source);

    struct LocalLyricsRequest {
        QString filePath;
        QString title;
        QString artist;
        int duration = 0;
    };

    int m_source = 0;
    NeteaseCloudApi m_netease;   // 网易云（源 1）：基于 QCloudMusicApi（weapi 加密协议）
    KugouApi m_kugou;
    AccountManager *m_account = nullptr;
    static AccountManager *s_accountManager; // create() 使用，main.cpp 注入

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

    QVariant m_allPlaylistMenu;
    QVariant m_playlistmenuInfo;
    QVariant m_lyricsData;
    QVariant m_lyricsTranslate;

    // 下载流程：按 hash 暂存待下载元数据，等歌词返回后一起发起下载
    QMap<QString, QVariantMap> m_pendingDownloads;
    QMap<int, LocalLyricsRequest> m_localLyricsSearches;
    QMap<QString, LocalLyricsRequest> m_pendingLocalLyrics;

    bool m_loadState = false;
    QVariant m_globalid;
    QVariant m_globaltagid;
    QVariant m_globalinfo;
    int m_nowIndex = 0;

    DownloadManager m_downloader;
};

#endif // MUSICAPISERVICE_H
