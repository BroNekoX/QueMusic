// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 收藏模型：SQL 走 DbService，isFavorite 用内存 id 集合回答（delegate 每行都会调用）。
#ifndef FAVORITES_H
#define FAVORITES_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QtQml/qqmlregistration.h>

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
    Q_INVOKABLE void removeFavorite(const QString &id, const QString &type);
    Q_INVOKABLE bool isFavorite(const QString &id, const QString &type) const;

    QString filterType() const { return m_filterType; }
    void setFilterType(const QString &type);
    bool loading() const { return m_loading; }
    int favoriteRevision() const { return s_revision; }

signals:
    void filterTypeChanged();
    void countChanged();
    void loadingChanged();
    // 某个 id 的收藏状态变了：委托里的收藏图标据此重算
    void favoriteStateChanged();
    void errorOccurred(const QString &message);

private:
    void refreshModel();
    void applyItems(const QVector<FavoriteItem> &items);
    void setLoading(bool loading);
    void bumpRevision();

    QVector<FavoriteItem> m_items;
    // 「是否已收藏」的内存答案：refreshModel 在 DB 线程一次性填好全部类型，
    // isFavorite 只读它，不做任何 SQL
    QHash<QString, QSet<QString>> m_idsByType;
    QString m_filterType;
    std::atomic<int> m_generation{0};
    bool m_loading = false;
    inline static int s_revision = 0;              // 只在 GUI 线程递增
    inline static QList<FavoritesModel *> s_instances;   // 三个单例，用于互相通知
};

#endif // FAVORITES_H
