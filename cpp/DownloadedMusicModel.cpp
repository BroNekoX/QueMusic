// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "DownloadedMusicModel.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QStringList>

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
    Q_UNUSED(parent);
    return m_items.size();
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
        { TranslateRole,  "translate" }
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
    return m;
}

QVariantMap DownloadedMusicModel::readMetadata(const QString &jsonPath) const
{
    QVariantMap meta;
    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly))
        return meta;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return meta;
    return doc.object().toVariantMap();
}

void DownloadedMusicModel::reload()
{
    static const QStringList audioExts = {
        QStringLiteral("mp3"),   QStringLiteral("wav"),  QStringLiteral("aac"),
        QStringLiteral("flac"),  QStringLiteral("ogg"),  QStringLiteral("eac3"),
        QStringLiteral("wma"),   QStringLiteral("ac3"),  QStringLiteral("alac"),
        QStringLiteral("m4a"),   QStringLiteral("mkv"),  QStringLiteral("wmv"),
        QStringLiteral("avi"),   QStringLiteral("mpeg4")
    };

    beginResetModel();
    m_items.clear();

    if (!m_downloadDir.isEmpty()) {
        const QDir dir(m_downloadDir);
        const QFileInfoList files = dir.entryInfoList(QDir::Files, QDir::Name);
        for (const QFileInfo &fi : files) {
            if (!audioExts.contains(fi.suffix().toLower()))
                continue;

            DownloadedItem item;
            item.fileName = fi.fileName();
            item.fileUrl = QUrl::fromLocalFile(fi.absoluteFilePath()).toString();

            const QString jsonPath = fi.absolutePath() + QLatin1Char('/')
                                     + fi.completeBaseName() + QStringLiteral(".json");
            const QVariantMap meta = readMetadata(jsonPath);
            item.title = meta.value(QStringLiteral("title")).toString();
            item.artist = meta.value(QStringLiteral("artist")).toString();
            item.cover = meta.value(QStringLiteral("cover")).toString();
            item.duration = meta.value(QStringLiteral("duration")).toInt();
            item.hash = meta.value(QStringLiteral("hash")).toString();
            item.lyrics = meta.value(QStringLiteral("lyrics")).toList();
            item.translate = meta.value(QStringLiteral("translate")).toList();

            if (item.title.isEmpty())
                item.title = fi.completeBaseName();

            m_items.append(item);
        }
    }

    endResetModel();
    emit countChanged();
}
