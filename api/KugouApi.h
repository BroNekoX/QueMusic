// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef KUGOUAPI_H
#define KUGOUAPI_H

#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <functional>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;

class KugouApi : public QObject
{
    Q_OBJECT
public:
    static constexpr int Source = 0;

    explicit KugouApi(QObject *parent = nullptr);

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

    void searchSongs(const QString &keyword, int type, int page, int pageSize);
    void getPlaylistMenu(int type);
    void getMusicPlaylists(const QString &tagid, int page, int pageSize);
    void getCategoryPlaylists(const QString &categoryid, int page, int pageSize);
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
    void get(const QString &url, const Callback &cb);
    void getLite(const QString &url, const Callback &cb);

    QString cookieValue(const QStringList &keys) const;

    static QByteArray kugouAndroidSignature(const QVariantMap &params);

    QJsonObject songInfoBaseParams() const;
    QVariantMap liteBaseParams() const;

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
    QString m_cookie;
    QString m_mid;
    QString m_dfid;
    int m_quality = 1;
    QString m_liteUserAuth;
    QHash<QString, QString> m_mixIdCache;
};

#endif
