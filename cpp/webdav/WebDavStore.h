// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// WebDAV 服务器配置与凭据：配置写 QSettings，密码在 Windows 上用 DPAPI 加密后落盘
#pragma once

#include <QHash>
#include <QJSEngine>
#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include <QtQml/qqml.h>

class WebDavStore : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(WebDav)
    QML_SINGLETON
    Q_PROPERTY(QVariantList servers READ servers NOTIFY serversChanged FINAL)

public:
    explicit WebDavStore(QObject *parent = nullptr);
    static WebDavStore *create(QQmlEngine *, QJSEngine *) { return new WebDavStore(); }

    QVariantList servers() const;

    Q_INVOKABLE void addServer(const QString &name, const QString &url, const QString &user,
                               const QString &password);
    // 密码留空表示沿用原密码
    Q_INVOKABLE void updateServer(const QString &id, const QString &name, const QString &url,
                                  const QString &user, const QString &password);
    Q_INVOKABLE void removeServer(const QString &id);

    Q_INVOKABLE QString authHeader(const QString &id) const;
    // 按远端 URL 找所属服务器（播放时给引擎附鉴权头用）
    Q_INVOKABLE QString authHeaderOfUrl(const QString &url) const;
    // 远端曲目随行的歌词/封面 URL（列目录时记下，播放时随歌缓存）
    Q_INVOKABLE void rememberSidecars(const QString &audioUrl, const QString &lyricsUrl,
                                      const QString &coverUrl);
    Q_INVOKABLE QVariantMap sidecarsOf(const QString &audioUrl) const;
    Q_INVOKABLE QString userOf(const QString &id) const;

signals:
    void serversChanged();

private:
    void load();
    void save() const;
    int indexOf(const QString &id) const;
    QVariantList m_servers;
    QHash<QString, QVariantMap> m_sidecars;   // 会话内：音频 URL → {lyricsUrl, coverUrl}
    // 解密后的 Basic 头按服务器缓存（配置一变就失效）
    mutable QString m_authCacheId;
    mutable QString m_authCacheHeader;
};
