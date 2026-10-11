// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "DownloadManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QRegularExpression>

namespace {

// 远端标题可能带路径分隔符、Windows 非法字符或超长内容，落盘前统一清洗
QString sanitizeFileName(const QString &raw)
{
    QString name = QFileInfo(raw).fileName().trimmed();
    // Windows 上 QFileInfo 会同时处理两种分隔符，这里再兜一层，确保没有残留目录成分
    name = name.section(QLatin1Char('/'), -1).section(QLatin1Char('\\'), -1);
    static const QRegularExpression illegal(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1F]"));
    name.replace(illegal, QStringLiteral("_"));
    // Windows 不允许文件名以点或空格结尾
    while (name.endsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char(' ')))
        name.chop(1);
    if (name.isEmpty() || name == QLatin1String(".") || name == QLatin1String(".."))
        return QStringLiteral("unknown");

    constexpr int kMaxNameLength = 120;
    if (name.size() > kMaxNameLength) {
        const QFileInfo fi(name);
        const QString suffix = fi.suffix();
        const int keep = qMax(1, kMaxNameLength - (suffix.isEmpty() ? 0 : suffix.size() + 1));
        const QString base = fi.completeBaseName().left(keep);
        name = suffix.isEmpty() ? base : base + QLatin1Char('.') + suffix;
    }
    return name;
}

} // namespace

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

// 实际保存音频/元数据的目录：自定义目录或系统音乐目录下统一挂 QueMusic 子目录
QString DownloadManager::effectiveDownloadDir() const
{
    QString dir = m_downloadPath;
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    return dir + QLatin1String("/QueMusic");
}

// QAbstractListModel

int DownloadManager::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_tasks.size());
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
        { TitleRole,       "title" },
        { ArtistRole,      "artist" },
        { CoverRole,       "cover" },
        { DurationRole,    "duration" },
        { HashRole,        "hash" }
    };
}

// Public API

void DownloadManager::addDownload(const QString &url, const QString &fileName,
                                  const QVariantMap &meta)
{
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
    task.fileName = sanitizeFileName(fileName);
    task.title = meta.value(QStringLiteral("title")).toString();
    task.artist = meta.value(QStringLiteral("artist")).toString();
    task.cover = meta.value(QStringLiteral("cover")).toString();
    task.duration = meta.value(QStringLiteral("duration")).toInt();
    task.hash = meta.value(QStringLiteral("hash")).toString();
    task.lyrics = meta.value(QStringLiteral("lyrics")).toList();
    task.translate = meta.value(QStringLiteral("translate")).toList();

    int row = m_tasks.size();
    beginInsertRows(QModelIndex(), row, row);
    m_tasks.append(task);
    endInsertRows();

    emit taskCountChanged();

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

    const QModelIndex mi = index(nextIdx);

    // 准备保存路径：先定 filePath 再发 dataChanged，否则 QML 会拿到空的 filePath
    const QString downloadDir = effectiveDownloadDir();
    QDir().mkpath(downloadDir);
    const QString rootDir = QDir(downloadDir).absolutePath();

    // 如果文件已存在，添加数字后缀避免覆盖
    const QString basePath = rootDir + QLatin1Char('/') + task.fileName;
    const QFileInfo baseInfo(basePath);
    const QString baseName = baseInfo.completeBaseName();
    const QString suffix = baseInfo.suffix();
    QString savePath = basePath;
    for (int counter = 1; QFile::exists(savePath); ++counter) {
        savePath = rootDir + QLatin1Char('/') + baseName
                   + QStringLiteral("(%1)").arg(counter);
        if (!suffix.isEmpty())
            savePath += QLatin1Char('.') + suffix;
    }

    const auto failTask = [&](const QString &message) {
        task.status = DownloadTask::Error;
        task.errorString = message;
        emit dataChanged(mi, mi, { StatusRole, ErrorStringRole });
        if (m_file) {
            m_file->deleteLater();
            m_file = nullptr;
        }
        m_currentTaskId = -1;
        emit currentTaskIdChanged();
        emit hasActiveTasksChanged();
        // 继续下一个
        startNextTask();
    };

    // 文件名已在 addDownload() 清洗；这里再校验最终路径仍在下载目录内
    if (!QFileInfo(savePath).absoluteFilePath().startsWith(rootDir + QLatin1Char('/'))) {
        failTask(tr("Invalid file name: %1").arg(task.fileName));
        return;
    }
    task.filePath = savePath;

    emit dataChanged(mi, mi, { StatusRole, FilePathRole });
    emit currentTaskIdChanged();
    emit hasActiveTasksChanged();

    m_file = new QFile(savePath, this);
    if (!m_file->open(QIODevice::WriteOnly)) {
        failTask(tr("Cannot open file: %1").arg(m_file->errorString()));
        return;
    }

    // 发起网络请求
    QNetworkRequest request;
    request.setUrl(QUrl(task.url));
    // B 站 CDN 会校验 Referer / UA，缺失时返回 403
    if (task.url.contains(QLatin1String("bilivideo"), Qt::CaseInsensitive)
        || task.url.contains(QLatin1String("bilibili"), Qt::CaseInsensitive)) {
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                                           "AppleWebKit/537.36 (KHTML, like Gecko) "
                                           "Chrome/120.0.0.0 Safari/537.36");
        request.setRawHeader("Referer", "https://www.bilibili.com");
    }
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
    // abort() 会同步触发 finished：先断开回调并清空成员，
    // 否则 onFinished() 会顺手启动下一个任务，回来后这里又把新任务的 reply/file 删一遍
    QNetworkReply *reply = m_reply;
    QFile *file = m_file;
    m_reply = nullptr;
    m_file = nullptr;

    if (reply) {
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    if (file) {
        file->close(); // 不删除文件 — 保留已下载的部分
        file->deleteLater();
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

// 下载完成后，在与音频同名（同 basename）的 .json 中保存元数据与歌词
void DownloadManager::writeMetadata(const DownloadTask &task)
{
    if (task.filePath.isEmpty())
        return;

    const QFileInfo fi(task.filePath);
    const QString jsonPath = fi.absolutePath() + QLatin1Char('/')
                             + fi.completeBaseName() + QStringLiteral(".json");

    QJsonObject root;
    root.insert(QStringLiteral("title"), task.title);
    root.insert(QStringLiteral("artist"), task.artist);
    root.insert(QStringLiteral("cover"), task.cover);
    root.insert(QStringLiteral("duration"), task.duration);
    root.insert(QStringLiteral("hash"), task.hash);

    QJsonArray lyricsArr;
    for (const QVariant &v : task.lyrics) {
        const QVariantMap m = v.toMap();
        QJsonObject line;
        line.insert(QStringLiteral("time"), m.value(QStringLiteral("time")).toInt());
        line.insert(QStringLiteral("text"), m.value(QStringLiteral("text")).toString());
        lyricsArr.append(line);
    }
    root.insert(QStringLiteral("lyrics"), lyricsArr);

    QJsonArray transArr;
    for (const QVariant &v : task.translate)
        transArr.append(QJsonValue::fromVariant(v));
    root.insert(QStringLiteral("translate"), transArr);

    QFile f(jsonPath);
    if (!f.open(QIODevice::WriteOnly)) {
        qWarning() << "[download] 无法写入元数据文件:" << jsonPath;
        return;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

// Network slots

void DownloadManager::onReadyRead()
{
    // 只认当前 reply：旧 reply 的延迟回调不能写进新任务的文件
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply || reply != m_reply || !m_file)
        return;
    m_file->write(reply->readAll());
}

void DownloadManager::onFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply || reply != m_reply)
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
        if (reply->error() == QNetworkReply::NoError) {
            task.status = DownloadTask::Completed;
            task.progress = 1.0;
            writeMetadata(task);
        }
        // 错误已在 onErrorOccurred 里处理

        QModelIndex mi = index(idx);
        emit dataChanged(mi, mi, { StatusRole, ProgressRole });
        emit completedCountChanged();
    }

    m_reply = nullptr;
    reply->deleteLater();

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
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply || reply != m_reply) return;

    int taskId = m_currentTaskId;
    int idx = findTaskById(taskId);
    if (idx < 0) return;

    DownloadTask &task = m_tasks[idx];
    task.status = DownloadTask::Error;
    task.errorString = reply->errorString();

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
