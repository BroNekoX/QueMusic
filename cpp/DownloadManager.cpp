// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "DownloadManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

DownloadManager::DownloadManager(QObject *parent)
    : QAbstractListModel(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

DownloadManager::~DownloadManager()
{
    abortCurrentDownload();
}

void DownloadManager::setDownloadPath(const QString &path)
{
    if (m_downloadPath == path)
        return;
    m_downloadPath = path;
    emit downloadPathChanged();
}

// QAbstractListModel

int DownloadManager::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_tasks.size();
}

QVariant DownloadManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_tasks.size())
        return {};

    const DownloadTask &task = m_tasks.at(index.row());

    switch (role) {
    case IdRole:          return task.id;
    case UrlRole:         return task.url;
    case FileNameRole:    return task.fileName;
    case FilePathRole:    return task.filePath;
    case ProgressRole:    return task.progress;
    case StatusRole:      return static_cast<int>(task.status);
    case ErrorStringRole: return task.errorString;
    // QListView-compatible aliases
    case TitleRole:       return task.fileName;
    case ArtistRole:      return task.filePath;
    case CoverRole:       return QString();
    case DurationRole:    return 0;
    case HashRole:        return task.id;
    default:              return {};
    }
}

QHash<int, QByteArray> DownloadManager::roleNames() const
{
    return {
        { IdRole,          "taskId" },
        { UrlRole,         "url" },
        { FileNameRole,    "fileName" },
        { FilePathRole,    "filePath" },
        { ProgressRole,    "progress" },
        { StatusRole,      "status" },
        { ErrorStringRole, "errorString" },
        // QListView-compatible aliases
        { TitleRole,       "title" },
        { ArtistRole,      "artist" },
        { CoverRole,       "cover" },
        { DurationRole,    "duration" },
        { HashRole,        "hash" }
    };
}

// Public API

void DownloadManager::addDownload(const QString &url, const QString &fileName)
{
    // 避免重复添加完全相同的下载
    for (const auto &t : m_tasks) {
        if (t.url == url && t.fileName == fileName &&
            (t.status == DownloadTask::Queued || t.status == DownloadTask::Downloading)) {
            return;
        }
    }

    int id = nextTaskId();
    DownloadTask task;
    task.id = id;
    task.url = url;
    task.fileName = fileName;

    int row = m_tasks.size();
    beginInsertRows(QModelIndex(), row, row);
    m_tasks.append(task);
    endInsertRows();

    emit taskCountChanged();

    // 没有活跃任务则立即启动
    if (!hasActiveTasks())
        startNextTask();
}

void DownloadManager::retryTask(int taskId)
{
    int idx = findTaskById(taskId);
    if (idx < 0) return;

    DownloadTask &task = m_tasks[idx];
    if (task.status == DownloadTask::Completed)
        return;

    // 重置任务状态
    task.progress = 0.0;
    task.errorString.clear();
    task.status = DownloadTask::Queued;
    task.filePath.clear();

    QModelIndex mi = index(idx);
    emit dataChanged(mi, mi, { ProgressRole, StatusRole, ErrorStringRole, FilePathRole });

    // 如果没有活跃任务，立即启动
    if (!hasActiveTasks())
        startNextTask();
}

void DownloadManager::removeTask(int taskId)
{
    int idx = findTaskById(taskId);
    if (idx < 0) return;

    // 如果正在下载该任务，先中止
    if (taskId == m_currentTaskId)
        abortCurrentDownload();

    beginRemoveRows(QModelIndex(), idx, idx);
    m_tasks.removeAt(idx);
    endRemoveRows();

    emit taskCountChanged();
    emit completedCountChanged();

    // 如果取消的是当前任务，启动下一个
    if (taskId == m_currentTaskId) {
        m_currentTaskId = -1;
        emit currentTaskIdChanged();
        emit hasActiveTasksChanged();
        startNextTask();
    }
}

void DownloadManager::clearCompleted()
{
    // 找出所有已完成/错误的任务索引
    QList<int> toRemove;
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].status == DownloadTask::Completed ||
            m_tasks[i].status == DownloadTask::Error) {
            toRemove.prepend(i);
        }
    }

    for (int idx : toRemove) {
        beginRemoveRows(QModelIndex(), idx, idx);
        m_tasks.removeAt(idx);
        endRemoveRows();
    }

    if (!toRemove.isEmpty()) {
        emit taskCountChanged();
        emit completedCountChanged();
    }
}

void DownloadManager::cancelCurrent()
{
    if (m_currentTaskId >= 0) {
        int idx = findTaskById(m_currentTaskId);
        if (idx >= 0) {
            // 直接移除任务
            removeTask(m_currentTaskId);
            return;
        }
    }
}

// Internal

void DownloadManager::startNextTask()
{
    // 找到第一个排队中的任务
    int nextIdx = -1;
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].status == DownloadTask::Queued) {
            nextIdx = i;
            break;
        }
    }

    if (nextIdx < 0)
        return;

    DownloadTask &task = m_tasks[nextIdx];
    task.status = DownloadTask::Downloading;
    m_currentTaskId = task.id;

    QModelIndex mi = index(nextIdx);
    emit dataChanged(mi, mi, { StatusRole });
    emit currentTaskIdChanged();
    emit hasActiveTasksChanged();

    // 准备保存路径
    QString downloadDir = m_downloadPath;
    if (downloadDir.isEmpty())
        downloadDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    QDir().mkpath(downloadDir);

    // 如果文件已存在，添加数字后缀避免覆盖
    QString basePath = downloadDir + "/" + task.fileName;
    QString savePath = basePath;
    int counter = 1;
    while (QFile::exists(savePath)) {
        QFileInfo fi(basePath);
        savePath = fi.completeBaseName() + QString("(%1)").arg(counter);
        if (!fi.suffix().isEmpty())
            savePath += "." + fi.suffix();
        savePath = downloadDir + "/" + savePath;
        counter++;
    }
    task.filePath = savePath;

    m_file = new QFile(savePath, this);
    if (!m_file->open(QIODevice::WriteOnly)) {
        task.status = DownloadTask::Error;
        task.errorString = tr("Cannot open file: %1").arg(m_file->errorString());
        emit dataChanged(mi, mi, { StatusRole, ErrorStringRole });

        m_file->deleteLater();
        m_file = nullptr;
        m_currentTaskId = -1;
        emit currentTaskIdChanged();
        emit hasActiveTasksChanged();

        // 继续下一个
        startNextTask();
        return;
    }

    // 发起网络请求
    QNetworkRequest request;
    request.setUrl(QUrl(task.url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_manager->get(request);

    connect(m_reply, &QNetworkReply::readyRead, this, &DownloadManager::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &DownloadManager::onFinished);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadManager::onDownloadProgress);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &DownloadManager::onErrorOccurred);
}

void DownloadManager::abortCurrentDownload()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_file) {
        m_file->close();
        // 不删除文件 — 保留已下载的部分
        m_file->deleteLater();
        m_file = nullptr;
    }
}

int DownloadManager::findTaskById(int id) const
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id == id)
            return i;
    }
    return -1;
}

int DownloadManager::nextTaskId()
{
    return m_nextId++;
}

int DownloadManager::completedCount() const
{
    int count = 0;
    for (const auto &t : m_tasks)
        if (t.status == DownloadTask::Completed)
            count++;
    return count;
}

// Network slots

void DownloadManager::onReadyRead()
{
    if (m_file && m_reply)
        m_file->write(m_reply->readAll());
}

void DownloadManager::onFinished()
{
    if (!m_reply)
        return;

    int taskId = m_currentTaskId;
    int idx = findTaskById(taskId);

    // 刷新文件
    if (m_file) {
        m_file->flush();
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;
    }

    if (idx >= 0) {
        DownloadTask &task = m_tasks[idx];
        if (m_reply->error() == QNetworkReply::NoError) {
            task.status = DownloadTask::Completed;
            task.progress = 1.0;
        }
        // 错误已在 onErrorOccurred 里处理

        QModelIndex mi = index(idx);
        emit dataChanged(mi, mi, { StatusRole, ProgressRole });
        emit completedCountChanged();
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    m_currentTaskId = -1;
    emit currentTaskIdChanged();
    emit hasActiveTasksChanged();

    // 启动下一个任务
    startNextTask();
}

void DownloadManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    int idx = findTaskById(m_currentTaskId);
    if (idx < 0) return;

    qreal progress = (bytesTotal > 0)
        ? static_cast<qreal>(bytesReceived) / bytesTotal
        : 0.0;

    DownloadTask &task = m_tasks[idx];
    if (qFuzzyCompare(task.progress, progress))
        return;

    task.progress = progress;
    QModelIndex mi = index(idx);
    emit dataChanged(mi, mi, { ProgressRole });
}

void DownloadManager::onErrorOccurred(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code)
    if (!m_reply) return;

    int taskId = m_currentTaskId;
    int idx = findTaskById(taskId);
    if (idx < 0) return;

    DownloadTask &task = m_tasks[idx];
    task.status = DownloadTask::Error;
    task.errorString = m_reply->errorString();

    QModelIndex mi = index(idx);
    emit dataChanged(mi, mi, { StatusRole, ErrorStringRole });
    emit completedCountChanged();

    // 清理文件
    if (m_file) {
        m_file->close();
        m_file->remove();
        m_file->deleteLater();
        m_file = nullptr;
    }

    // m_currentTaskId 会在 onFinished 中被清除
}
