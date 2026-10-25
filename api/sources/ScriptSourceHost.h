// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// JS 音源插件宿主：每个插件在独立线程的 QJSEngine 内运行，
// 约定全局 meta = { id, name, capabilities }，并实现 search(q) / musicUrl(t) / lyrics(t)。
#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

class QJSEngine;
class QNetworkAccessManager;

class ScriptSourceWorker : public QObject
{
    Q_OBJECT
public:
    ~ScriptSourceWorker() override;

public slots:
    void reload(const QString &dir);
    void call(int source, const QString &action, const QVariantMap &args);

signals:
    void plugins(const QVariantList &list);
    void result(const QString &action, const QVariant &data, int source);

private:
    struct Entry {
        int id = 0;
        QString name;
        QString file;
        QJSEngine *js = nullptr;
    };
    Entry *find(int id) const;
    QNetworkAccessManager *network();

    QList<Entry *> m_entries;
    QNetworkAccessManager *m_nam = nullptr;
};

class ScriptSourceHost : public QObject
{
    Q_OBJECT
public:
    static constexpr int kPluginSourceBase = 100;

    explicit ScriptSourceHost(QObject *parent = nullptr);
    ~ScriptSourceHost() override;

    void setPluginDir(const QString &dir);
    void reload();
    void request(int source, const QString &action, const QVariantMap &args);
    bool isPlugin(int source) const;

    QVariantList plugins() const { return m_plugins; }

signals:
    void pluginsChanged();
    void resultReady(const QString &action, const QVariant &data, int source);

private:
    QString m_dir;
    QVariantList m_plugins;
    QThread m_thread;
    ScriptSourceWorker *m_worker = nullptr;
};
