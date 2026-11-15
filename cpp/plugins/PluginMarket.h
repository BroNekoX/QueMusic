// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

class QNetworkReply;

// 插件市场：远程索引 + 5 个分类（推荐 / 最新 / 歌词界面 / 功能 / 音乐源）
class PluginMarket : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PluginMarket)
    QML_SINGLETON

    Q_PROPERTY(QVariantList list READ list NOTIFY changed FINAL)
    Q_PROPERTY(int category READ category WRITE setCategory NOTIFY changed FINAL)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged FINAL)
    Q_PROPERTY(QString defaultSourceUrl READ defaultSourceUrl CONSTANT FINAL)
    Q_PROPERTY(QString installingId READ installingId NOTIFY busyChanged FINAL)

public:
    explicit PluginMarket(QObject *parent = nullptr);
    static PluginMarket *create(QQmlEngine *, QJSEngine *);

    QVariantList list() const { return m_list; }
    int category() const { return m_category; }
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    QString error() const { return m_error; }
    QString defaultSourceUrl() const;
    QString installingId() const { return m_installingId; }

    void setCategory(int category);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void install(const QString &id);
    Q_INVOKABLE void reloadConfig();

signals:
    void changed();
    void busyChanged();
    void statusChanged();
    void errorChanged();
    void installed(const QString &id);
    void failed(const QString &id, const QString &text);

private:
    void setBusy(bool busy, const QString &id = QString());
    void setStatus(const QString &text);
    void setError(const QString &text);
    void parseIndex(const QByteArray &data);
    void rebuild();
    void markInstalled(QVariantMap &item) const;
    void syncInstalled();
    QString localVersion(const QString &id) const;

    QNetworkAccessManager m_net;
    QVariantList m_all;
    QVariantList m_featured;
    QVariantList m_list;
    QString m_sourceUrl;
    QString m_status;
    QString m_error;
    QString m_installingId;
    QNetworkReply *m_reply = nullptr;
    int m_category = 0;
    bool m_busy = false;
};