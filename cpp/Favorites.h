// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 收藏模型：SQL 走 DbService，isFavorite 用内存 id 集合回答（delegate 每行都会调用）。
// 收藏夹复用 folders 表（type='fav'），favorites.folder_id 记录歌曲所属夹；歌单/歌手收藏不参与。
#ifndef FAVORITES_H
#define FAVORITES_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include "FolderModel.h"

#include <atomic>

// 收藏项结构体
struct FavoriteItem {
    QString id;
    QString title;
    QString artist;
    QString cover;
    int source = 0;
    int duration = 0;
    QString type;
    QDateTime createdAt;
};

class FavoritesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged FINAL)
    Q_PROPERTY(int folderId READ folderId WRITE setFolderId NOTIFY folderIdChanged FINAL)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged FINAL)
    // 收藏表的全局版本号：任一实例增删收藏都会 +1，各列表的收藏图标绑它即可同步刷新
    Q_PROPERTY(int favoriteRevision READ favoriteRevision NOTIFY favoriteStateChanged FINAL)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        TitleRole,
        ArtistRole,
        CoverRole,
        SourceRole,
        DurationRole,
        TypeRole,
        CreatedAtRole,
        PaytypeRole        // 收藏不存付费类型：data() 恒返回 0
    };

    explicit FavoritesModel(QObject *parent = nullptr);
    ~FavoritesModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    // 落库失败会通过 errorOccurred 报出来；调用方不要抢先提示成功
    Q_INVOKABLE void addFavorite(const QString &id, const QString &title,
                                 const QString &artist, const QString &cover,
                                 int source, int duration, const QString &type);
    // 收藏到指定收藏夹（歌曲收藏夹选择弹窗用）
    Q_INVOKABLE void addToFolder(const QString &id, const QString &title,
                                 const QString &artist, const QString &cover,
                                 int source, int duration, int folderId);
    Q_INVOKABLE void removeFavorite(const QString &id, const QString &type);
    Q_INVOKABLE bool isFavorite(const QString &id, const QString &type) const;
    Q_INVOKABLE void refresh() { refreshModel(); }

    QString filterType() const { return m_filterType; }
    void setFilterType(const QString &type);
    int folderId() const { return m_folderId; }
    void setFolderId(int folderId);
    QString searchText() const { return m_searchText; }
    void setSearchText(const QString &text);     // 内存内过滤，不查库
    bool loading() const { return m_loading; }
    int favoriteRevision() const { return s_revision; }

    // 收藏夹被删后让所有收藏列表重新拉取
    static void reloadAll();

signals:
    void filterTypeChanged();
    void folderIdChanged();
    void searchTextChanged();
    void countChanged();
    void loadingChanged();
    // 某个 id 的收藏状态变了：委托里的收藏图标据此重算
    void favoriteStateChanged();
    void errorOccurred(const QString &message);

private:
    void refreshModel();
    void applyItems(const QVector<FavoriteItem> &items);
    void applyFilter();
    void setLoading(bool loading);
    void bumpRevision();
    void writeFavorite(const QString &id, const QString &title, const QString &artist,
                       const QString &cover, int source, int duration,
                       const QString &type, int folderId, bool moveExisting);

    QVector<FavoriteItem> m_items;      // 过滤后的当前视图
    QVector<FavoriteItem> m_allItems;   // 当前夹/类型的全量，供内存过滤
    // 「是否已收藏」的内存答案：refreshModel 在 DB 线程一次性填好全部类型，
    // isFavorite 只读它，不做任何 SQL
    QHash<QString, QSet<QString>> m_idsByType;
    QString m_filterType;
    QString m_searchText;
    int m_folderId = -1;          // >=0 时只看该收藏夹（仅歌曲）
    std::atomic<int> m_generation{0};
    bool m_loading = false;
    inline static int s_revision = 0;              // 只在 GUI 线程递增
    inline static QList<FavoritesModel *> s_instances;   // 三个单例，用于互相通知
};

// 收藏夹列表（folders.type = 'fav'）：新增/重命名/删除；删除时连带清掉夹内歌曲收藏。
class FavoriteFoldersModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(int defaultFolderId READ defaultFolderId NOTIFY foldersChanged FINAL)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        IsDefaultRole
    };

    explicit FavoriteFoldersModel(QObject *parent);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addFolder(const QString &name);
    Q_INVOKABLE void renameFolder(int folderId, const QString &newName);
    Q_INVOKABLE void deleteFolder(int folderId);

    int defaultFolderId() const { return m_defaultId; }

signals:
    void countChanged();
    void foldersChanged();
    void errorOccurred(const QString &message);

private:
    void reload();
    void applyRows(quint64 generation, const QVector<FolderItem> &rows);
    void mutate(DbMutator mutator, bool reloadFavorites = false);

    QVector<FolderItem> m_items;
    int m_defaultId = -1;
    std::atomic<quint64> m_generation{0};
};

#endif // FAVORITES_H
