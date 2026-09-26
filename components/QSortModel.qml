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
    // 以 var 持有源模型，才能探测它的取行接口
    readonly property var sourceModel: root.model

    readonly property int menuIndex: {
        for (let i = 0; i < root.options.length; i++)
            if (root.options[i].mode === root.sortMode && root.options[i].desc === root.sortDesc) return i
        return 0
    }

    function orderFor(baseDesc: bool): int {
        return baseDesc !== root.sortDesc ? Qt.DescendingOrder : Qt.AscendingOrder
    }

    function selectMenu(i: int): void {
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

    // 行取值：Favorites / SongModel 用 get(row)，SearchResultModel 用 getRow(row)
    function get(i: int): var {
        const s = sourceModel
        if (!s || i < 0)
            return null
        const src = root.mapToSource(root.index(i, 0))
        return src.valid ? (s.getRow ? s.getRow(src.row) : s.get(src.row)) : null
    }
    function at(i: int): var { return get(i) }
}
