// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
#include "ScriptSourceHost.h"

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

// 插件可用全局 http.get / http.getJson（阻塞，运行在插件自己的线程）
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
    const QDir root(dir);
    const QStringList files = root.entryList({QStringLiteral("*.js")}, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        QFile script(root.filePath(file));
        if (!script.open(QIODevice::ReadOnly))
            continue;

        auto *js = new QJSEngine;
        js->installExtensions(QJSEngine::ConsoleExtension);
        auto *http = new ScriptHttp(network(), js);
        js->globalObject().setProperty(QStringLiteral("http"), js->newQObject(http));

        const QJSValue evaluated = js->evaluate(QString::fromUtf8(script.readAll()),
                                                root.filePath(file));
        if (evaluated.isError()) {
            qWarning() << "[sources] 插件加载失败:" << file << evaluated.toString();
            delete js;
            continue;
        }

        const QJSValue meta = js->globalObject().property(QStringLiteral("meta"));
        auto *entry = new Entry;
        entry->js = js;
        entry->file = file;
        entry->id = ScriptSourceHost::kPluginSourceBase + m_entries.size();
        entry->name = meta.property(QStringLiteral("name")).toString();
        if (entry->name.isEmpty())
            entry->name = QFileInfo(file).completeBaseName();

        QStringList capabilities;
        const QJSValue caps = meta.property(QStringLiteral("capabilities"));
        const int capCount = caps.property(QStringLiteral("length")).toInt();
        for (int i = 0; i < capCount; ++i)
            capabilities << caps.property(i).toString();
        m_entries << entry;
        list << QVariantMap{
            {QStringLiteral("id"), entry->id},
            {QStringLiteral("name"), entry->name},
            {QStringLiteral("file"), file},
            {QStringLiteral("plugin"), true},
            {QStringLiteral("capabilities"), capabilities},
        };
    }
    emit plugins(list);
}

void ScriptSourceWorker::call(int source, const QString &action, const QVariantMap &args)
{
    Entry *entry = find(source);
    const QVariantMap failure{{QStringLiteral("error"), -1}};
    if (!entry) {
        emit result(action, failure, source);
        return;
    }

    QString function;
    if (action == QLatin1String("searchSongs"))
        function = QStringLiteral("search");
    else if (action == QLatin1String("getMusicInfo"))
        function = QStringLiteral("musicUrl");
    else if (action == QLatin1String("getLyricInfo"))
        function = QStringLiteral("lyrics");

    const QJSValue callable = function.isEmpty()
                                  ? QJSValue()
                                  : entry->js->globalObject().property(function);
    if (!callable.isCallable()) {
        qWarning() << "[sources] 插件缺少函数:" << entry->file << function;
        emit result(action, failure, source);
        return;
    }

    const QJSValue returned = callable.call({entry->js->toScriptValue(args)});
    if (returned.isError()) {
        qWarning() << "[sources] 插件执行出错:" << entry->file << returned.toString();
        emit result(action, QVariantMap{{QStringLiteral("error"), -2}, {QStringLiteral("message"), returned.toString()}}, source);
        return;
    }

    const QVariant raw = returned.toVariant();
    QVariant payload = raw;
    if (action == QLatin1String("searchSongs")) {
        QVariantList tracks = raw.toList();
        for (QVariant &track : tracks) {
            QVariantMap map = track.toMap();
            map.insert(QStringLiteral("source"), source);
            track = map;
        }
        payload = QVariantMap{{QStringLiteral("info"), tracks}};
    } else if (action == QLatin1String("getMusicInfo")) {
        const QString url = raw.typeId() == QMetaType::QString
                                ? raw.toString()
                                : raw.toMap().value(QStringLiteral("url")).toString();
        payload = QVariantMap{
            {QStringLiteral("url"), url},
            {QStringLiteral("backup_url"), url},
            {QStringLiteral("hash"), args.value(QStringLiteral("hash"))},
            {QStringLiteral("type"), args.value(QStringLiteral("type"))},
            {QStringLiteral("url_unavailable"), url.isEmpty()},
        };
    } else if (action == QLatin1String("getLyricInfo")) {
        const QVariantMap lyrics = raw.toMap();
        payload = QVariantMap{
            {QStringLiteral("info"), lyrics.value(QStringLiteral("info"))},
            {QStringLiteral("translate"), lyrics.value(QStringLiteral("translate"))},
        };
    }
    emit result(action, payload, source);
}

ScriptSourceHost::ScriptSourceHost(QObject *parent)
    : QObject(parent)
{
    m_worker = new ScriptSourceWorker;
    m_worker->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &ScriptSourceWorker::plugins, this, [this](const QVariantList &list) {
        m_plugins = list;
        emit pluginsChanged();
    });
    connect(m_worker, &ScriptSourceWorker::result, this, &ScriptSourceHost::resultReady);
    m_thread.start();
}

ScriptSourceHost::~ScriptSourceHost()
{
    m_thread.quit();
    m_thread.wait();
}

void ScriptSourceHost::setPluginDir(const QString &dir)
{
    m_dir = dir;
}

void ScriptSourceHost::reload()
{
    QMetaObject::invokeMethod(m_worker, "reload", Qt::QueuedConnection, Q_ARG(QString, m_dir));
}

void ScriptSourceHost::request(int source, const QString &action, const QVariantMap &args)
{
    QMetaObject::invokeMethod(m_worker, "call", Qt::QueuedConnection,
                              Q_ARG(int, source), Q_ARG(QString, action), Q_ARG(QVariantMap, args));
}

bool ScriptSourceHost::isPlugin(int source) const
{
    for (const QVariant &entry : m_plugins) {
        if (entry.toMap().value(QStringLiteral("id")).toInt() == source)
            return true;
    }
    return false;
}

#include "ScriptSourceHost.moc"
