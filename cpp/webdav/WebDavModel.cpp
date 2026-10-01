// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "WebDavModel.h"

#include "CoverHelper.h"
#include "WebDavCache.h"   // 已落地的封面按缓存的同一规则查

#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

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

QString stemOf(const QString &name)
{
    const int dot = name.lastIndexOf(QLatin1Char('.'));
    return dot > 0 ? name.left(dot) : name;
}

// 网盘里的命名习惯是「歌名 - 歌手」（实测：`グッバイ宣言 - Chinozo.mp3`、
// `I Don't Do Drugs - Doja Cat、Ariana Grande.mp3`），与酷狗接口返回的「歌手 - 歌名」相反；
// 只在第一个 '-' 处分割，两侧都可能带多歌手/多语言。若你的库是「歌手 - 歌名」，把两个函数对调即可。
QString titleOf(const QString &stem)
{
    const int i = stem.indexOf(QLatin1Char('-'));
    return i > 0 ? stem.left(i).trimmed() : stem;
}

QString artistOf(const QString &stem)
{
    const int i = stem.indexOf(QLatin1Char('-'));
    return i > 0 ? stem.mid(i + 1).trimmed() : QString();
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
    // 文件夹名就是标题；音频按「歌手 - 歌名」拆开，好让列表和本地音乐一样有两列
    case TitleRole:
        return titleOf(stemOf(entry.name));
    case ArtistRole:
        return entry.isDir ? QString() : artistOf(stemOf(entry.name));
    case CoverUrlRole:
        // 侧车封面是远端 URL，列表加载不了它（要带鉴权头）；这里只暴露已经落地到本地的那份
        return QString();
    case LocalCoverRole:
        return m_coverCache.value(entry.url);   // 纯查表：提取在工作线程做（见 warmCovers）
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
        { IsDirRole, "isDir" },
        { LocalCoverRole, "localCover" }
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
    out.insert(QStringLiteral("artist"), data(index(row), ArtistRole));
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
    warmCovers();
}

// 已落地的文件才有封面可取（内嵌图或随播放落地的 cover.*）；提取要开 TagLib + 解码，
// 所以放工作线程，回来再刷新这些行的 localCover，列表本身不等它
void WebDavModel::warmCovers()
{
    QStringList pending;
    for (const WebDavClient::Entry &entry : std::as_const(m_entries)) {
        if (entry.isDir || m_coverCache.contains(entry.url) || m_coverPending.contains(entry.url))
            continue;
        if (WebDavCache::cachedAudioFor(entry.url).isEmpty())
            continue;
        pending.append(entry.url);
    }
    if (pending.isEmpty())
        return;
    for (const QString &url : std::as_const(pending))
        m_coverPending.insert(url);

    const quint64 generation = m_generation;
    auto *watcher = new QFutureWatcher<QHash<QString, QString>>(this);
    connect(watcher, &QFutureWatcher<QHash<QString, QString>>::finished, this,
            [this, watcher, generation]() {
        watcher->deleteLater();
        const QHash<QString, QString> found = watcher->result();
        if (generation != m_generation || found.isEmpty() || m_entries.isEmpty())
            return;   // 已经翻到别的目录了，结果作废
        for (auto it = found.constBegin(); it != found.constEnd(); ++it)
            m_coverCache.insert(it.key(), it.value());
        emit dataChanged(index(0), index(int(m_entries.size()) - 1), { LocalCoverRole });
    });
    watcher->setFuture(QtConcurrent::run([pending]() {
        QHash<QString, QString> covers;
        const QString coverDir = CoverHelper::defaultCacheDir();
        for (const QString &url : pending) {
            const QString sidecar = WebDavCache::cachedCoverFor(url);   // 同目录 cover.*
            if (!sidecar.isEmpty()) {
                covers.insert(url, QUrl::fromLocalFile(sidecar).toString());
                continue;
            }
            const QString local = WebDavCache::cachedAudioFor(url);
            const QString cover = local.isEmpty() ? QString()
                                                  : CoverHelper::readCoverFromTag(local, coverDir);
            if (!cover.isEmpty())
                covers.insert(url, cover);
        }
        return covers;
    }));
}
