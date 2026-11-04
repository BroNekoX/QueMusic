// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 脚本音源宿主：每个 .js 文件是一个音源，跑在独立线程的 QJSEngine 里。
//
// 插件契约（全部可选，缺哪个就只缺那项能力）：
//   meta = { id, name, version, author, description, capabilities: [] }
//   search(q) / searchLyrics(q)      -> 歌曲数组
//   musicUrl(t) / getMusicInfo(t)    -> 播放地址字符串，或 { url, ... }
//   lyrics(t) / getLyricInfo(t)      -> { info: [{time,text,info?}], translate: [] }
//   其它 action 同名函数（getPlaylistMenu / getHotSingers ...）如存在就调用。
//
// 宿主返回的所有结果都会经过与内置音源相同的归一化和结果路由。

#ifndef SCRIPTSOURCEHOST_H
#define SCRIPTSOURCEHOST_H

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include "MusicSource.h"

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
    struct Entry
    {
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

class ScriptSourceHost : public MusicSource
{
    Q_OBJECT
public:
    static constexpr int kPluginSourceBase = ApiSource::PluginBase;

    explicit ScriptSourceHost(QObject *parent = nullptr);
    ~ScriptSourceHost() override;

    QString sourceName() const override { return QStringLiteral("脚本音源"); }
    bool isPlugin() const override { return true; }

    void setPluginDir(const QString &dir);
    void reload();
    void request(int sourceId, const QString &action, const QVariantMap &args) override;

    QVariantList plugins() const { return m_plugins; }

signals:
    void pluginsChanged();

private:
    QString m_dir;
    QVariantList m_plugins;
    QThread m_thread;
    ScriptSourceWorker *m_worker = nullptr;
};

#endif // SCRIPTSOURCEHOST_H
