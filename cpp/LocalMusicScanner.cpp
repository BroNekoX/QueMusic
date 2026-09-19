// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "LocalMusicScanner.h"

#include "CoverHelper.h"
#include "SearchResultModel.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <algorithm>

namespace {
// QML 可能传本地路径或 file:// URL，统一成绝对路径再与模型比较
QString toLocalPath(const QString &raw)
{
    if (raw.isEmpty())
        return {};
    const QUrl url(raw);
    if (url.isLocalFile())
        return QDir::cleanPath(url.toLocalFile());
    return QDir::cleanPath(QFileInfo(raw).absoluteFilePath());
}
} // namespace

void LocalScanWorker::run()
{
    cancel.store(false);
    const QString path = folder;
    if (path.isEmpty()) {
        emit failed(generation.load(), QStringLiteral("empty folder"));
        return;
    }
    QDir dir(path);
    if (!dir.exists()) {
        emit failed(generation.load(), path);
        return;
    }
    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot;
    if (showDirs)
        filters |= QDir::Dirs;
    QDirIterator it(dir.absolutePath(), nameFilters, filters, QDirIterator::NoIteratorFlags);
    QList<LocalFileEntry> list;
    list.reserve(1024);
    int reported = 0;
    int count = 0;
    while (it.hasNext()) {
        if (cancel.load())
            return;
        it.next();
        const QFileInfo fi = it.fileInfo();
        LocalFileEntry e;
        e.name = fi.fileName();
        e.path = fi.absoluteFilePath();
        e.size = fi.size();
        e.modified = fi.lastModified();
        list.append(e);
        if ((++count - reported) >= 200) {
            reported = count;
            emit progress(generation.load(), count);
        }
    }
    const bool rev = sortReversed;
    switch (sortField) {
    case 1:
        std::sort(list.begin(), list.end(), [rev](const LocalFileEntry &a, const LocalFileEntry &b) { return rev ? a.name > b.name : a.name < b.name; });
        break;
    case 2:
        std::sort(list.begin(), list.end(), [rev](const LocalFileEntry &a, const LocalFileEntry &b) { return rev ? a.modified > b.modified : a.modified < b.modified; });
        break;
    case 3:
        std::sort(list.begin(), list.end(), [rev](const LocalFileEntry &a, const LocalFileEntry &b) { return rev ? a.size > b.size : a.size < b.size; });
        break;
    default:
        break;
    }
    emit finished(generation.load(), list);

    // 元数据富集：与扫描同线程，逐文件一次 TagLib 打开（title/artist/cover 共享），
    // 按 50 个一批回传，UI 侧渐进刷新行数据
    const quint64 gen = generation.load();
    const int total = list.size();
    int chunkStart = 0;
    for (int i = 0; i < total; ++i) {
        if (cancel.load())
            return;
        CoverHelper::Metadata meta;
        LocalFileEntry &e = list[i];
        e.coverUrl = CoverHelper::readCoverFromTag(e.path, cacheDir, &meta);
        e.title = meta.title;
        e.artist = meta.artist;
        if (i - chunkStart == 49 || i == total - 1) {
            emit enriched(gen, chunkStart, list.mid(chunkStart, i + 1 - chunkStart));
            chunkStart = i + 1;
            emit progress(gen, i + 1);
        }
    }
    emit enrichDone(gen, total);
}

// 不检查 cancel：删除是用户确认过的操作，中途被取消会出现「删一半」
void LocalScanWorker::deleteFiles(const QStringList &paths)
{
    const int total = paths.size();
    int processed = 0;
    QStringList removed;
    QStringList failed;
    removed.reserve(total);
    for (const QString &raw : paths) {
        const QString local = toLocalPath(raw);
        // 文件已不在磁盘上也算处理完成
        bool ok = false;
        if (!local.isEmpty()) {
            QFile file(local);
            ok = !file.exists() || file.moveToTrash();
        }
        if (ok)
            removed.append(local);
        else
            failed.append(raw);
        ++processed;
        emit deleteProgress(generation.load(), processed, total);
    }
    emit deleteFinished(generation.load(), removed, failed);
}

LocalMusicScanner::LocalMusicScanner(QObject *parent)
    : QAbstractListModel(parent)
{
    qRegisterMetaType<QList<LocalFileEntry>>("QList<LocalFileEntry>");

    m_worker = new LocalScanWorker;
    m_worker->moveToThread(&m_thread);

    m_searchResults = new SearchResultModel({SearchResultModel::FileNameRole,
                                             SearchResultModel::FileUrlRole,
                                             SearchResultModel::FileSizeRole,
                                             SearchResultModel::FileModifiedRole,
                                             SearchResultModel::TitleRole,
                                             SearchResultModel::ArtistRole,
                                             SearchResultModel::CoverUrlRole}, this);
    m_searchTimer = new QTimer(this);
    m_searchTimer->setInterval(0);
    connect(m_searchTimer, &QTimer::timeout, this, &LocalMusicScanner::searchStep);

    connect(m_worker, &LocalScanWorker::progress, this, &LocalMusicScanner::onWorkerProgress);
    connect(m_worker, &LocalScanWorker::finished, this, &LocalMusicScanner::onWorkerFinished);
    connect(m_worker, &LocalScanWorker::enriched, this, &LocalMusicScanner::onWorkerEnriched);
    connect(m_worker, &LocalScanWorker::enrichDone, this, &LocalMusicScanner::onWorkerEnrichDone);
    connect(m_worker, &LocalScanWorker::failed, this, &LocalMusicScanner::onWorkerFailed);
    connect(m_worker, &LocalScanWorker::deleteProgress, this, &LocalMusicScanner::onWorkerDeleteProgress);
    connect(m_worker, &LocalScanWorker::deleteFinished, this, &LocalMusicScanner::onWorkerDeleteFinished);

    m_thread.start();
}

LocalMusicScanner::~LocalMusicScanner()
{
    m_worker->cancel.store(true);
    m_worker->generation.fetch_add(1);
    m_thread.quit();
    m_thread.wait();
}

int LocalMusicScanner::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant LocalMusicScanner::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const auto &e = m_entries.at(index.row());
    switch (role) {
    case FileNameRole:     return e.name;
    case FileSizeRole:     return e.size;
    case FileModifiedRole: return e.modified;
    case FileUrlRole:      return QUrl::fromLocalFile(e.path);
    case TitleRole:        return e.title;
    case ArtistRole:       return e.artist;
    case CoverUrlRole:     return e.coverUrl;
    default:               return {};
    }
}

QHash<int, QByteArray> LocalMusicScanner::roleNames() const
{
    return {
        {FileNameRole,     "fileName"},
        {FileSizeRole,     "fileSize"},
        {FileModifiedRole, "fileModified"},
        {FileUrlRole,      "fileUrl"},
        {TitleRole,        "title"},
        {ArtistRole,       "artist"},
        {CoverUrlRole,     "coverUrl"}
    };
}

QVariant LocalMusicScanner::get(int index, const QString &role) const
{
    if (index < 0 || index >= m_entries.size())
        return {};
    const auto &e = m_entries.at(index);
    const QString r = role;
    if (r == QStringLiteral("fileName") || r == QStringLiteral("name"))
        return e.name;
    if (r == QStringLiteral("fileUrl") || r == QStringLiteral("url"))
        return QUrl::fromLocalFile(e.path);
    if (r == QStringLiteral("fileSize") || r == QStringLiteral("size"))
        return e.size;
    if (r == QStringLiteral("fileModified") || r == QStringLiteral("modified"))
        return e.modified;
    if (r == QStringLiteral("title"))
        return e.title;
    if (r == QStringLiteral("artist"))
        return e.artist;
    if (r == QStringLiteral("coverUrl"))
        return e.coverUrl;
    return {};
}

void LocalMusicScanner::clearEntries()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

void LocalMusicScanner::setFolder(const QUrl &folder)
{
    if (m_folder == folder)
        return;
    m_folder = folder;
    emit folderChanged();
    startScan();
}

void LocalMusicScanner::setNameFilters(const QStringList &filters)
{
    if (m_nameFilters == filters)
        return;
    m_nameFilters = filters;
    emit nameFiltersChanged();
    if (!m_folder.isEmpty())
        startScan();
}

void LocalMusicScanner::setShowDirs(bool v)
{
    if (m_showDirs == v)
        return;
    m_showDirs = v;
    emit showDirsChanged();
    if (!m_folder.isEmpty())
        startScan();
}

void LocalMusicScanner::setSortField(int v)
{
    if (m_sortField == v)
        return;
    m_sortField = v;
    emit sortFieldChanged();
    if (!m_entries.isEmpty() && !m_folder.isEmpty())
        startScan();
}

void LocalMusicScanner::setSortReversed(bool v)
{
    if (m_sortReversed == v)
        return;
    m_sortReversed = v;
    emit sortReversedChanged();
    if (!m_entries.isEmpty() && !m_folder.isEmpty())
        startScan();
}

void LocalMusicScanner::startScan()
{
    clearSearch();
    m_worker->cancel.store(true);
    const quint64 gen = m_generation.fetch_add(1) + 1;
    m_worker->generation.store(gen);

    if (m_folder.isEmpty()) {
        clearEntries();
        if (m_scanning) {
            m_scanning = false;
            emit scanningChanged();
        }
        emit scanFinished(0);
        return;
    }

    m_worker->folder = m_folder.toLocalFile();
    m_worker->nameFilters = m_nameFilters;
    m_worker->showDirs = m_showDirs;
    m_worker->sortField = m_sortField;
    m_worker->sortReversed = m_sortReversed;
    m_worker->cacheDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + QStringLiteral("/cache");
    QDir().mkpath(m_worker->cacheDir);

    if (!m_scanning) {
        m_scanning = true;
        emit scanningChanged();
    }
    QMetaObject::invokeMethod(m_worker, "run", Qt::QueuedConnection);
}

void LocalMusicScanner::deleteFiles(const QVariantList &paths)
{
    QStringList list;
    list.reserve(paths.size());
    for (const QVariant &value : paths) {
        const QString path = value.toString();
        if (!path.isEmpty())
            list.append(path);
    }
    if (list.isEmpty())
        return;

    if (!m_deleting) {
        m_deleting = true;
        emit deletingChanged();
    }
    emit deleteProgress(0, list.size()); // 立刻让进度条显示 0 / N
    QMetaObject::invokeMethod(m_worker, "deleteFiles", Qt::QueuedConnection,
                              Q_ARG(QStringList, list));
}

void LocalMusicScanner::startSearch(const QString &text)
{
    m_searchText = text.trimmed();
    m_searchResults->clearRows();
    m_searchPos = 0;
    if (m_searchText.isEmpty()) {
        if (m_searchActive) {
            m_searchActive = false;
            emit searchActiveChanged();
        }
        m_searchTimer->stop();
        emit searchFinished(0);
        return;
    }
    if (!m_searchActive) {
        m_searchActive = true;
        emit searchActiveChanged();
    }
    m_searchTimer->start();
}

void LocalMusicScanner::clearSearch()
{
    m_searchTimer->stop();
    m_searchText.clear();
    m_searchResults->clearRows();
    m_searchPos = 0;
    if (m_searchActive) {
        m_searchActive = false;
        emit searchActiveChanged();
    }
}

void LocalMusicScanner::searchStep()
{
    const int chunk = 400;
    const int total = m_entries.size();
    const QString needle = m_searchText;
    QList<SearchResultModel::Row> batch;
    for (int i = 0; i < chunk && m_searchPos < total; ++m_searchPos, ++i) {
        const LocalFileEntry &e = m_entries.at(m_searchPos);
        if (e.name.contains(needle, Qt::CaseInsensitive)
                || e.title.contains(needle, Qt::CaseInsensitive)
                || e.artist.contains(needle, Qt::CaseInsensitive)) {
            SearchResultModel::Row row;
            row.name = e.name;
            row.path = e.path;
            row.fileUrl = QUrl::fromLocalFile(e.path);
            row.size = e.size;
            row.modified = e.modified;
            row.title = e.title;
            row.artist = e.artist;
            row.coverUrl = e.coverUrl;
            batch.append(row);
        }
    }
    if (!batch.isEmpty())
        m_searchResults->appendBatch(batch);
    if (m_searchPos >= total) {
        m_searchTimer->stop();
        emit searchFinished(m_searchResults->rowCount());
    }
}

void LocalMusicScanner::onWorkerProgress(quint64 gen, int count)
{
    if (gen != m_generation.load())
        return;
    emit scanProgress(count);
}

void LocalMusicScanner::onWorkerFinished(quint64 gen, QList<LocalFileEntry> entries)
{
    if (gen != m_generation.load())
        return;
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
    if (m_scanning) {
        m_scanning = false;
        emit scanningChanged();
    }
    emit countChanged();
    emit scanFinished(m_entries.size());
}

void LocalMusicScanner::onWorkerEnriched(quint64 gen, int startIndex, QList<LocalFileEntry> chunk)
{
    if (gen != m_generation.load())
        return;
    const int last = qMin(startIndex + chunk.size(), m_entries.size()) - 1;
    if (startIndex < 0 || last < startIndex)
        return;
    for (int i = 0; i < chunk.size() && startIndex + i < m_entries.size(); ++i) {
        LocalFileEntry &dst = m_entries[startIndex + i];
        dst.title = chunk[i].title;
        dst.artist = chunk[i].artist;
        dst.coverUrl = chunk[i].coverUrl;
    }
    emit dataChanged(index(startIndex), index(last),
                     {TitleRole, ArtistRole, CoverUrlRole});
}

void LocalMusicScanner::onWorkerEnrichDone(quint64 gen, int count)
{
    if (gen != m_generation.load())
        return;
    emit metadataReady(count);
}

void LocalMusicScanner::onWorkerFailed(quint64 gen, const QString &message)
{
    if (gen != m_generation.load())
        return;
    if (m_scanning) {
        m_scanning = false;
        emit scanningChanged();
    }
    clearEntries();
    qWarning() << "LocalMusicScanner: scan failed:" << message;
}

// 不参与 generation 过期判断：用户主动发起的删除，结果必须处理
void LocalMusicScanner::onWorkerDeleteProgress(quint64 gen, int processed, int total)
{
    Q_UNUSED(gen);
    emit deleteProgress(processed, total);
}

void LocalMusicScanner::onWorkerDeleteFinished(quint64 gen, const QStringList &removed,
                                               const QStringList &failed)
{
    Q_UNUSED(gen);
    if (m_deleting) {
        m_deleting = false;
        emit deletingChanged();
    }
    removeEntriesByPath(removed);
    emit deleteFinished(removed.size(), failed.size());
}

// 只摘掉被删行，不重扫目录、不重跑 TAG
void LocalMusicScanner::removeEntriesByPath(const QStringList &paths)
{
    if (paths.isEmpty() || m_entries.isEmpty())
        return;

    QSet<QString> targets;
    for (const QString &path : paths)
        targets.insert(toLocalPath(path));

    QList<LocalFileEntry> kept;
    kept.reserve(m_entries.size());
    for (const LocalFileEntry &entry : m_entries) {
        if (!targets.contains(entry.path))
            kept.append(entry);
    }
    if (kept.size() == m_entries.size())
        return; // 未命中（列表已刷新），跳过重绘

    beginResetModel();
    m_entries = std::move(kept);
    endResetModel();
    emit countChanged();
}
