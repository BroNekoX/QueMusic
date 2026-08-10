// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "Favorites.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QMutexLocker>

// 使用共享数据库连接
static QSqlDatabase& sharedDatabase()
{
	static QSqlDatabase db = []() -> QSqlDatabase {
		// 此处复用原有数据库路径和连接名，无需重复创建
		QSqlDatabase db = QSqlDatabase::database("shared_player_db");
		if (!db.isValid()) {
			qWarning() << "Shared database not available!";
			return db;
		}
		return db;
	}();
	return db;
}

// FavoritesModel 实现
FavoritesModel::FavoritesModel(QObject *parent)
: QAbstractListModel(parent)
{
	m_db = sharedDatabase();
	if (!m_db.isOpen()) {
		qWarning() << "Database not open!";
	}
	createTableIfNeeded();
	refreshModel();
}

int FavoritesModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) return 0;
	return m_items.size();
}

QVariant FavoritesModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= m_items.size())
		return {};
	
	const auto &item = m_items.at(index.row());
	switch (role) {
		case IdRole:        return item.id;
		case TitleRole:     return item.title;
		case ArtistRole:    return item.artist;
		case CoverRole:     return item.cover;
		case SourceRole:    return item.source;
		case DurationRole:  return item.duration;
		case TypeRole:      return item.type;
		case CreatedAtRole: return item.createdAt;
		default:            return {};
	}
}

QHash<int, QByteArray> FavoritesModel::roleNames() const
{
    return {
        {IdRole,        "favId"},
        {TitleRole,     "title"},
        {ArtistRole,    "artist"},
        {CoverRole,     "cover"},
        {SourceRole,    "source"},
        {DurationRole,  "duration"},
        {TypeRole,      "type"},
        {CreatedAtRole, "createdAt"}
    };
}

void FavoritesModel::createTableIfNeeded()
{
	QSqlQuery query(m_db);
	query.exec("PRAGMA foreign_keys = ON");
	query.exec(
			   "CREATE TABLE IF NOT EXISTS favorites ("
			   "id TEXT NOT NULL,"
			   "title TEXT NOT NULL,"
			   "artist TEXT,"
			   "cover TEXT,"
			   "source INTEGER NOT NULL,"
			   "duration INTEGER DEFAULT 0,"
			   "type TEXT NOT NULL,"
			   "created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
               "PRIMARY KEY (type, id))"
			   );
	if (query.lastError().isValid()) {
		qWarning() << "Create favorites table failed:" << query.lastError().text();
	}
}

void FavoritesModel::addFavorite(const QString &id, const QString &title,
								 const QString &artist, const QString &cover,
								 int source, int duration, const QString &type)
{
	// 先检查是否已存在
    if (isFavorite(id, type)) {
		QSqlQuery query(m_db);
		query.prepare("UPDATE favorites SET title=:title, artist=:artist, cover=:cover, "
					  "source=:source, duration=:duration WHERE type=:type AND id=:id");
		query.bindValue(":title", title);
		query.bindValue(":artist", artist);
		query.bindValue(":cover", cover);
		query.bindValue(":source", source);
		query.bindValue(":duration", duration);
		query.bindValue(":type", type);
		query.bindValue(":id", id);
		if (!query.exec()) {
			emit errorOccurred("更新收藏失败: " + query.lastError().text());
		}
	} else {
		QSqlQuery query(m_db);
		query.prepare("INSERT INTO favorites (id, title, artist, cover, source, duration, type) "
					  "VALUES (:id, :title, :artist, :cover, :source, :duration, :type)");
		query.bindValue(":id", id);
		query.bindValue(":title", title);
		query.bindValue(":artist", artist);
		query.bindValue(":cover", cover);
		query.bindValue(":source", source);
		query.bindValue(":duration", duration);
		query.bindValue(":type", type);
		if (!query.exec()) {
			emit errorOccurred("添加收藏失败: " + query.lastError().text());
			return;
		}
	}
	refreshModel();
	emit countChanged();
}

bool FavoritesModel::removeFavorite(const QString &id, const QString &type)
{
	QSqlQuery query(m_db);
	query.prepare("DELETE FROM favorites WHERE type=:type AND id=:id");
	query.bindValue(":type", type);
	query.bindValue(":id", id);
	if (!query.exec()) {
		emit errorOccurred("删除收藏失败: " + query.lastError().text());
		return false;
	}
	refreshModel();
	emit countChanged();
	return true;
}

bool FavoritesModel::isFavorite(const QString &id, const QString &type) const
{
	QSqlQuery query(m_db);
	query.prepare("SELECT COUNT(*) FROM favorites WHERE type=:type AND id=:id");
	query.bindValue(":type", type);
	query.bindValue(":id", id);
	if (query.exec() && query.next()) {
		return query.value(0).toInt() > 0;
	}
	return false;
}

void FavoritesModel::clearFavorites(const QString &type)
{
	QSqlQuery query(m_db);
	if (type.isEmpty()) {
		query.exec("DELETE FROM favorites");
	} else {
		query.prepare("DELETE FROM favorites WHERE type=:type");
		query.bindValue(":type", type);
		query.exec();
	}
	if (query.lastError().isValid()) {
		emit errorOccurred("清空收藏失败: " + query.lastError().text());
	} else {
		refreshModel();
		emit countChanged();
	}
}

void FavoritesModel::setFilterType(const QString &type)
{
	if (m_filterType != type) {
		m_filterType = type;
		emit filterTypeChanged();
		refreshModel();
	}
}

void FavoritesModel::refreshModel()
{
	beginResetModel();
	m_items.clear();
	
	QSqlQuery query(m_db);
	QString sql = "SELECT id, title, artist, cover, source, duration, type, created_at FROM favorites";
	if (!m_filterType.isEmpty()) {
		sql += " WHERE type = :type";
	}
	sql += " ORDER BY created_at DESC"; // 最新的在前
	
	query.prepare(sql);
	if (!m_filterType.isEmpty()) {
		query.bindValue(":type", m_filterType);
	}
	
	if (query.exec()) {
		while (query.next()) {
			FavoriteItem item;
			item.id = query.value(0).toString();
			item.title = query.value(1).toString();
			item.artist = query.value(2).toString();
			item.cover = query.value(3).toString();
			item.source = query.value(4).toInt();
			item.duration = query.value(5).toInt();
			item.type = query.value(6).toString();
			item.createdAt = query.value(7).toDateTime();
			m_items.append(item);
		}
	} else {
		qWarning() << "Refresh favorites failed:" << query.lastError().text();
	}
	endResetModel();
}

QVariantMap FavoritesModel::get(int row) const
{
    if (row < 0 || row >= m_items.size())
        return {};

    const auto &item = m_items.at(row);
    QVariantMap map;
    map["id"] = item.id;
    map["title"] = item.title;
    map["artist"] = item.artist;
    map["cover"] = item.cover;
    map["source"] = item.source;
    map["duration"] = item.duration;
    map["type"] = item.type;
    map["createdAt"] = item.createdAt;
    return map;
}
