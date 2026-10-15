// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

// 「3D」歌词界面的自有设置（自动持久化），同 LyricsFreeConfig：单例化以便跨组件引用可被 AOT 编译。
pragma Singleton
import QtCore

Settings {
    // 换过 key 空间：这一版改了相机/场景尺度与默认值，旧键值（camDist 8.4、fog 0.45…）
    // 会把新默认值顶掉，导致看着还是"老样子"。换 category 让新默认值真正生效。
    category: "Lyrics3D2"

    property int preset: 0
    property real bright: 0.85
    property real saturate: 1.45
    property real audio: 1.0
    property real density: 1.0
    property real fieldScale: 1.0
    property real camDist: 10.4         // 相机半径（5.0 ~ 14.0）
    property real camSens: 1.0          // 拖动灵敏度倍率
    property real fov: 46               // 垂直视场角（度）
    property real lyricScale: 1.0       // 歌词板世界尺寸（1.0 ≈ 占画面 6.5 成）
    property real lyricY: 0.0           // 歌词板高度偏移
    property real lyricZ: 0.0           // 歌词板前后偏移（沿场景正前方）
    property real glow: 0.55
    property real sweep: 0.7
    property real beatGlow: 0.6
    property real reflect: 0.45         // 地面倒影
    property real readab: 0.40          // 暗底衬可读性
    property real fog: 0.28             // 深度雾（按行距淡出）
    property real dof: 0.30             // 按行距的景深虚化
    property real floatAmt: 0.4
    property string textColorCustom: ""
    property string glowColorCustom: ""
}