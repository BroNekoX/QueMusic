// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 面向 QML 的数据模型单例。
//
// 用独立单例而非 setContextProperty：上下文属性无法被 qmlcachegen 编译期解析，按类型名访问
// （如 FavoriteSongs.count）才能编译；也不能合并成"一个单例持多个模型属性"，会拒绝继续属性查找。
//
// ⚠️ 构造函数【绝不能】带默认参数：singletonConstructionMode() 中 is_default_constructible
// 优先于 HasSingletonFactory，写成 X(QObject *parent = nullptr) 会走默认构造、create() 永不执行，
// 表现为过滤类型失效、收藏列表列出全部数据。
#ifndef APPMODELS_H
#define APPMODELS_H

#include <QJSEngine>
#include <QQmlEngine>

#include "Favorites.h"
#include "FolderModel.h"

#define QUEMUSIC_DECLARE_MODEL_SINGLETON(CLASS, BASE, FILTER) \
    class CLASS : public BASE                                    \
    {                                                            \
        Q_OBJECT                                                 \
        QML_ELEMENT                                              \
        QML_SINGLETON                                            \
    public:                                                      \
        explicit CLASS(QObject *parent) : BASE(parent) {}         \
        static CLASS *create(QQmlEngine *qmlEngine, QJSEngine *)  \
        {                                                        \
            CLASS *model = new CLASS(qmlEngine);                 \
            model->setFilterType(QStringLiteral(FILTER));        \
            return model;                                        \
        }                                                        \
    }

// 我的歌单 / 本地音乐（过滤类型即模型自身的语义）
QUEMUSIC_DECLARE_MODEL_SINGLETON(MyFolders, FolderModel, "my");
QUEMUSIC_DECLARE_MODEL_SINGLETON(LocalFolders, FolderModel, "local");

// 歌单内的歌曲列表（该模型不使用过滤类型）
class Songs : public SongModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit Songs(QObject *parent) : SongModel(parent) {}
    static Songs *create(QQmlEngine *qmlEngine, QJSEngine *)
    {
        return new Songs(qmlEngine);
    }
};

// 收藏：歌曲 / 歌单 / 歌手
QUEMUSIC_DECLARE_MODEL_SINGLETON(FavoriteSongs, FavoritesModel, "song");
QUEMUSIC_DECLARE_MODEL_SINGLETON(FavoritePlaylists, FavoritesModel, "playlist");
QUEMUSIC_DECLARE_MODEL_SINGLETON(FavoriteArtists, FavoritesModel, "artist");

#undef QUEMUSIC_DECLARE_MODEL_SINGLETON

#endif // APPMODELS_H
