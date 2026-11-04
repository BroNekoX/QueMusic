// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 播放地址 / 歌词 / 下载元数据链路；通过信号向宿主发请求。

#ifndef PLAYBACKCOORDINATOR_H
#define PLAYBACKCOORDINATOR_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QualityHashCache;

class PlaybackCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackCoordinator(QualityHashCache *qualityCache, QObject *parent = nullptr);
    ~PlaybackCoordinator() override = default;

    void setDefaultSource(int source) { m_defaultSource = source; }
    void setKugouLoggedIn(bool loggedIn) { m_kugouLoggedIn = loggedIn; }

    // 播放代次：type 0 的 getMusicInfo 发起时调用，用来源 hash 判定过期响应。
    void beginPlay(const QString &hash, int source);
    void invalidate();

    void handleMusicInfo(const QVariantMap &data, int source);
    void handleLyricInfo(const QVariantMap &data, int source);
    void handleLyricSearchResult(const QVariant &info, int source, bool failed);

    void findLocalLyrics(const QString &filePath, const QString &title, const QString &artist,
                         int duration, int source = -1);
    void readLocalLyricsAsync(const QString &filePath, const QString &title,
                              const QString &artist, int duration, bool allowOnlineSearch);

signals:
    void urlplay(const QString &playurl, const QString &title, const QString &artist,
                 const QString &cover, const QString &solve, const QString &hash, int source);
    void playUrlReady(const QString &hash, const QString &url, const QString &title,
                      const QString &artist, const QString &cover, int duration, int source);
    void playFailed(const QString &hash, int source);
    void warned(const QString &text, int type);

    void lyricsReady(const QVariantList &lyrics, const QVariantList &translate);
    void localLyricsReady(const QString &filePath, const QVariantList &lyrics,
                          const QVariantList &translate);
    void localLyricsFailed(const QString &filePath);

    // 请求宿主代为发送（宿主统一计加载态 / 走音源注册表）。
    void lyricRequested(const QString &hash, int duration, int source);
    void searchRequested(const QString &action, const QVariantMap &args, int source);
    void downloadRequested(const QString &url, const QString &fileName, const QVariantMap &meta);

private:
    struct LyricSearch
    {
        QString filePath; // 为空 = 纯在线回退（B 站无字幕时用）
        QString title;
        QString artist;
        int duration = 0;
    };

    struct BiliTrack
    {
        QString hash;
        QString title;
        QString artist;
        int duration = 0;
    };

    int resolveSource(int source) const { return source < 0 ? m_defaultSource : source; }
    bool isStalePlayResponse(const QString &playHash, int source) const;
    void findOnlineLyrics(const QString &title, const QString &artist, int duration);
    static QString keywordOf(const QString &title, const QString &artist);

    QualityHashCache *m_qualityCache = nullptr;
    int m_defaultSource = 0;
    bool m_kugouLoggedIn = false;

    int m_playGeneration = 0;
    QString m_playHash;
    int m_playSource = -1;

    int m_localLyricsGeneration = 0;
    QHash<int, LyricSearch> m_localLyricSearches;   // 本地文件搜词（结果回填文件）
    QHash<int, LyricSearch> m_onlineLyricSearches;  // B 站无字幕时的在线回退
    QHash<QString, LyricSearch> m_pendingLocalLyrics;
    QHash<QString, int> m_lyricGenerations;
    QHash<QString, QVariantMap> m_pendingDownloads;
    BiliTrack m_biliTrack;
};

#endif // PLAYBACKCOORDINATOR_H
