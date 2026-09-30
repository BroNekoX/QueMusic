// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 功能插件宿主：放进主窗口（不可见、不占位）。每个插件一个加载器，只有启用中的会被创建；
// 启用状态变化只影响对应那一个插件，其余不受影响。
import QtQuick
import QueMusic 1.0

Item {
    id: host

    width: 0
    height: 0
    visible: false

    // 某个插件实例建好了：宿主据此重做一次界面层收尾（如标题栏扩展点的可命中标记）
    signal pluginLoaded()

    Repeater {
        model: FunctionPlugins.plugins

        delegate: PluginLoader {
            info: modelData
            active: FunctionPlugins.enabledIds.indexOf(modelData.id) >= 0
            onInstanceChanged: if (instance) host.pluginLoaded()
        }
    }
}
