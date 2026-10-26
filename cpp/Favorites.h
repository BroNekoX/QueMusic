// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef FAVORITES_H
#define FAVORITES_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include "FolderModel.h"

#include <atomic>

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
        PaytypeRole
    };

    explicit FavoritesModel(QObject *parent = nullptr);
    ~FavoritesModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void addFavorite(const QString &id, const QString &title,
                                 const QString &artist, const QString &cover,
                                 int source, int duration, const QString &type);
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
    void setSearchText(const QString &text);
    bool loading() const { return m_loading; }
    int favoriteRevision() const { return s_revision; }

    static void reloadAll();

signals:
    void filterTypeChanged();
    void folderIdChanged();
    void searchTextChanged();
    void countChanged();
    void loadingChanged();
    void favoriteStateChanged();
    void favoriteChanged(const QString &id, const QString &type, bool favorite);
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

    static void refreshIndex(bool force = false);
    static void notifyAll();

    QVector<FavoriteItem> m_items;
    QVector<FavoriteItem> m_allItems;
    QString m_filterType;
    QString m_searchText;
    int m_folderId = -1;
    int m_generation = 0;
    bool m_loading = false;
    inline static int s_revision = 0;
    inline static QList<FavoritesModel *> s_instances;
};

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

#endif