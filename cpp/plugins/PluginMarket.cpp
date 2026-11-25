// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/PluginMarket.h"

#include "AppSettings.h"
#include "plugins/PluginStore.h"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QSet>

#include <algorithm>
#include <utility>

namespace {

const QString kDefaultServer = QStringLiteral("http://8.133.18.144/");
constexpr qint64 kMaxArchiveBytes = 64LL * 1024 * 1024;

QString indexUrlFromBase(QString base)
{
    base = base.trimmed();
    if (base.isEmpty())
        return QString();
    if (base.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
        return base;
    while (base.endsWith(QLatin1Char('/')))
        base.chop(1);
    return base + QStringLiteral("/market/index.json");
}

QString configuredIndexUrl()
{
    const QString override = qEnvironmentVariable("QUE_MUSIC_MARKET_URL");
    if (!override.isEmpty())
        return override;
    const QString base = AppSettings::value(QStringLiteral("Options"), QStringLiteral("pluginServer"),
                                            kDefaultServer).toString();
    return indexUrlFromBase(base);
}

QUrl requestUrl(const QString &raw)
{
    const QUrl url(raw);
    if (url.isValid() && !url.scheme().isEmpty())
        return url;
    return QUrl::fromLocalFile(QFileInfo(raw).absoluteFilePath());
}

QString normalizedType(QString type)
{
    type = type.toLower();
    if (type == QStringLiteral("music") || type == QStringLiteral("sources"))
        return QStringLiteral("source");
    if (type != QStringLiteral("lyrics") && type != QStringLiteral("source"))
        return QStringLiteral("function");
    return type;
}

} // namespace

PluginMarket::PluginMarket(QObject *parent)
    : QObject(parent)
    , m_sourceUrl(configuredIndexUrl())
{
    connect(PluginStore::instance(), &PluginStore::changed, this, &PluginMarket::syncInstalled);
}

PluginMarket *PluginMarket::create(QQmlEngine *, QJSEngine *)
{
    static PluginMarket *const market = new PluginMarket();
    return market;
}

QString PluginMarket::defaultSourceUrl() const
{
    return indexUrlFromBase(kDefaultServer);
}

void PluginMarket::reloadConfig()
{
    const QString url = configuredIndexUrl();
    if (!url.isEmpty())
        m_sourceUrl = url;
}

void PluginMarket::setBusy(bool busy, const QString &id)
{
    if (m_busy == busy && m_installingId == id)
        return;
    m_busy = busy;
    m_installingId = busy ? id : QString();
    emit busyChanged();
}

void PluginMarket::setStatus(const QString &text)
{
    if (m_status == text)
        return;
    m_status = text;
    emit statusChanged();
}

void PluginMarket::setError(const QString &text)
{
    if (m_error == text)
        return;
    m_error = text;
    emit errorChanged();
}

void PluginMarket::setCategory(int category)
{
    category = qBound(0, category, 4);
    if (m_category == category)
        return;
    m_category = category;
    rebuild();
    emit changed();
}

void PluginMarket::abortReply()
{
    if (!m_reply)
        return;
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->disconnect(this);
    reply->abort();
    reply->deleteLater();
}

void PluginMarket::markInstalled(QVariantMap &item) const
{
    const QString id = item.value(QStringLiteral("id")).toString();
    const PluginStore *store = PluginStore::instance();
    const bool installed = store->installed(id);
    const QString local = store->versionOf(id);
    item.insert(QStringLiteral("installed"), installed);
    item.insert(QStringLiteral("localVersion"), local);
    item.insert(QStringLiteral("updatable"),
                installed && !local.isEmpty()
                    && local != item.value(QStringLiteral("version")).toString());
}

void PluginMarket::rebuild()
{
    QVariantList source;
    switch (m_category) {
    case 0:
        source = m_featured;
        break;
    case 1:
        source = m_all;
        std::sort(source.begin(), source.end(), [](const QVariant &a, const QVariant &b) {
            return a.toMap().value(QStringLiteral("updated")).toString()
                   > b.toMap().value(QStringLiteral("updated")).toString();
        });
        break;
    case 2:
    case 3:
    case 4: {
        const QString type = m_category == 2 ? QStringLiteral("lyrics")
                            : m_category == 3 ? QStringLiteral("function")
                                              : QStringLiteral("source");
        for (const QVariant &v : std::as_const(m_all)) {
            if (v.toMap().value(QStringLiteral("type")).toString() == type)
                source.append(v);
        }
        break;
    }
    default:
        source = m_all;
    }

    m_list.clear();
    m_list.reserve(source.size());
    for (const QVariant &v : std::as_const(source)) {
        QVariantMap item = v.toMap();
        markInstalled(item);
        m_list.append(item);
    }
}

void PluginMarket::syncInstalled()
{
    if (m_all.isEmpty())
        return;
    rebuild();
    emit changed();
}

void PluginMarket::refresh()
{
    if (m_busy)
        return;
    abortReply();

    const QUrl url = requestUrl(m_sourceUrl);
    if (url.isEmpty() || !url.isValid()) {
        setError(QStringLiteral("插件市场地址无效"));
        setStatus(QString());
        return;
    }

    setError(QString());
    setBusy(true);
    setStatus(QStringLiteral("获取插件列表…"));

    QNetworkRequest request{url};
    request.setTransferTimeout(8000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_net.get(request);
    m_reply = reply;

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        reply->deleteLater();
        setBusy(false);
        if (reply->error() != QNetworkReply::NoError) {
            setError(QStringLiteral("无法获取插件列表：%1").arg(reply->errorString()));
            setStatus(QString());
            return;
        }
        const QByteArray data = reply->readAll();
        if (data.isEmpty()) {
            setError(QStringLiteral("插件列表为空"));
            setStatus(QString());
            return;
        }
        parseIndex(data);
    });
}

void PluginMarket::parseIndex(const QByteArray &data)
{
    QJsonParseError error{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        setError(QStringLiteral("插件列表格式错误：%1").arg(error.errorString()));
        setStatus(QString());
        return;
    }

    const QJsonArray array = doc.object().value(QStringLiteral("plugins")).toArray();
    const QUrl base = requestUrl(m_sourceUrl);
    QVariantList parsed;
    parsed.reserve(array.size());
    QSet<QString> seen;
    for (const QJsonValue &value : array) {
        const QJsonObject obj = value.toObject();
        const QString id = obj.value(QStringLiteral("id")).toString();
        if (id.isEmpty() || seen.contains(id))
            continue;
        seen.insert(id);

        QVariantMap item;
        item.insert(QStringLiteral("id"), id);
        item.insert(QStringLiteral("name"),
                    obj.value(QStringLiteral("name")).toString(id));
        item.insert(QStringLiteral("type"),
                    normalizedType(obj.value(QStringLiteral("type")).toString()));
        item.insert(QStringLiteral("author"), obj.value(QStringLiteral("author")).toString());
        item.insert(QStringLiteral("version"), obj.value(QStringLiteral("version")).toString());
        item.insert(QStringLiteral("description"), obj.value(QStringLiteral("description")).toString());
        item.insert(QStringLiteral("updated"), obj.value(QStringLiteral("updated")).toString());

        const QString preview = obj.value(QStringLiteral("preview")).toString();
        const QString download = obj.value(QStringLiteral("download")).toString();
        item.insert(QStringLiteral("preview"), preview.isEmpty() ? QString() : base.resolved(QUrl(preview)).toString());
        item.insert(QStringLiteral("download"), download.isEmpty() ? QString() : base.resolved(QUrl(download)).toString());
        item.insert(QStringLiteral("sha256"), obj.value(QStringLiteral("sha256")).toString());
        item.insert(QStringLiteral("size"), qint64(obj.value(QStringLiteral("size")).toDouble()));
        item.insert(QStringLiteral("builtin"), false);
        parsed.append(item);
    }

    std::sort(parsed.begin(), parsed.end(), [](const QVariant &a, const QVariant &b) {
        return a.toMap().value(QStringLiteral("name")).toString()
               < b.toMap().value(QStringLiteral("name")).toString();
    });

    m_all = parsed;
    m_featured = parsed;
    for (int i = m_featured.size() - 1; i > 0; --i)
        m_featured.swapItemsAt(i, QRandomGenerator::global()->bounded(i + 1));

    rebuild();
    setStatus(QStringLiteral("共 %1 个插件").arg(m_all.size()));
    emit changed();
}

void PluginMarket::install(const QString &id)
{
    if (m_busy || id.isEmpty())
        return;

    QVariantMap target;
    for (const QVariant &v : std::as_const(m_all)) {
        QVariantMap item = v.toMap();
        if (item.value(QStringLiteral("id")).toString() == id) {
            target = item;
            break;
        }
    }
    if (target.isEmpty()) {
        const QString text = QStringLiteral("插件不在列表里，请刷新后重试");
        setError(text);
        emit failed(id, text);
        return;
    }
    if (target.value(QStringLiteral("installed")).toBool()
        && !target.value(QStringLiteral("updatable")).toBool()) {
        emit installed(id);
        return;
    }

    const QUrl url(target.value(QStringLiteral("download")).toString());
    if (url.isEmpty() || !url.isValid()) {
        const QString text = QStringLiteral("插件包地址无效");
        setError(text);
        emit failed(id, text);
        return;
    }

    abortReply();
    setError(QString());
    setBusy(true, id);
    setStatus(QStringLiteral("下载中 0%"));

    QNetworkRequest request{url};
    request.setTransferTimeout(120000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("QueMusic-Market"));

    QNetworkReply *reply = m_net.get(request);
    m_reply = reply;

    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 done, qint64 total) {
        if (reply != m_reply)
            return;
        if (total > kMaxArchiveBytes || done > kMaxArchiveBytes) {
            reply->abort();
            return;
        }
        if (total > 0)
            setStatus(QStringLiteral("下载中 %1%").arg(int(done * 100 / total)));
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, id, target]() {
        if (reply != m_reply)
            return;
        m_reply = nullptr;
        reply->deleteLater();

        const auto fail = [this, id](const QString &text) {
            setBusy(false);
            setStatus(QString());
            setError(text);
            emit failed(id, text);
        };

        if (reply->error() != QNetworkReply::NoError) {
            fail(reply->error() == QNetworkReply::OperationCanceledError
                     ? QStringLiteral("插件包超过 64 MB，已取消下载")
                     : QStringLiteral("下载失败：%1").arg(reply->errorString()));
            return;
        }

        const QByteArray archive = reply->readAll();
        if (archive.isEmpty()) {
            fail(QStringLiteral("插件包为空"));
            return;
        }
        if (archive.size() > kMaxArchiveBytes) {
            fail(QStringLiteral("插件包超过 64 MB"));
            return;
        }

        const QString expectHash = target.value(QStringLiteral("sha256")).toString();
        if (!expectHash.isEmpty()) {
            const QString actual = QString::fromLatin1(
                QCryptographicHash::hash(archive, QCryptographicHash::Sha256).toHex());
            if (actual.compare(expectHash, Qt::CaseInsensitive) != 0) {
                fail(QStringLiteral("校验失败，插件包可能已损坏"));
                return;
            }
        }

        setStatus(QStringLiteral("安装中…"));
        const QString installError = PluginStore::instance()->installArchive(archive, id);
        if (!installError.isEmpty()) {
            fail(installError);
            return;
        }

        setBusy(false);
        setStatus(QStringLiteral("已安装"));
        rebuild();
        emit changed();
        emit installed(id);
    });
}
