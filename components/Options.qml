// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QtCore

QtObject {
    //property alias settings: settings

    // 配置存储，后续也可以存储在服务器数据库中
    // 使用存储仅需把QtObject换成Settings
    property Settings settings: Settings {
        //id: settings
        category: "Options"
        //location: configDir + "/Options.ini"
        
        //全局
        property real musicVolume: 0.6
        property bool closeToManage: false //关闭则最小化托盘
        property bool autoUpdate: false //自动检查更新
        property list<string> searchList: [] //搜索记录
        property bool openShortCut: true
        property bool recordingShortCut: false // 正在录制快捷键时禁用所有全局快捷键，避免组合键被拦截
        // 每个功能单独控制是否为全局快捷键（#44：不要用总开关控制所有功能）
        property bool globalShortcutPlay: true
        property bool globalShortcutBack: true
        property bool globalShortcutForward: true
        property bool globalShortcutPlayList: true
        property bool globalShortcutMusicControl: true
        
        //播放器
        property int soundQuality: 1 //音质
        property int equalizer: 0 //均衡器 0.默认 1.自定义 
        //property url downloadFile: ""
        property bool useDefaultDevice: true //默认输出设备
        property int audioDevice: 0 //输出设备
        property int sampleRate: 2 //对应不同质量如44100 96000
        property int depth: 1 //位深
        property int bufferSize: 2 //缓冲大小
        property bool autoPlay: true
        
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

    // 播放器行为设置（跨平台持久化：Windows/macOS/Linux 由 QSettings 写入各自配置目录）
    property Settings playSettings: Settings {
        category: "Player"
        // 播放顺序：0 列表循环，1 单曲循环，2 随机播放，3 顺序播放
        property int cycleIndex: 0
    }

    // 最后播放的歌曲（关闭软件时保存，下次打开首页显示）
    property Settings lastSongs: Settings {
        category: "LastMedia"
        //location: configDir + "/LastMedia.ini"
        property string name: ""
        property string artist: ""
        property string cover: ""
        property string hash: ""
        property int source: -1
    }

    property QtObject shortCuts: QtObject {
        //category: "ShortCuts"
        //location: configDir + "/ShortCut.ini"
        property string play: "Space"
        property string back: "Left"
        property string forward: "Right"
        property string playList: "Alt"
        property string musicControl: "Up"
    }
    signal changeOptions()
}