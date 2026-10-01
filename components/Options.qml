// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick

QtObject {
    property bool recordingShortCut: false
    readonly property string version: "0.5.1"
    readonly property int versionCode: 51
    readonly property int pluginApi: 10

    // 配置存储，后续也可以存储在服务器数据库中
    // 使用存储仅需把 QtObject 换成 Settings
    property OptionsSettings settings: OptionsSettings {}

    // 最后播放的歌曲（关闭软件时保存，下次打开首页显示）
    property OptionsLastSongs lastSongs: OptionsLastSongs {}

    // 快捷键（持久化到 ShortCuts 配置组）
    property OptionsShortCuts shortCuts: OptionsShortCuts {}

    signal changeOptions()
}
