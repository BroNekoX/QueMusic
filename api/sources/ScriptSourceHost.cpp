// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "ScriptSourceHost.h"

#include "ApiActions.h"

#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJSEngine>
#include <QJSValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>

#include <utility>

class ScriptHttp : public QObject
{
    Q_OBJECT
public:
    explicit ScriptHttp(QNetworkAccessManager *nam, QObject *parent = nullptr)
        : QObject(parent), m_nam(nam) {}

    Q_INVOKABLE QString get(const QString &url, const QVariantMap &headers = {})
    {
        return QString::fromUtf8(fetch(url, headers));
    }

    Q_INVOKABLE QVariant getJson(const QString &url, const QVariantMap &headers = {})
    {
        const QJsonDocument doc = QJsonDocument::fromJson(fetch(url, headers));
        return doc.isArray() ? doc.toVariant() : QVariant(doc.object().toVariantMap());
    }

private:
    QByteArray fetch(const QString &url, const QVariantMap &headers)
    {
        QNetworkRequest request{QUrl(url)};
        request.setTransferTimeout(12000);
        request.setRawHeader("User-Agent", "QueMusic/1.0");
        for (auto it = headers.cbegin(); it != headers.cend(); ++it)
            request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());

        QNetworkReply *reply = m_nam->get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        if (!reply->isFinished())
            loop.exec(QEventLoop::ExcludeUserInputEvents);
        const QByteArray body = reply->readAll();
        reply->deleteLater();
        return body;
    }

    QNetworkAccessManager *m_nam;
};

namespace {

// 稳定 source id：基于 meta.id（缺省为文件名）的 SHA-1，跨重启不变，
// 避免插件排序变化导致用户配置里的 songSource 指向别的插件。
int stablePluginId(const QString &key, QSet<int> &used)
{
    const QByteArray digest = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha1);
    quint32 value = 0;
    for (int i = 0; i < 4; ++i)
        value = (value << 8) | quint32(uchar(digest.at(i)));

    constexpr int kRange = 60000;
    int id = ApiSource::PluginBase + int(value % kRange);
    while (used.contains(id))
        id = ApiSource::PluginBase + ((id - ApiSource::PluginBase + 1) % kRange);
    used.insert(id);
    return id;
}

QString jsText(const QJSValue &value)
{
    return value.isUndefined() || value.isNull() ? QString() : value.toString();
}

QStringList functionCandidates(const QString &action)
{
    if (action == ApiAction::SearchSongs)
        return {"searchSongs", "search"};
    if (action == ApiAction::SearchLyrics)
        return {"searchLyrics", "search"};
    if (action == ApiAction::MusicInfo)
        return {"getMusicInfo", "musicUrl"};
    if (action == ApiAction::LyricInfo)
        return {"getLyricInfo", "lyrics"};
    if (action == ApiAction::PlaylistMenu)
        return {"getPlaylistMenu", "playlistMenu"};
    if (action == ApiAction::MusicPlaylists || action == ApiAction::CategoryPlaylists)
        return {"getMusicPlaylists", "playlists"};
    if (action == ApiAction::PlaylistSongs)
        return {"getPlaylistSongs", "playlistSongs"};
    if (action == ApiAction::Comments)
        return {"getComments", "comments"};
    if (action == ApiAction::RecommendSongs || action == ApiAction::NewSongs)
        return {"getNewSongs", "newSongs"};
    if (action == ApiAction::AllToplist)
        return {"getAllToplist", "toplists"};
    if (action == ApiAction::MusicToplist)
        return {"getMusicToplist", "toplistSongs"};
    if (action == ApiAction::HotPlaylistMenu)
        return {"getHotPlaylistMenu", "hotPlaylistMenu"};
    if (action == ApiAction::HotPlaylists)
        return {"getHotPlaylists", "hotPlaylists"};
    if (action == ApiAction::SingerSongs)
        return {"getSingerSongs", "singerSongs"};
    return {action};
}

QVariantMap shapeMusicInfo(QVariantMap detail, const QVariantMap &args)
{
    QString url = detail.value("url").toString();
    if (url.isEmpty())
        url = detail.value("backup_url").toString();

    detail.insert("url", url);
    if (detail.value("backup_url").toString().isEmpty())
        detail.insert("backup_url", url);
    detail.insert("hash", args.value(ApiArg::Hash));
    detail.insert("type", args.value(ApiArg::Type));
    if (url.isEmpty())
        detail.insert("url_unavailable", true);
    return detail;
}

QVariant shapeResult(const QString &action, const QVariant &raw, int source,
                     const QVariantMap &args)
{
    if (action == ApiAction::MusicInfo) {
        QVariantMap detail;
        if (raw.typeId() == QMetaType::QString)
            detail.insert("url", raw.toString());
        else
            detail = raw.toMap();
        return shapeMusicInfo(detail, args);
    }

    if (action == ApiAction::LyricInfo) {
        QVariantMap payload;
        if (raw.typeId() == QMetaType::QVariantMap || raw.typeId() == QMetaType::QVariantHash) {
            payload = raw.toMap();
        } else if (raw.typeId() == QMetaType::QVariantList) {
            payload.insert("info", raw.toList());
        }
        if (!payload.contains("info"))
            payload.insert("info", QVariantList());
        if (!payload.contains("translate"))
            payload.insert("translate", QVariantList());
        payload.insert("hash", args.value(ApiArg::Hash));
        return payload;
    }

    if (ApiAction::isListAction(action)) {
        QVariantMap payload;
        if (raw.typeId() == QMetaType::QVariantList) {
            payload.insert("info", raw.toList());
        } else {
            const QVariantMap map = raw.toMap();
            if (map.contains("error")) {
                // 插件用返回值表示失败：保留 error，不要把它当成一条数据。
                payload = map;
                if (!payload.contains("info"))
                    payload.insert("info", QVariantList());
            } else if (map.contains("info")) {
                payload = map;
            } else if (map.isEmpty()) {
                payload.insert("info", QVariantList());
            } else {
                payload.insert("info", QVariantList{map});
            }
        }

        // 与旧行为一致：插件条目自带 source，便于收藏 / 混合列表判断来源。
        QVariantList items = payload.value("info").toList();
        for (QVariant &item : items) {
            QVariantMap map = item.toMap();
            map.insert("source", source);
            item = map;
        }
        payload.insert("info", items);

        if (action == ApiAction::Comments
            && !payload.contains("hasMore")) {
            payload.insert("hasMore", false);
        }
        return payload;
    }

    return raw;
}

} // namespace

ScriptSourceWorker::~ScriptSourceWorker()
{
    for (Entry *entry : std::as_const(m_entries))
        delete entry->js;
    qDeleteAll(m_entries);
}

QNetworkAccessManager *ScriptSourceWorker::network()
{
    if (!m_nam)
        m_nam = new QNetworkAccessManager(this);
    return m_nam;
}

ScriptSourceWorker::Entry *ScriptSourceWorker::find(int id) const
{
    for (Entry *entry : m_entries) {
        if (entry->id == id)
            return entry;
    }
    return nullptr;
}

void ScriptSourceWorker::reload(const QString &dir)
{
    for (Entry *entry : std::as_const(m_entries))
        delete entry->js;
    qDeleteAll(m_entries);
    m_entries.clear();

    QVariantList list;
    QSet<int> usedIds;
    const QDir root(dir);
    const QStringList files = root.entryList({"*.js"}, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        QFile script(root.filePath(file));
        if (!script.open(QIODevice::ReadOnly))
            continue;

        auto *js = new QJSEngine;
        js->installExtensions(QJSEngine::ConsoleExtension);
        auto *http = new ScriptHttp(network(), js);
        js->globalObject().setProperty("http", js->newQObject(http));

        const QJSValue evaluated = js->evaluate(QString::fromUtf8(script.readAll()),
                                                root.filePath(file));
        if (evaluated.isError()) {
            qWarning() << "[sources] 插件加载失败:" << file << evaluated.toString();
            delete js;
            continue;
        }

        const QJSValue meta = js->globalObject().property("meta");
        QString name = jsText(meta.property("name"));
        if (name.isEmpty())
            name = QFileInfo(file).completeBaseName();

        QString identity = jsText(meta.property("id"));
        if (identity.isEmpty())
            identity = QFileInfo(file).completeBaseName();

        auto *entry = new Entry;
        entry->js = js;
        entry->file = file;
        entry->id = stablePluginId(identity, usedIds);
        entry->name = name;

        QStringList capabilities;
        const QJSValue caps = meta.property("capabilities");
        const int capCount = caps.property("length").toInt();
        for (int i = 0; i < capCount; ++i) {
            const QString capability = jsText(caps.property(i));
            if (!capability.isEmpty())
                capabilities << capability;
        }

        const QJSValue versionValue = meta.property("version");
        const QVariant version = versionValue.isUndefined() || versionValue.isNull()
                                     ? QVariant()
                                     : versionValue.toVariant();

        m_entries << entry;
        list << QVariantMap{
            {"id", entry->id},
            {"name", entry->name},
            {"file", file},
            {"plugin", true},
            {"version", version},
            {"author", jsText(meta.property("author"))},
            {"description",
             jsText(meta.property("description"))},
            {"capabilities", capabilities},
        };
    }
    emit plugins(list);
}

void ScriptSourceWorker::call(int source, const QString &action, const QVariantMap &args)
{
    Entry *entry = find(source);
    if (!entry) {
        emit result(action, QVariantMap{
            {"error", -1},
            {"message", "plugin not found"}
        }, source);
        return;
    }

    QJSValue callable;
    QString usedFunction;
    const QStringList candidates = functionCandidates(action);
    for (const QString &name : candidates) {
        const QJSValue candidate = entry->js->globalObject().property(name);
        if (candidate.isCallable()) {
            callable = candidate;
            usedFunction = name;
            break;
        }
    }

    if (!callable.isCallable()) {
        qWarning() << "[sources] 插件缺少函数:" << entry->file << action
                   << candidates.join(QLatin1Char('/'));
        emit result(action, QVariantMap{
            {"error", -3},
            {"message", QStringLiteral("缺少函数: ") + action}
        }, source);
        return;
    }

    const QJSValue returned = callable.call({entry->js->toScriptValue(args)});
    if (returned.isError()) {
        qWarning() << "[sources] 插件执行出错:" << entry->file << usedFunction
                   << returned.toString();
        emit result(action, QVariantMap{
            {"error", -2},
            {"message", returned.toString()}
        }, source);
        return;
    }

    emit result(action, shapeResult(action, returned.toVariant(), source, args), source);
}

ScriptSourceHost::ScriptSourceHost(QObject *parent)
    : MusicSource(ApiSource::PluginBase, parent)
{
    m_worker = new ScriptSourceWorker;
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ScriptSourceWorker::plugins, this, [this](const QVariantList &list) {
        m_plugins = list;
        emit pluginsChanged();
    });
    connect(m_worker, &ScriptSourceWorker::result, this,
            [this](const QString &action, const QVariant &data, int source) {
                deliver(action, data, source);
            });
    m_thread.start();
}

ScriptSourceHost::~ScriptSourceHost()
{
    m_thread.quit();
    if (m_thread.wait(2000))
        return;
    qWarning() << "[sources] 插件线程未退出，强制终止";
    m_thread.terminate();
    m_thread.wait(1000);
}

void ScriptSourceHost::setPluginDir(const QString &dir)
{
    m_dir = dir;
}

void ScriptSourceHost::reload()
{
    QMetaObject::invokeMethod(m_worker, "reload", Qt::QueuedConnection, Q_ARG(QString, m_dir));
}

void ScriptSourceHost::request(int sourceId, const QString &action, const QVariantMap &args)
{
    QMetaObject::invokeMethod(m_worker, "call", Qt::QueuedConnection,
                              Q_ARG(int, sourceId), Q_ARG(QString, action),
                              Q_ARG(QVariantMap, args));
}

#include "ScriptSourceHost.moc"
