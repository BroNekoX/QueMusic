// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 播放器数据库（唯一共享连接）的统一入口。
//
// 为什么必须有这个入口：
// 连接名 "shared_player_db" 原先只由 FolderModel 的静态函数创建，而 FavoritesModel
// 只做查找、并把查找结果缓存在自己的函数内静态变量里。模型改成 QML 单例后由 QML 引擎
// 按“首次访问顺序”惰性创建，一旦收藏模型先于歌单模型被访问，它就会永久持有一个无效连接
// （日志里的 "Shared database not available!" / "Driver not loaded"），表现为收藏列表为空。
//
// 现在创建与查找都收在这里，任何模块先调用都会正确初始化，不再依赖构造顺序。
#ifndef PLAYERDATABASE_H
#define PLAYERDATABASE_H

#include <QSqlDatabase>

// 返回共享连接（必要时创建连接、打开数据库并建好基础表）。
// 线程亲和性：仅限 GUI/主线程使用；工作线程请自建独立连接（见 Favorites.cpp）。
QSqlDatabase playerDatabase();

#endif // PLAYERDATABASE_H
