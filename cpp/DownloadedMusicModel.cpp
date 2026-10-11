// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "DownloadedMusicModel.h"
#include "MediaTypes.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QUrl>
#include <QtConcurrent/QtConcurrentRun>

namespace {

QVariantMap readMetadataFile(const QString &jsonPath)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly))
        return {};

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject())
        return {};
    return doc.object().toVariantMap();
}

// 目录枚举 + 逐文件读同名 .json：全部在工作线程执行
QVector<DownloadedItem> scanDownloads(const QString &downloadDir)
{
    QVector<DownloadedItem> items;
    if (downloadDir.isEmpty())
        return items;

    const QFileInfoList files = QDir(downloadDir).entryInfoList(QDir::Files, QDir::Name);
    items.reserve(files.size());

    for (const QFileInfo &fi : files) {
        if (!MediaTypes::isAudioFile(fi))
            continue;

        DownloadedItem item;
        item.fileName = fi.fileName();
        item.fileUrl = QUrl::fromLocalFile(fi.absoluteFilePath()).toString();

        const QString jsonPath = fi.absolutePath() + QLatin1Char('/')
                                 + fi.completeBaseName() + QStringLiteral(".json");
        const QVariantMap meta = readMetadataFile(jsonPath);
        item.title = meta.value(QStringLiteral("title")).toString();
        item.artist = meta.value(QStringLiteral("artist")).toString();
        item.cover = meta.value(QStringLiteral("cover")).toString();
        item.duration = meta.value(QStringLiteral("duration")).toInt();
        item.hash = meta.value(QStringLiteral("hash")).toString();
        item.lyrics = meta.value(QStringLiteral("lyrics")).toList();
        item.translate = meta.value(QStringLiteral("translate")).toList();
        if (item.title.isEmpty())
            item.title = fi.completeBaseName();

        items.append(item);
    }
    return items;
}

} // namespace

DownloadedMusicModel::DownloadedMusicModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void DownloadedMusicModel::setDownloadDir(const QString &dir)
{
    if (m_downloadDir == dir)
        return;
    m_downloadDir = dir;
    emit downloadDirChanged();
}

int DownloadedMusicModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_items.size());
}

QVariant DownloadedMusicModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};

    const DownloadedItem &item = m_items.at(index.row());
    switch (role) {
    case TitleRole:     return item.title;
    case ArtistRole:    return item.artist;
    case CoverRole:     return item.cover;
    case DurationRole:  return item.duration;
    case HashRole:      return item.hash;
    case FileNameRole:  return item.fileName;
    case FileUrlRole:   return item.fileUrl;
    case LyricsRole:    return item.lyrics;
    case TranslateRole: return item.translate;
    case PaytypeRole:   return 0;   // 线上付费类型：下载页无此信息，恒免费
    default:            return {};
    }
}

QHash<int, QByteArray> DownloadedMusicModel::roleNames() const
{
    return {
        { TitleRole,      "title" },
        { ArtistRole,     "artist" },
        { CoverRole,      "cover" },
        { DurationRole,   "duration" },
        { HashRole,       "hash" },
        { FileNameRole,   "fileName" },
        { FileUrlRole,    "fileUrl" },
        { LyricsRole,     "lyrics" },
        { TranslateRole,  "translate" },
        { PaytypeRole,    "paytype" }
    };
}

QVariantMap DownloadedMusicModel::get(int row) const
{
    if (row < 0 || row >= m_items.size())
        return {};

    const DownloadedItem &item = m_items.at(row);
    QVariantMap m;
    m.insert(QStringLiteral("title"), item.title);
    m.insert(QStringLiteral("artist"), item.artist);
    m.insert(QStringLiteral("cover"), item.cover);
    m.insert(QStringLiteral("duration"), item.duration);
    m.insert(QStringLiteral("hash"), item.hash);
    m.insert(QStringLiteral("fileName"), item.fileName);
    m.insert(QStringLiteral("fileUrl"), item.fileUrl);
    m.insert(QStringLiteral("lyrics"), item.lyrics);
    m.insert(QStringLiteral("translate"), item.translate);
    m.insert(QStringLiteral("paytype"), 0);
    return m;
}

// 目录枚举与逐文件读 .json 放到线程池，避免阻塞界面
void DownloadedMusicModel::reload()
{
    const QString downloadDir = m_downloadDir;
    const quint64 generation = ++m_generation;

    auto *watcher = new QFutureWatcher<QVector<DownloadedItem>>(this);
    connect(watcher, &QFutureWatcher<QVector<DownloadedItem>>::finished, this,
            [this, watcher, generation] {
        watcher->deleteLater();
        if (generation != m_generation)
            return;
        applyItems(watcher->result());
    });
    watcher->setFuture(QtConcurrent::run([downloadDir] { return scanDownloads(downloadDir); }));
}

void DownloadedMusicModel::applyItems(const QVector<DownloadedItem> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
    emit countChanged();
}
