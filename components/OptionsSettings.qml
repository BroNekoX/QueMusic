// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 通用配置存储（有类型，供 qmlcachegen 做 AOT 编译，详见 StyleSettings.qml 说明）
import QtCore

Settings {
    category: "Options"

    //全局
    property real musicVolume: 0.6
    property bool closeToManage: false //关闭则最小化托盘
    property bool rememberWindow: false //记住窗口位置和大小
    property int winX: 0
    property int winY: 0
    property int winW: 0
    property int winH: 0
    property bool autoUpdate: false //自动检查更新
    property list<string> searchList: [] //搜索记录
    property bool openShortCut: true
    property int cycleIndex: 0

    //播放器
    property int soundQuality: 1 //音质
    property int equalizer: 0 //均衡器 0.默认 1.自定义
    property bool useDefaultDevice: true //默认输出设备
    property int audioDevice: 0 //输出设备
    property int sampleRate: 2 //对应不同质量如44100 96000
    property int depth: 1 //位深
    property int bufferSize: 2 //缓冲大小
    property bool autoPlay: true
    property int playerRateIndex: 2 //倍速预设
    property int volumeStep: 5 //音量步长(%)

    //播放增强
    property int seekStep: 5 //精确跳转步长（秒）
    property bool fadeEnabled: true //播放淡入淡出
    property int fadeMs: 200 //淡入淡出时长
    property int sleepMinutes: 30 //睡眠定时默认分钟
    property int historyLimit: 200 //播放历史上限
    property int shuffleAvoid: 2 //随机播放避免最近N首
    property bool resumePosition: true //断点续播
    property bool autoRestoreQueue: true //启动时恢复上次播放列表
    property string playHistory: "[]" //播放历史(JSON)
    property string lastQueue: "[]" //上次播放列表(JSON)
    property int lastQueueIndex: -1 //上次播放位置

    //媒体
    property string downloadFolder: ""
    property int scanTime: 15 //自动扫描更新文件夹内容
    property string cacheUrl: "" //默认缓存位置
    property string dataUrl: "" //数据库存储位置
    property int metaDataSource: 0 //0.默认 1.Metadata 2.NetWork

    property int mainMusicSource: 0 //默认歌曲源，详见MusicApi
    property int serverAgency: 0 //代理服务器
    property int cacheSize: 500 //缓存大小mb
    property string editSource: "" //自定义源

    //advance高级
    property bool noWindowKit: false //不使用高级无边框窗口
    property bool softwareRender: false //使用软件渲染
    property int gpuRenderMode: 0 //0.默认平台 1.OpenGL 2.Vulkan
    property bool displayFps: false //显示帧率
    property bool debug: false //使用调试模式
    property bool timerAnimator: false //使用Timer动画引擎
    property bool qmlAnimator: false //使用vsync动画引擎
    property bool displayDebugControl //显示控制台
}
