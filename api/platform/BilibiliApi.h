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

#include "MusicSource.h"

class QJsonObject;
class QNetworkAccessManager;

class BilibiliApi : public MusicSource
{
    Q_OBJECT
public:
    static constexpr int Source = ApiSource::Bilibili;

    explicit BilibiliApi(QObject *parent = nullptr);
    ~BilibiliApi() override = default;

    QString sourceName() const override { return QStringLiteral("哔哩哔哩"); }

    void applyCredentials(const SourceCredentials &credentials) override;

    void setQuality(int quality) { m_quality = qBound(0, quality, 3); }
    void setCookie(const QString &cookie) { m_loginCookie = cookie; }

protected:
    void onSearchSongs(const QString &keyword, int type, int page, int pageSize) override;
    void onSearchLyrics(const QString &keyword, int page, int pageSize) override;
    void onPlaylistMenu(int type) override;
    void onMusicPlaylists(const QString &tagid, int page, int pageSize) override;
    void onCategoryPlaylists(const QString &id, int page, int pageSize) override;
    void onPlaylistSongs(const QString &listid, int page, int pageSize) override;
    void onRecommendSongs(int page, int pageSize) override;
    void onHotPlaylistMenu(int type) override;
    void onHotPlaylists(int page, int pageSize) override;
    void onNewSongs(int type, int page, int pageSize) override;
    void onAllToplist() override;
    void onMusicToplist(int page, int pageSize, int rankid) override;
    void onHotSingers(int page, int pageSize) override;
    void onSingerCategory(int area, int page, int pageSize) override;
    void onSingerSongs(const QString &singerid, int page, int pageSize) override;
    void onMusicInfo(const QString &hash, int type) override;
    void onLyricInfo(const QString &hash, int duration) override;
    void onComments(const QString &hash, int page, int pageSize) override;
    void onPersonalFm(int page, int pageSize) override;
    void onPersonalRadar(int page, int pageSize) override;

private:
    using Callback = std::function<void(const QJsonObject &)>;
    using Task = std::function<void()>;

    struct PlaylistSearch
    {
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
    void rememberUpName(const QString &mid, const QString &name);
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
    SourceCredentials m_credentials;

    QHash<QString, qint64> m_commentCursors;
    QHash<QString, qint64> m_aidCache;
    bool m_keyRequesting = false;
    QList<Task> m_keyWaiters;
    QHash<QString, QString> m_upNames;
};

#endif
