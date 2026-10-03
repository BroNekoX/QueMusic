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
    Q_PROPERTY(QString filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

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
        PaytypeRole        // 收藏未存付费类型：只给角色名 → QML 取 0
    };

    explicit FavoritesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE void addFavorite(const QString &id, const QString &title,
                                 const QString &artist, const QString &cover,
                                 int source, int duration, const QString &type);
    Q_INVOKABLE void removeFavorite(const QString &id, const QString &type);
    Q_INVOKABLE bool isFavorite(const QString &id, const QString &type) const;

    QString filterType() const { return m_filterType; }
    void setFilterType(const QString &type);
    bool loading() const { return m_loading; }

signals:
    void filterTypeChanged();
    void countChanged();
    void loadingChanged();
    void errorOccurred(const QString &message);

private:
    void refreshModel();
    void applyItems(const QVector<FavoriteItem> &items);
    void setLoading(bool loading);

    QVector<FavoriteItem> m_items;
    // 「是否已收藏」的内存答案，随 refresh 同步维护
    QHash<QString, QSet<QString>> m_idsByType;
    QString m_filterType;
    std::atomic<int> m_generation{0};
    bool m_loading = false;
};

#endif // FAVORITES_H
