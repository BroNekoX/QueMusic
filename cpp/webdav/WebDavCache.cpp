// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "WebDavCache.h"
#include "DbService.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <algorithm>

namespace {

constexpr qint64 kCacheLimitBytes = 1024LL * 1024 * 1024;   // 上限 1GB
constexpr qint64 kProtectMs = 5 * 60 * 1000;                // 最近 5 分钟用过的不清理

bool isAudioSuffix(const QString &suffix)
{
    static const QStringList list = { QStringLiteral("mp3"),  QStringLiteral("flac"),
                                      QStringLiteral("m4a"),  QStringLiteral("aac"),
                                      QStringLiteral("wav"),  QStringLiteral("ogg"),
                                      QStringLiteral("opus"), QStringLiteral("wma"),
                                      QStringLiteral("ape"),  QStringLiteral("aiff"),
                                      QStringLiteral("dsf"),  QStringLiteral("dff"),
                                      QStringLiteral("mp4"),  QStringLiteral("m4b") };
    return list.contains(suffix);
}

} // namespace

WebDavCache::WebDavCache(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

QString WebDavCache::dirFor(const QString &url) const
{
    const QString key = QString::fromLatin1(
        QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha1).toHex());
    return DbService::cacheDir() + QStringLiteral("/webdav/") + key;
}

QString WebDavCache::localPathFor(const QString &url) const
{
    const QDir dir(dirFor(url));
    if (!dir.exists())
        return {};
    for (const QFileInfo &file : dir.entryInfoList(QDir::Files, QDir::Time)) {
        if (isAudioSuffix(file.suffix().toLower()))
            return file.absoluteFilePath();
    }
    return {};
}

void WebDavCache::cache(const QString &url, const QString &authHeader, const QString &lyricsUrl,
                        const QString &coverUrl)
{
    if (url.isEmpty())
        return;
    const QString existing = localPathFor(url);
    if (!existing.isEmpty()) {
        emit cached(url, existing);
        return;
    }
    for (const Task &queued : m_queue) {
        if (queued.url == url)
            return;                          // 已在排队或进行中
    }

    Task task;
    task.url = url;
    task.authHeader = authHeader;
    task.dir = dirFor(url);
    task.audioName = QUrl(url).fileName();
    if (task.audioName.isEmpty())
        task.audioName = QStringLiteral("audio");
    task.lyricsUrl = lyricsUrl;
    task.coverUrl = coverUrl;
    m_queue.append(task);
    startNext();
}

void WebDavCache::startNext()
{
    if (m_busy || m_queue.isEmpty())
        return;
    m_busy = true;
    Task &task = m_queue.first();
    QDir().mkpath(task.dir);
    downloadNext(&task);
}

void WebDavCache::downloadNext(Task *task)
{
    while (task->step < 3) {
        QString source;
        QString target;
        if (task->step == 0) {
            source = task->url;
            target = task->audioName;
        } else if (task->step == 1) {
            if (task->lyricsUrl.isEmpty()) {
                task->step = 2;
                continue;
            }
            source = task->lyricsUrl;
            target = QFileInfo(task->audioName).completeBaseName() + QStringLiteral(".lrc");
        } else {
            if (task->coverUrl.isEmpty()) {
                task->step = 3;
                continue;
            }
            source = task->coverUrl;
            QString suffix = QFileInfo(QUrl(task->coverUrl).fileName()).suffix().toLower();
            if (suffix.isEmpty())
                suffix = QStringLiteral("jpg");
            target = QStringLiteral("cover.") + suffix;
        }

        const QString path = task->dir + QLatin1Char('/') + target;
        if (QFileInfo::exists(path)) {
            ++task->step;
            continue;
        }

        QNetworkRequest request{ QUrl(source) };
        request.setTransferTimeout(20000);
        if (!task->authHeader.isEmpty())
            request.setRawHeader("Authorization", task->authHeader.toUtf8());
        QNetworkReply *reply = m_nam->get(request);
        const QString url = task->url;
        connect(reply, &QNetworkReply::finished, this, [this, reply, url, path] {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                QFile file(path + QStringLiteral(".part"));
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(reply->readAll());
                    file.close();
                    QFile::remove(path);
                    QFile::rename(path + QStringLiteral(".part"), path);
                }
            }
            if (m_queue.isEmpty() || m_queue.first().url != url) {
                m_busy = false;
                startNext();
                return;
            }
            ++m_queue.first().step;
            downloadNext(&m_queue.first());
        });
        return;
    }

    finish(task->url, task->dir + QLatin1Char('/') + task->audioName);
}

void WebDavCache::finish(const QString &url, const QString &localPath)
{
    m_queue.removeFirst();
    m_busy = false;
    if (QFileInfo::exists(localPath)) {
        emit cached(url, localPath);
        // prune 要遍历整个缓存目录，限频到每分钟一次
        if (!m_pruneStamp.isValid() || m_pruneStamp.elapsed() > 60000) {
            m_pruneStamp.restart();
            prune();
        }
    }
    startNext();
}

void WebDavCache::prune()
{
    const QDir root(DbService::cacheDir() + QStringLiteral("/webdav"));
    struct Entry {
        qint64 latest;
        qint64 size;
        QString path;
    };
    QList<Entry> entries;
    qint64 total = 0;
    for (const QFileInfo &dir : root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        Entry entry{ dir.lastModified().toMSecsSinceEpoch(), 0, dir.absoluteFilePath() };
        for (const QFileInfo &file : QDir(entry.path).entryInfoList(QDir::Files)) {
            entry.size += file.size();
            entry.latest = qMax(entry.latest, file.lastModified().toMSecsSinceEpoch());
        }
        total += entry.size;
        entries.append(entry);
    }
    if (total <= kCacheLimitBytes)
        return;

    std::sort(entries.begin(), entries.end(),
              [](const Entry &a, const Entry &b) { return a.latest < b.latest; });
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (const Entry &entry : entries) {
        if (total <= kCacheLimitBytes)
            break;
        if (now - entry.latest < kProtectMs)   // 正在听的别删
            continue;
        QDir(entry.path).removeRecursively();
        total -= entry.size;
    }
}

void WebDavCache::clear()
{
    QDir(DbService::cacheDir() + QStringLiteral("/webdav")).removeRecursively();
}
