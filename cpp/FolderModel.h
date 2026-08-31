// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QVector>
#include <QDateTime>

// 文件夹项结构体
struct FolderItem {
    int id = -1;
    QString name;
    QString type;
    QString path;
    QDateTime createdAt;
};

// 分支用于存储歌曲项结构体
struct SongItem {
    int id = -1;
    int folderId = -1;
    QString name;
    QString path;
    QString singer;
    int duration = 0;
};

// 文件夹模型
class FolderModel : public QAbstractListModel
{
    Q_OBJECT
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

    // QML 可调用的方法
    Q_INVOKABLE void loadFromDatabase();
    Q_INVOKABLE int addFolder(const QString &name, const QString &type, const QString &path = "");
    Q_INVOKABLE bool deleteFolder(int folderId);
    Q_INVOKABLE bool renameFolder(int folderId, const QString &newName);

    QString filterType() const { return m_filterType; }
    void setFilterType(const QString &type);

signals:
    void filterTypeChanged();
    void errorOccurred(const QString &message);

private:
    void refreshModel();
    QSqlDatabase m_db;
    QVector<FolderItem> m_items;
    QString m_filterType = "my";
};

// 存储歌曲模型
class SongModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int folderId READ folderId WRITE setFolderId NOTIFY folderIdChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        FolderIdRole,
        NameRole,
        PathRole,
        SingerRole,
        DurationRole
    };

    explicit SongModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // QML 可调用的方法
    Q_INVOKABLE void loadByFolder(int folderId);
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE int addSong(int folderId, const QString &name, const QString &path, const QString &singer = "");
    Q_INVOKABLE int addSongs(int folderId, const QVariantList &songs);
    Q_INVOKABLE bool deleteSong(int songId);

    int folderId() const { return m_folderId; }
    void setFolderId(int folderId);

signals:
    void folderIdChanged();
    void errorOccurred(const QString &message);

private:
    void refreshModel();
    QSqlDatabase m_db;
    QVector<SongItem> m_items;
    int m_folderId = -1;
};

#endif // FOLDERMODEL_H