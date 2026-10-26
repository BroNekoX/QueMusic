// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef BILIBILIAPI_H
#define BILIBILIAPI_H

#include <QHash>
#include <QJsonArray>
#include <QList>
#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <functional>

class QJsonObject;
class QNetworkAccessManager;

class BilibiliApi : public QObject
{
    Q_OBJECT
public:
    static constexpr int Source = 2;

    explicit BilibiliApi(QObject *parent = nullptr);

    void setQuality(int q) { m_quality = q; }
    void setCookie(const QString &cookie) { m_loginCookie = cookie; }

    void searchSongs(const QString &keyword, int type, int page, int pageSize);
    void getPlaylistMenu(int type);
    void getMusicPlaylists(const QString &tagid, int page, int pageSize);
    void getPlaylistSongs(const QString &listid, int page, int pageSize);
    void getRecommendSongs(int page, int pageSize);
    void getHotPlaylistMenu(int type);
    void getHotPlaylists(int page, int pageSize);
    void getNewSongs(int type, int page, int pageSize);
    void getAllToplist();
    void getMusicToplist(int page, int pageSize, int rankid);
    void getHotSingers(int page, int pageSize);
    void getSingerCategory(int area, int page, int pageSize);
    void getSingerSongs(const QString &singerid, int page, int pageSize);
    void getMusicInfo(const QString &hash, int type);
    void getLyricInfo(const QString &hash, int duration);
    void getComments(const QString &hash, int page, int pageSize);
    void getPersonalFm(int page, int pageSize);
    void getPersonalRadar(int page, int pageSize);

signals:
    void resultReady(const QString &action, const QVariant &data, int source);

private:
    using Callback = std::function<void(const QJsonObject &)>;
    using Task = std::function<void()>;

    struct PlaylistSearch {
        QString action;
        int pending = 0;
        int limit = 0;
        QVariantList items;
        QSet<QString> seen;
    };

    void get(const QString &url, const Callback &cb, bool withCookie = true);
    void getSigned(const QString &path, QVariantMap params, const Callback &cb,
                   bool withCookie = true);
    void ensureKeys(const Task &then);
    void searchVideos(const QString &keyword, int tid, int page, const QString &action,
                      const QString &order = QString());
    void searchPlaylists(const QString &keyword, int page, int limit, const QString &action);
    void seasonsOf(const QString &action, const QStringList &mids, int limit);
    void fetchSeasons(const QString &mid, const QSharedPointer<PlaylistSearch> &st);
    void seasonSongs(const QString &sid, const QString &mid, int page, int pageSize);
    void ranking(int rid, int page, int pageSize, const QString &action);
    void newVideos(int page, int pageSize, const QString &action);
    void rankingUsers(const QString &action, int limit);
    QStringList upMids(const QJsonArray &items, int limit);
    void emitList(const QString &action, const QVariantList &info);
    void emitLyrics(const QString &hash, const QVariantList &lyrics, int error = 0);

    QVariantMap toSong(const QJsonObject &v, const QString &fallbackArtist = QString());

    static QString mixinKey(const QString &imgKey, const QString &subKey);

    QNetworkAccessManager *m_nam = nullptr;
    QString m_mixinKey;
    QString m_buvid3;
    QString m_buvid4;
    QString m_loginCookie;
    int m_quality = 1;

    QHash<QString, qint64> m_commentCursors;
    QHash<QString, qint64> m_aidCache;
    bool m_keyRequesting = false;
    QList<Task> m_keyWaiters;
    QHash<QString, QString> m_upNames;
};

#endif
