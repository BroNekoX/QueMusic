// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 扩展点注册表：宿主界面里放一个 PluginSlot 即登记一处「名字 → 容器」，插件用 api.mount 往里挂界面。
pragma Singleton
import QtQuick

QtObject {
    // 宿主开放：titlebar / player / sidebar.bottom / window.overlay
    readonly property var items: ({})

    function register(name: string, target: var): void {
        if (name && target)
            items[name] = target;
    }

    function unregister(name: string): void {
        delete items[name];
    }

    function item(name: string): var {
        return items[name] !== undefined ? items[name] : null;
    }
}
