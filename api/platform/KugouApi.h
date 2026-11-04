// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef KUGOUAPI_H
#define KUGOUAPI_H

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <functional>

#include "MusicSource.h"

class QNetworkAccessManager;
class QNetworkReply;

class KugouApi : public MusicSource
{
    Q_OBJECT
public:
    static constexpr int Source = ApiSource::Kugou;

    explicit KugouApi(QObject *parent = nullptr);
    ~KugouApi() override = default;

    QString sourceName() const override { return QStringLiteral("酷狗音乐"); }

    void applyCredentials(const SourceCredentials &credentials) override;

    void setCookie(const QString &cookie)
    {
        if (m_cookie != cookie)
            m_liteUserAuth.clear();
        m_cookie = cookie;
    }

    void setDeviceInfo(const QString &mid, const QString &dfid)
    {
        if (!mid.isEmpty())
            m_mid = mid;
        if (!dfid.isEmpty())
            m_dfid = dfid;
    }

    void setQuality(int quality) { m_quality = qBound(0, quality, 3); }

    static QByteArray kugouWebSignature(const QJsonObject &params);
    static QString randomGuid();
    static QString kugouMidFromGuid(const QString &guid);

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
    using Callback = std::function<void(const QJsonObject &)>;
    void get(const QString &url, const Callback &cb);
    void getLite(const QString &url, const Callback &cb);

    QString cookieValue(const QStringList &keys) const;

    static QByteArray kugouAndroidSignature(const QVariantMap &params);

    QJsonObject songInfoBaseParams() const;
    QVariantMap liteBaseParams() const;

    // 列表接口的公共解析 / 请求（合并了原先各写一遍的重复实现）。
    void performSearch(const QString &keyword, int type, int page, int pageSize,
                       const QString &resultAction);
    void requestRankSongs(int rankid, int page, int pageSize, const QString &action);
    void requestNewSongs(int type, int page, int pageSize, const QString &action,
                         bool joinAuthors);
    void requestSingerList(const QUrl &url, const QString &action);

    struct PlayRequest {
        QVariantMap meta;
        QString metaUrl;
        QString metaBackupUrl;
        bool metaDone = false;
        QString trackerUrl;
        QString trackerExt;
        int trackerRate = 0;
        int trackerStatus = 0;
        bool trackerDone = false;

        QString albumId;
        QString albumAudioId;
    };

    void requestPlayMeta(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);
    void requestTrackerUrl(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);

    void finishPlayInfo(const QString &hash, int type, const QSharedPointer<PlayRequest> &st);
    void finishWithFallback(const QString &hash, int type,
                            const QSharedPointer<PlayRequest> &st, const QString &reason,
                            int transportError = 0);

    void requestLitePlayInfo(const QString &hash, int type,
                             const QSharedPointer<PlayRequest> &st, const QString &reason);

    void requestLiteSongAuth(const QString &hash, int type,
                             const QSharedPointer<PlayRequest> &st, const QString &userAuth,
                             const QString &reason);
    void requestLiteUrlStep(const QString &hash, int type,
                            const QSharedPointer<PlayRequest> &st, const QString &auth,
                            const QString &openTime, int quality, const QString &reason);

    void requestSignedPlayInfo(const QString &hash, int type,
                               const QSharedPointer<PlayRequest> &st, const QString &reason);
    void requestSongInfo(const QJsonObject &params, const QString &hash, int type,
                         const QSharedPointer<PlayRequest> &st, const QString &reason,
                         const QJsonObject &metaFallback = QJsonObject());

    static QString decodeKrc(const QByteArray &base64);
    static QVariantList krcToLyrics(const QString &krc);
    static QVariantList krcTranslations(const QString &krc);

    QNetworkAccessManager *m_nam = nullptr;
    SourceCredentials m_credentials;
    QString m_cookie;
    QString m_mid;
    QString m_dfid;
    int m_quality = 1;
    QString m_liteUserAuth;
    QHash<QString, QString> m_mixIdCache;
};

#endif
