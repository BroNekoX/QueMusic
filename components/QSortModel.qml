// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQml.Models

// 排序代理：mode 0 源模型自然顺序（免排序） 1 名称 2 歌手 3 时长，方向由 sortDesc 决定
SortFilterProxyModel {
    id: root
    property int sortMode: 0
    property bool sortDesc: false
    property var options: []            // [{label, mode, desc}]，供菜单展示与回显
    property string nameRole: "name"
    property string artistRole: "artist"

    readonly property int menuIndex: {
        for (var i = 0; i < root.options.length; i++)
            if (root.options[i].mode === root.sortMode && root.options[i].desc === root.sortDesc) return i
        return 0
    }

    function orderFor(baseDesc) {
        return baseDesc !== root.sortDesc ? Qt.DescendingOrder : Qt.AscendingOrder
    }

    function selectMenu(i) {
        root.sortMode = root.options[i].mode
        root.sortDesc = root.options[i].desc
    }

    sorters: [
        StringSorter {
            roleName: root.nameRole
            enabled: root.sortMode === 1
            sortOrder: root.orderFor(false)
            caseSensitivity: Qt.CaseInsensitive
        },
        StringSorter {
            roleName: root.artistRole
            enabled: root.sortMode === 2
            sortOrder: root.orderFor(false)
            caseSensitivity: Qt.CaseInsensitive
        },
        RoleSorter {
            roleName: "duration"
            enabled: root.sortMode === 3
            sortOrder: root.orderFor(false)
        }
    ]
    onSortModeChanged: root.invalidateSorter()
    onSortDescChanged: root.invalidateSorter()

    function at(i) { return root.model.get(root.mapToSource(root.index(i, 0)).row) }
}
