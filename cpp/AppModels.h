// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 面向 QML 的数据模型单例。
//
// 背景：这些模型原先通过 QQmlContext::setContextProperty() 以裸名字（songModel、
// favoritesSong…）暴露给 QML。上下文属性无法被 qmlcachegen 在编译期解析，相关绑定只能
// 退回解释执行。改造成 QML 单例后 QML 侧按类型名访问（如 FavoriteSongs.count）。
//
// 为什么必须拆成一个个独立的单例类型，而不是"一个单例持有多个模型属性"：
// 实测（Qt 6.10.3）qmlcachegen 对"经由对象属性拿到的对象"一律拒绝继续做属性查找：
//     Cannot use shadowable base type for further lookups: Xxx::yyy with type ...
// 只有直接按类型名访问的单例（与工程里 MusicApi 的用法一致）才能把 xxx.count /
// xxx.loading / xxx.isFavorite(...) 这类访问编译成 C++。
//
// ⚠️ 构造函数【绝不能】带默认参数（LANGUAGE 级坑，务必保留注释）：
// Qt 的 qqmlprivate.h 中 singletonConstructionMode() 的判定顺序是
//     FactoryWrapper  ->  is_default_constructible  ->  HasSingletonFactory
// 即"可默认构造"优先于 create()。若构造函数写成 X(QObject *parent = nullptr)，
// 引擎会走默认构造分支（new T），下面的 create() 永远不会被调用 —— 过滤类型也就永远
// 不会被设置，表现为各个收藏列表（歌曲/歌单/歌手）都把全部数据列出来。
// 显式去掉默认参数即可强制引擎走 create() 分支。
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
