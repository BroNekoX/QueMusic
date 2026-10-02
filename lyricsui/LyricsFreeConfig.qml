// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 「自由」歌词界面的自有设置（自动持久化）。
// 以前是界面根里的 Settings + 文件内 id（cfg），子组件跨边界引用无法被 AOT 编译，改为单例。
pragma Singleton
import QtCore

Settings {
    category: "LyricsFree"

    property int bgStyle: 0            // 0 流体 / 1 图片 / 2 视频 / 3 平面星空
    property string bgImage: ""
    property string bgVideo: ""
    property real lyricSpacingScale: 1.0
    property int lyricCardAngle: 0
    property real lyricCurrentScale: 1.02
    property real lyricIdleOpacity: 0.5
    property string lyricSungColor: ""
    property string lyricLineColor: ""
}
