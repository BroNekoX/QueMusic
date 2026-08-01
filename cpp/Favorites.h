// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
#ifndef FAVORITES_H
#define FAVORITES_H

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QVector>
#include <QDateTime>

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

// 收藏夹模型
class FavoritesModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(QString filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)
	Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
	
public:
	enum Roles {
		IdRole = Qt::UserRole + 1,
		TitleRole,
		ArtistRole,
		CoverRole,
		SourceRole,
		DurationRole,
		TypeRole,
		CreatedAtRole
	};

    Q_INVOKABLE QVariantMap get(int row) const;
	
	explicit FavoritesModel(QObject *parent = nullptr);
	
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;
	
	// QML 可调用方法
	Q_INVOKABLE void addFavorite(const QString &id, const QString &title,
								 const QString &artist, const QString &cover,
								 int source, int duration, const QString &type);
	Q_INVOKABLE bool removeFavorite(const QString &id, const QString &type);
	Q_INVOKABLE bool isFavorite(const QString &id, const QString &type) const;
	Q_INVOKABLE void clearFavorites(const QString &type = QString());
	
	QString filterType() const { return m_filterType; }
	void setFilterType(const QString &type);
	
	signals:
	void filterTypeChanged();
	void countChanged();
	void errorOccurred(const QString &message);
	
private:
	void refreshModel();
	void createTableIfNeeded();
	QSqlDatabase m_db;
	QVector<FavoriteItem> m_items;
	QString m_filterType;   // 空字符串表示显示所有类型
};

#endif // FAVORITES_H
