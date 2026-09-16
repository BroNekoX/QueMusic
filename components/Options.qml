// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QtCore

QtObject {
    property bool recordingShortCut: false
    property string version: "Beta-0.5.0"
    property int versionCode: 50

    // 配置存储，后续也可以存储在服务器数据库中
    // 使用存储仅需把 QtObject 换成 Settings
    property OptionsSettings settings: OptionsSettings {}

    // 最后播放的歌曲（关闭软件时保存，下次打开首页显示）
    property OptionsLastSongs lastSongs: OptionsLastSongs {}

    // 快捷键（持久化到 ShortCuts 配置组）
    property OptionsShortCuts shortCuts: OptionsShortCuts {}

    signal changeOptions()
}
