// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 快捷键配置（有类型，供 qmlcachegen 做 AOT 编译）
import QtCore

Settings {
    category: "ShortCuts"

    property string play: "Space"
    property string back: "Left"
    property string forward: "Right"
    property string playList: "Alt"
    property string musicControl: "Up"
    // 辅助快捷键
    property string volumeUp: "Ctrl+Up"
    property string volumeDown: "Ctrl+Down"
    property string seekBack: "Ctrl+Left"
    property string seekForward: "Ctrl+Right"
    property string mute: "Ctrl+M"
    property string abLoop: "Ctrl+B"
    property string favorite: "Ctrl+D"
    property string playerOptions: "Ctrl+T"

    // 每个功能单独控制是否为全局快捷键（#44：不要用总开关控制所有功能）
    property bool globalShortcutPlay: true
    property bool globalShortcutBack: true
    property bool globalShortcutForward: true
    property bool globalShortcutPlayList: true
    property bool globalShortcutMusicControl: true
    property bool globalShortcutVolumeUp: true
    property bool globalShortcutVolumeDown: true
    property bool globalShortcutSeekBack: true
    property bool globalShortcutSeekForward: true
    property bool globalShortcutMute: true
    property bool globalShortcutAbLoop: true
    property bool globalShortcutFavorite: true
    property bool globalShortcutPlayerOptions: true
}
