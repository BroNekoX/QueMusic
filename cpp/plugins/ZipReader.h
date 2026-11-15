// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#pragma once

#include <QByteArray>
#include <QString>

// 最小 ZIP 解包：只支持 store / deflate，够插件包用
class ZipReader
{
public:
    static bool extract(const QByteArray &zip, const QString &destDir, QString *error = nullptr);
};