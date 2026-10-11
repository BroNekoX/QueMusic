// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
pragma Singleton
import QtCore

Settings {
    category: "Options"

    // 全局
    property real musicVolume: 0.6
    property bool closeToManage: false     // 关闭则最小化到托盘
    property bool rememberWindow: false    // 记住窗口位置和大小
    property int winX: 0
    property int winY: 0
    property int winW: 0
    property int winH: 0
    property bool autoUpdate: false
    property list<string> searchList: []   // 搜索历史
    property bool openShortCut: true
    property int cycleIndex: 0
    property bool picCache: false

    // 播放器
    property int soundQuality: 1           // 音质档位
    property bool useDefaultDevice: true
    property int audioDevice: 0
    property bool autoPlay: true
    property int playerRateIndex: 2        // 倍速预设
    property int volumeStep: 5             // 音量步长(%)

    // 播放增强
    property int seekStep: 5               // 精确跳转步长（秒）
    property bool fadeEnabled: true
    property int fadeMs: 200
    property int sleepMinutes: 30
    property int historyLimit: 200
    property int shuffleAvoid: 2           // 随机播放避开最近 N 首
    property bool autoRestoreQueue: true
    property string playHistory: "[]"      // 播放历史(JSON)
    property string lastQueue: "[]"        // 上次播放列表(JSON)
    property int lastQueueIndex: -1

    // 媒体
    property string downloadFolder: ""
    property int scanTime: 15              // 自动扫描更新间隔
    property string cacheUrl: ""
    property string dataUrl: ""
    property int metaDataSource: 0         // 0.默认 1.Metadata 2.NetWork
    property int mainMusicSource: 0        // 默认歌曲源，详见 MusicApi
    property int serverAgency: 0
    property int cacheSize: 500            // 缓存上限(mb)
    property string editSource: ""

    // 高级
    property bool noWindowKit: false
    property bool softwareRender: false
    property int gpuRenderMode: 0          // 0.默认平台 1.OpenGL 2.Vulkan
    property bool displayFps: false
    property bool debug: false
    property bool timerAnimator: false
    property bool qmlAnimator: false
    property bool displayDebugControl
}
