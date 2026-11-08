// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QUrl>

#include <functional>

#include "MusicSource.h"

class AppleMusicApi : public MusicSource
{
    Q_OBJECT
public:
    static constexpr int Source = ApiSource::AppleMusic;

    explicit AppleMusicApi(QObject *parent = nullptr);
    ~AppleMusicApi() override = default;

    QString sourceName() const override { return "Apple Music"; }

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

    void get(const QUrl &url, const JsonCallback &done);
    void search(const QString &term, const QString &entity, int page, int pageSize, const QString &action);
    void lookupTracks(const QString &id, int page, int pageSize, const QString &action);
    void rss(const QString &action, bool albums, int page, int pageSize);

    static QVariantMap track(const QJsonObject &object);
    static QVariantMap album(const QJsonObject &object);
    static QVariantMap rssItem(const QJsonObject &object, bool asAlbum);
    static QString artwork(QString url);
    static QString idOf(const QJsonObject &object, const char *key);

    QNetworkAccessManager *m_nam = nullptr;
};
