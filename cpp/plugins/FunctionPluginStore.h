// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 功能插件：和歌词界面插件同一套目录结构（info.json + 入口 QML），但可以同时启用多个。
// 宿主 components/PluginHost.qml 按 enabledIds 给每个启用的插件起一个加载器；
// 插件拿到注入的 api 后自己挂到扩展点（PluginSlots）或另建 Loader。
#pragma once

#include "plugins/PluginStore.h"

#include <QStringList>
#include <QtQml/qqml.h>

class FunctionPluginStore : public PluginStore
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FunctionPlugins)
    QML_SINGLETON
    Q_PROPERTY(QStringList enabledIds READ enabledIds NOTIFY enabledChanged)

public:
    explicit FunctionPluginStore(QObject *parent = nullptr);
    static FunctionPluginStore *create(QQmlEngine *, QJSEngine *) { return new FunctionPluginStore(); }

    const QStringList &enabledIds() const { return m_enabled; }
    Q_INVOKABLE bool isEnabled(const QString &id) const { return m_enabled.contains(id); }
    Q_INVOKABLE void setEnabled(const QString &id, bool on);

    Q_INVOKABLE void rescan() override;

signals:
    void enabledChanged();

private:
    void loadEnabled();   // 新插件默认启用，被停用/加载失败过的保持停用

    QStringList m_enabled;
};
