// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 功能插件扩展点注册表：宿主界面里放一个 PluginSlot 就登记一处「名字 → 容器」，
// 插件随后用 api.slot(name) 取容器、api.mount(name, Component) 往里面挂界面。
// 目前开放：titlebar.left / titlebar.right / player.left / player.right /
//           sidebar.bottom / window.overlay
pragma Singleton
import QtQuick

QtObject {
    id: registry

    // 扩展点名 → 容器对象；只在宿主里声明，插件侧只读
    readonly property var items: ({})

    function register(name: string, target: var): void {
        if (!name || !target)
            return;
        items[name] = target;
    }

    function unregister(name: string): void {
        delete items[name];
    }

    function item(name: string): var {
        return items[name] !== undefined ? items[name] : null;
    }
}
