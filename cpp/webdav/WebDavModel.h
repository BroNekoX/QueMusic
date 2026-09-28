// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 远端目录列表模型：角色与 LocalMusicScanner 对齐（列表委托可直接复用），另加 isDir
#pragma once

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QHash>
#include <QList>
#include <QString>
#include <QtQml/qqml.h>

#include "WebDavClient.h"

class WebDavModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString serverId READ serverId NOTIFY stateChanged)
    Q_PROPERTY(QString authHeader READ authHeader WRITE setAuthHeader NOTIFY stateChanged)
    Q_PROPERTY(QString dirUrl READ dirUrl NOTIFY stateChanged)
    Q_PROPERTY(QString dirName READ dirName NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY stateChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY stateChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,   // fileName
        UrlRole,                       // fileUrl
        SizeRole,                      // fileSize
        ModifiedRole,                  // fileModified
        TitleRole,
        ArtistRole,
        CoverUrlRole,
        IsDirRole
    };

    explicit WebDavModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString serverId() const { return m_serverId; }
    QString authHeader() const { return m_authHeader; }
    void setAuthHeader(const QString &authHeader);
    QString dirUrl() const { return m_dirUrl; }
    QString dirName() const { return m_dirName; }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    bool canGoUp() const;

    Q_INVOKABLE void openServer(const QString &serverId, const QString &authHeader,
                                const QString &rootUrl);
    Q_INVOKABLE void list(const QString &url);
    Q_INVOKABLE void enter(int row);
    Q_INVOKABLE void goUp();
    Q_INVOKABLE void refresh();
    // 行数据（播放/菜单用）：{title, url, isDir, lyricsUrl, coverUrl}
    Q_INVOKABLE QVariantMap at(int row) const;

signals:
    void stateChanged();
    void busyChanged();
    void errorChanged();
    void listingFailed(const QString &message);

private:
    // 目录列表缓存：短时间内重复进同一目录（含「上一级」）不再发 PROPFIND
    struct Cached {
        QList<WebDavClient::Entry> entries;
        QElapsedTimer stamp;
    };

    void setBusy(bool busy);
    void list(const QString &url, bool force);
    void remember(const QString &url, const QList<WebDavClient::Entry> &entries);
    void apply(const QList<WebDavClient::Entry> &entries, const QString &error);

    WebDavClient m_client;
    QHash<QString, Cached> m_cache;
    QList<WebDavClient::Entry> m_entries;
    QHash<QString, QPair<QString, QString>> m_sidecars;   // 音频 URL → (歌词, 封面)
    QString m_serverId;
    QString m_authHeader;
    QString m_rootUrl;
    QString m_dirUrl;
    QString m_dirName;
    QString m_error;
    bool m_busy = false;
    quint64 m_generation = 0;
};
