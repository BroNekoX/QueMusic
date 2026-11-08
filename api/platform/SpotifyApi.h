// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#pragma once

#include <QJsonObject>
#include <QList>
#include <QNetworkAccessManager>
#include <QUrlQuery>

#include <functional>

#include "MusicSource.h"

class SpotifyApi : public MusicSource
{
    Q_OBJECT
public:
    static constexpr int Source = ApiSource::Spotify;

    explicit SpotifyApi(QObject *parent = nullptr);
    ~SpotifyApi() override = default;

    QString sourceName() const override { return "Spotify"; }
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
    using TokenCallback = std::function<void(bool)>;

    void ensureToken(const TokenCallback &done);
    void get(const QString &path, const QUrlQuery &query, const JsonCallback &done);
    void playlistTracks(const QString &id, int page, int pageSize, const QString &action);
    void recommendations(int page, int pageSize, const QString &action);
    void search(const QString &keyword, int type, int page, int pageSize, const QString &action);

    static QVariantMap track(const QJsonObject &object);
    static QVariantMap playlist(const QJsonObject &object);
    static QVariantMap album(const QJsonObject &object);
    static QVariantMap artist(const QJsonObject &object);
    static QString imageOf(const QJsonObject &object);

    QNetworkAccessManager *m_nam = nullptr;
    QString m_cookie;
    QString m_accessToken;
    QString m_token;
    qint64 m_expiry = 0;
    bool m_tokenPending = false;
    QList<TokenCallback> m_waiters;
};
