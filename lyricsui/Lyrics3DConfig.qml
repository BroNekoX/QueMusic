// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 「3D」歌词界面的自有设置（自动持久化），同 LyricsFreeConfig：单例化以便跨组件引用可被 AOT 编译。
pragma Singleton
import QtCore

Settings {
    category: "Lyrics3D"

    property int preset: 0
    property real bright: 0.85
    property real saturate: 1.45
    property real audio: 1.0
    property real density: 1.0
    property real fieldScale: 1.0
    property real camDist: 9.0
    property real camSens: 1.0
    property real tilt: 0.5
    property real lyricScale: 1.0
    property real lyricY: 0.0
    property real lyricZ: 0.0
    property real glow: 0.55
    property real sweep: 0.7
    property real beatGlow: 0.6
    property real floatAmt: 0.4
    property string textColorCustom: ""
    property string glowColorCustom: ""
}
