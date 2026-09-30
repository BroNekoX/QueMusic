// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 插件目录通用逻辑：每个插件一个目录（info.json + 入口 QML + 可选预览图），目录名即插件 id。
// 这里只负责「磁盘上的插件」——扫描、安装（复制目录）、删除、打开目录；
// 歌词界面插件（单选）与功能插件（可多选）各自的状态由派生类补上。
// 注意：派生类构造函数里要调一次 load()，load 会回调虚函数 builtinPlugins()。
#pragma once

#include <QObject>
#include <QVariantList>
#include <QtQml/qqml.h>

class PluginStore : public QObject
{
    Q_OBJECT
    // 只作为两个插件单例的基类：登记为匿名类型，QML 工具链才认得出派生类继承来的 plugins / dir
    QML_ANONYMOUS
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY pluginsChanged)   // 内置 + 已安装
    Q_PROPERTY(QString dir READ dir NOTIFY pluginsChanged)                // 安装目录

public:
    const QVariantList &plugins() const { return m_plugins; }
    QString dir() const;

    Q_INVOKABLE virtual void rescan();
    // 安装 / 覆盖更新；返回错误文本，空串表示成功
    Q_INVOKABLE QString install(const QString &folder);
    Q_INVOKABLE QString remove(const QString &id);
    // id 为空表示打开插件根目录
    Q_INVOKABLE void reveal(const QString &id);

signals:
    void pluginsChanged();

protected:
    PluginStore(QString kind, QString folder, QObject *parent = nullptr);

    // 内置插件（不可卸载），默认没有
    virtual QVariantList builtinPlugins() const { return {}; }
    virtual int apiVersion() const { return 1; }

    void load();
    bool find(const QString &id, QVariantMap *info) const;

    QVariantList m_plugins;

private:
    QString writableRoot() const;

    const QString m_kind;      // 错误文案用，如「歌词界面插件」
    const QString m_folder;    // 安装子目录，如 lyrics / function
};
