// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef QUEUEMODEL_H
#define QUEUEMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include <atomic>

// 已保存播放列表：只存元数据，歌曲在 playlist_songs 表。
class SavedPlaylistsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    enum Roles {
        PlaylistIdRole = Qt::UserRole + 1,
        NameRole,
        SongCountRole,
        CreatedAtRole
    };

    explicit SavedPlaylistsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();

signals:
    void countChanged();
    void loaded();

private:
    struct Item {
        int id = -1;
        QString name;
        int songCount = 0;
        QDateTime createdAt;
    };

    void applyRows(quint64 generation, const QVector<Item> &rows);

    QVector<Item> m_items;
    std::atomic<quint64> m_generation{0};
};

// 预览已保存播放列表：与当前队列同角色，ListView 可直接复用 delegate。
class PlaylistPreviewModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(QString name READ name NOTIFY loaded FINAL)
    Q_PROPERTY(int playlistId READ playlistId NOTIFY loaded FINAL)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SongerRole,
        SourceRole
    };

    explicit PlaylistPreviewModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString name() const { return m_name; }
    int playlistId() const { return m_playlistId; }

    Q_INVOKABLE void load(int playlistId);
    Q_INVOKABLE void clear();

signals:
    void countChanged();
    void loaded();

private:
    struct Song {
        QString name;
        QString path;
        QString songer;
        int source = 0;
    };

    void applySongs(quint64 generation, int playlistId, const QString &name,
                    const QVector<Song> &songs);

    QVector<Song> m_songs;
    QString m_name;
    int m_playlistId = -1;
    std::atomic<quint64> m_generation{0};
};

class QueueModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count NOTIFY countChanged FINAL)
    Q_PROPERTY(int playListIndex READ playListIndex WRITE setPlayListIndex NOTIFY playListIndexChanged FINAL)
    Q_PROPERTY(SavedPlaylistsModel *savedPlaylists READ savedPlaylists CONSTANT FINAL)
    Q_PROPERTY(PlaylistPreviewModel *playlistPreview READ playlistPreview CONSTANT FINAL)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SongerRole,
        SourceRole
    };
    Q_ENUM(Roles)

    explicit QueueModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void append(const QVariantMap &item);
    Q_INVOKABLE int appendBatch(const QVariantList &items, bool skipExisting = true);
    Q_INVOKABLE void insert(int index, const QVariantMap &item);
    Q_INVOKABLE void moveTo(int from, int to);
    Q_INVOKABLE void remove(int index, int count = 1);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int indexOfPath(const QString &path) const;

    Q_INVOKABLE void savePlaylist(const QString &name);
    Q_INVOKABLE void loadPlaylist(int id);
    Q_INVOKABLE void renamePlaylist(int id, const QString &name);
    Q_INVOKABLE void deletePlaylist(int id);
    Q_INVOKABLE void previewPlaylist(int id);

    int count() const { return m_items.size(); }
    int playListIndex() const { return m_playListIndex; }
    void setPlayListIndex(int index);

    SavedPlaylistsModel *savedPlaylists() const { return m_savedPlaylists; }
    PlaylistPreviewModel *playlistPreview() const { return m_preview; }

signals:
    void countChanged();
    void playListIndexChanged();
    void playlistSaved(int id, const QString &name, int count);
    void playlistLoaded(int id, const QString &name, int count);
    void playlistRenamed(int id, const QString &name);
    void playlistRemoved(int id);
    void playlistError(const QString &message);

private:
    struct Track
    {
        QString name;
        QString path;
        QString songer;
        int source = 0;
    };

    void rebuildIndex();
    Track toTrack(const QVariantMap &item) const;

    QVector<Track> m_items;
    QHash<QString, int> m_indexOfPath;
    int m_playListIndex = -1;
    SavedPlaylistsModel *m_savedPlaylists = nullptr;
    PlaylistPreviewModel *m_preview = nullptr;
};

#endif // QUEUEMODEL_H
