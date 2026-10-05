// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QueMusic 1.0
import QWindowKit 1.0

QtObject {
    // 跨文件共享对象：main.qml 启动时注入
    property QueueModel queue
    property QWarn warn
    property QAlertDialog dialog
    property WindowAgent agent
    property DesktopPlayer desktop
    property SmtcBridge smtc

    // window 上移过来的状态
    property int exitIndex: 0          // 要求主界面切到的页
    property bool homeLoaded: false    // 首页是否已加载过
    property bool recordingShortCut: false
    readonly property string version: "0.6.0"
    readonly property int versionCode: 60
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
