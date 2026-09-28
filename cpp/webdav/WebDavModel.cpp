// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "WebDavModel.h"

namespace {

// 只列目录与常见音频，其余（图片/歌词/文档）一期不上列表
bool isAudio(const QString &name)
{
    static const QStringList extensions = {
        QStringLiteral("mp3"),  QStringLiteral("flac"), QStringLiteral("m4a"),
        QStringLiteral("aac"),  QStringLiteral("wav"),  QStringLiteral("ogg"),
        QStringLiteral("opus"), QStringLiteral("wma"),  QStringLiteral("ape"),
        QStringLiteral("aiff"), QStringLiteral("dsf"),  QStringLiteral("dff"),
        QStringLiteral("mp4"),  QStringLiteral("m4b")
    };
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    return dot > 0 && extensions.contains(name.mid(dot + 1).toLower());
}

QString stripTrailingSlashes(QString path)
{
    while (path.endsWith(QLatin1Char('/')) && path.size() > 1)
        path.chop(1);
    return path;
}

} // namespace

WebDavModel::WebDavModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int WebDavModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_entries.size());
}

QVariant WebDavModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const WebDavClient::Entry &entry = m_entries.at(index.row());
    switch (role) {
    case NameRole:
        return entry.name;
    case UrlRole:
        return QUrl(entry.url);
    case SizeRole:
        return entry.size;
    case ModifiedRole:
        return entry.modified;
    case TitleRole: {
        const int dot = entry.name.lastIndexOf(QLatin1Char('.'));
        return dot > 0 ? entry.name.left(dot) : entry.name;
    }
    case ArtistRole:
        return QString();
    case CoverUrlRole:
        return QString();
    case IsDirRole:
        return entry.isDir;
    default:
        return {};
    }
}

QHash<int, QByteArray> WebDavModel::roleNames() const
{
    return {
        { NameRole, "fileName" },
        { UrlRole, "fileUrl" },
        { SizeRole, "fileSize" },
        { ModifiedRole, "fileModified" },
        { TitleRole, "title" },
        { ArtistRole, "artist" },
        { CoverUrlRole, "coverUrl" },
        { IsDirRole, "isDir" }
    };
}

void WebDavModel::setAuthHeader(const QString &authHeader)
{
    if (m_authHeader == authHeader)
        return;
    m_authHeader = authHeader;
    m_cache.clear();          // 换了鉴权 ⇒ 旧列表可能不再可见
    emit stateChanged();
}

bool WebDavModel::canGoUp() const
{
    if (m_dirUrl.isEmpty() || m_rootUrl.isEmpty())
        return false;
    return stripTrailingSlashes(QUrl(m_dirUrl).path()) != stripTrailingSlashes(QUrl(m_rootUrl).path());
}

QVariantMap WebDavModel::at(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    const WebDavClient::Entry &entry = m_entries.at(row);
    const QPair<QString, QString> sidecars = m_sidecars.value(entry.url);
    QVariantMap out;
    out.insert(QStringLiteral("title"), data(index(row), TitleRole));
    out.insert(QStringLiteral("url"), entry.url);
    out.insert(QStringLiteral("isDir"), entry.isDir);
    out.insert(QStringLiteral("lyricsUrl"), sidecars.first);
    out.insert(QStringLiteral("coverUrl"), sidecars.second);
    return out;
}

void WebDavModel::openServer(const QString &serverId, const QString &authHeader,
                             const QString &rootUrl)
{
    m_serverId = serverId;
    m_rootUrl = rootUrl;
    m_cache.clear();
    setAuthHeader(authHeader);
    list(rootUrl, true);
    emit stateChanged();
}

void WebDavModel::list(const QString &url)
{
    list(url, false);
}

void WebDavModel::list(const QString &url, bool force)
{
    if (url.isEmpty())
        return;
    if (!force && url == m_dirUrl && m_busy)
        return;                                   // 同一目录正在请求：忽略重复点击
    if (!force) {
        const auto cached = m_cache.constFind(url);
        if (cached != m_cache.constEnd() && cached->stamp.elapsed() < 30000) {
            ++m_generation;                       // 作废在途请求
            m_dirUrl = url;
            m_error.clear();
            apply(cached->entries, QString());
            return;
        }
    }

    const quint64 generation = ++m_generation;
    m_dirUrl = url;
    m_error.clear();
    setBusy(true);
    emit stateChanged();
    m_client.listDir(QUrl(url), m_authHeader,
                     [this, generation, url](const QList<WebDavClient::Entry> &entries,
                                             const QString &error) {
                         if (generation != m_generation)   // 期间已切目录，丢弃
                             return;
                         if (error.isEmpty())
                             remember(url, entries);
                         apply(entries, error);
                     });
}

void WebDavModel::remember(const QString &url, const QList<WebDavClient::Entry> &entries)
{
    Cached &cached = m_cache[url];
    cached.entries = entries;
    cached.stamp.restart();
    while (m_cache.size() > 12) {                 // 只留最近用到的 12 个目录
        auto oldest = m_cache.begin();
        for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
            if (it->stamp.elapsed() > oldest->stamp.elapsed())
                oldest = it;
        }
        m_cache.erase(oldest);
    }
}

void WebDavModel::enter(int row)
{
    if (row < 0 || row >= m_entries.size() || !m_entries.at(row).isDir)
        return;
    list(m_entries.at(row).url);
}

void WebDavModel::goUp()
{
    if (!canGoUp())
        return;
    const QString path = stripTrailingSlashes(QUrl(m_dirUrl).path());
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    QUrl parent(m_dirUrl);
    parent.setPath(path.left(slash + 1));
    list(parent.toString());
}

void WebDavModel::refresh()
{
    list(m_dirUrl, true);
}

void WebDavModel::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void WebDavModel::apply(const QList<WebDavClient::Entry> &entries, const QString &error)
{
    if (!error.isEmpty()) {
        m_error = error;
        emit errorChanged();
        emit listingFailed(error);
        setBusy(false);
        return;
    }

    // 同目录同名歌词/封面：记下来，播放时随歌一起缓存
    QHash<QString, QString> byName;
    for (const WebDavClient::Entry &entry : entries) {
        if (!entry.isDir)
            byName.insert(entry.name.toLower(), entry.url);
    }
    auto pick = [&byName](const QString &base, const QStringList &extensions) -> QString {
        for (const QString &ext : extensions) {
            const auto it = byName.constFind(base + QLatin1Char('.') + ext);
            if (it != byName.constEnd())
                return it.value();
        }
        return {};
    };
    static const QStringList kCoverExtensions = { QStringLiteral("jpg"), QStringLiteral("jpeg"),
                                                  QStringLiteral("png"), QStringLiteral("webp"),
                                                  QStringLiteral("bmp"), QStringLiteral("gif") };

    beginResetModel();
    m_entries.clear();
    m_sidecars.clear();
    for (const WebDavClient::Entry &entry : entries) {
        if (!entry.isDir && !isAudio(entry.name))
            continue;
        m_entries.append(entry);
        if (entry.isDir)
            continue;
        const int dot = entry.name.lastIndexOf(QLatin1Char('.'));
        const QString base = (dot > 0 ? entry.name.left(dot) : entry.name).toLower();
        QString cover = pick(base, kCoverExtensions);
        for (const QString &name : { QStringLiteral("cover"), QStringLiteral("folder"),
                                     QStringLiteral("albumart") }) {
            if (!cover.isEmpty())
                break;
            cover = pick(name, kCoverExtensions);
        }
        m_sidecars.insert(entry.url,
                          { pick(base, { QStringLiteral("lrc"), QStringLiteral("txt") }), cover });
    }
    endResetModel();

    m_dirName = QUrl::fromPercentEncoding(
        stripTrailingSlashes(QUrl(m_dirUrl).path()).section(QLatin1Char('/'), -1).toUtf8());
    setBusy(false);
    emit stateChanged();
}
