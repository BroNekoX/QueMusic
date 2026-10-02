// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 声明一个扩展点：target 传该处的容器（Row / Column / Item），插件挂上来的界面以它为 parent。
import QtQuick
import QueMusic 1.0

QtObject {
    id: slot

    property string slotName: ""
    property var target: null

    Component.onCompleted: PluginSlots.register(slot.slotName, slot.target)
    Component.onDestruction: PluginSlots.unregister(slot.slotName)
}
