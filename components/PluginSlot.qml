// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 声明一个功能插件扩展点：放在需要对外开放的位置，target 传该处的容器（Row / Item 都行），
// 插件挂上来的界面就以它为 parent，因此横排、锚定还是自由摆放由宿主决定。
// 本身不是 Item，不参与排版、不占空间。
import QtQuick
import QueMusic 1.0

QtObject {
    id: slot

    property string slotName: ""
    property var target: null

    Component.onCompleted: PluginSlots.register(slot.slotName, slot.target)
    Component.onDestruction: PluginSlots.unregister(slot.slotName)
}
