// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef PLAYERDATABASE_H
#define PLAYERDATABASE_H

#include <QSqlDatabase>

// GUI 线程共享连接（懒建、自动建表）。批量读写请走 DbService。
QSqlDatabase playerDatabase();

#endif // PLAYERDATABASE_H
