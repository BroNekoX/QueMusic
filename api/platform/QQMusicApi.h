// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef QQMUSICAPI_H
#define QQMUSICAPI_H

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <functional>

#include "MusicSource.h"

class QNetworkAccessManager;

class QQMusicApi : public MusicSource
{
    Q_OBJECT
public:
    static constexpr int Source = ApiSource::QQMusic;

    explicit QQMusicApi(QObject *parent = nullptr);
    ~QQMusicApi() override = default;

    QString sourceName() const override { return QStringLiteral("QQ音乐"); }
    void applyCredentials(const SourceCredentials &credentials) override;

protected:
    void onSearchSongs(const QString &keyword, int type, int page, int pageSize) override;
    void onSearchLyrics(const QString &keyword, int page, int pageSize) override;
    void onPlaylistMenu(int type) override;
    void onMusicPlaylists(const QString &tagid, int page, int pageSize) override;
    void onCategoryPlaylists(const QString &categoryid, int page, int pageSize) override;
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
    using JsonCallback = std::function<void(const QJsonObject &)>;

    struct SongDetail
    {
        QString name;
        QString artist;
        QString albumMid;
        QString mediaMid;
        qint64 id = 0;
        int interval = 0;
        bool paid = false;
    };

    void getJson(const QUrl &url, const JsonCallback &callback);
    void postJson(const QUrl &url, const QJsonObject &body, const JsonCallback &callback);

    void requestSearch(const QString &keyword, int type, int page, int pageSize,
                       const QString &action);
    void requestPlaylists(const QString &categoryId, int page, int pageSize,
                          const QString &action);
    void requestDetail(const QString &mid, const std::function<void(const SongDetail &)> &done);
    void requestTopSongs(int topId, int page, int pageSize, const QString &action);
    void requestSingerList(int area, int page, int pageSize, const QString &action);
    void requestPlayUrl(const QString &mid, int type, const SongDetail &detail);
    void emitList(const QString &action, const QVariantList &info, const QJsonObject &json);

    QVariantMap parseSong(const QJsonObject &song, const QString &fallbackArtist = QString());
    QVariantMap parsePlaylist(const QJsonObject &playlist) const;
    QVariantMap parseAlbum(const QJsonObject &album) const;
    QString singerNames(const QJsonValue &value) const;
    SongDetail detailFromJson(const QJsonObject &song) const;
    void rememberSong(const QJsonObject &song);
    void rememberDetail(const QString &mid, const SongDetail &detail);
    static QString makeSearchId();
    static QVariantList parseLrc(const QString &text);
    static QVariantList alignedTranslations(const QVariantList &lyrics, const QString &text);

    QNetworkAccessManager *m_nam = nullptr;
    int m_quality = 1;
    QHash<QString, SongDetail> m_details;
    QHash<QString, QVariantList> m_playlistCache;
    QVariantList m_menuCache;
};

#endif // QQMUSICAPI_H
