// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 歌单/歌曲模型：SQL 走 DbService，TAG 解析结果写回 songs 表。
#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QSqlDatabase>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include "SearchResultModel.h"

#include <atomic>
#include <functional>

struct FolderItem {
    int id = -1;
    QString name;
    QString type;
    QString path;
    QDateTime createdAt;
};

struct SongItem {
    int id = -1;
    int folderId = -1;
    QString name;
    QString path;
    QString singer;
    int duration = 0;
    QString tagTitle;
    QString tagArtist;
    QString tagCoverUrl;
    bool tagged = false;
};

// 在 DB 线程执行；失败时写 error
using DbMutator = std::function<void(QSqlDatabase &db, QString &error)>;

class FolderModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        PathRole,
        CreatedAtRole
    };

    explicit FolderModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadFromDatabase();
    Q_INVOKABLE void addFolder(const QString &name, const QString &type, const QString &path = QString());
    Q_INVOKABLE void deleteFolder(int folderId);
    Q_INVOKABLE void deleteFolders(const QVariantList &folderIds);
    Q_INVOKABLE void renameFolder(int folderId, const QString &newName);

    QString filterType() const { return m_filterType; }
    void setFilterType(const QString &type);

signals:
    void filterTypeChanged();
    void errorOccurred(const QString &message);

private:
    void applyRows(quint64 generation, const QVector<FolderItem> &rows);
    void mutate(DbMutator mutator);

    QVector<FolderItem> m_items;
    QString m_filterType = QStringLiteral("my");
    std::atomic<quint64> m_generation{0};
};

class SongModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int folderId READ folderId WRITE setFolderId NOTIFY folderIdChanged)
    Q_PROPERTY(bool searchActive READ searchActive NOTIFY searchActiveChanged)
    Q_PROPERTY(QAbstractListModel *searchResults READ searchResults CONSTANT)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        FolderIdRole,
        NameRole,
        PathRole,
        SingerRole,
        DurationRole,
        TagTitleRole,
        TagArtistRole,
        TagCoverUrlRole
    };

    explicit SongModel(QObject *parent = nullptr);
    ~SongModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadByFolder(int folderId);
    Q_INVOKABLE void reload() { loadByFolder(m_folderId); }
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void addSong(int folderId, const QString &name, const QString &path, const QString &singer = QString());
    Q_INVOKABLE void addSongs(int folderId, const QVariantList &songs);
    Q_INVOKABLE void deleteSong(int songId);
    Q_INVOKABLE void deleteSongs(const QVariantList &songIds);
    // 丢弃 TAG 缓存重解析；path 为空表示整个文件夹
    Q_INVOKABLE void rescanTags(const QString &path = QString());

    Q_INVOKABLE void startSearch(const QString &text);
    Q_INVOKABLE void clearSearch();
    bool searchActive() const { return m_searchActive; }
    QAbstractListModel *searchResults() const { return m_searchResults; }

    int folderId() const { return m_folderId; }
    void setFolderId(int folderId);

signals:
    void folderIdChanged();
    void errorOccurred(const QString &message);
    void searchActiveChanged();
    void searchFinished(int count);
    // 批量导入完成（替代原来的同步返回值）
    void songsAdded(int count);

private:
    void refreshModel();
    void applyRows(quint64 generation, const QVector<SongItem> &rows);
    void startEnrichment();
    void enrichBatch();
    void persistTags(const QList<int> &ids, const QStringList &titles,
                     const QStringList &artists, const QStringList &covers);
    void submitSearch();
    void mutate(DbMutator mutator);

    QVector<SongItem> m_items;
    QList<int> m_enrichPending;
    int m_folderId = -1;
    std::atomic<quint64> m_loadGeneration{0};
    std::atomic<quint64> m_searchGeneration{0};
    SearchResultModel *m_searchResults = nullptr;
    QString m_searchText;
    bool m_searchActive = false;
    bool m_enriching = false;
};

#endif // FOLDERMODEL_H
