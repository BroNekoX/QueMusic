// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
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
        // 配置文件存放在软件目录下（统一使用 INI，不使用注册表/plist）
        location: configDir + "/Options.ini"
        
        //全局
        property real musicVolume: 0.6
        property bool closeToManage: false //关闭则最小化托盘
        property bool autoUpdate: false //自动检查更新
        
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
        property bool displayDebugControl //显示控制台
    }

    // 最后播放的歌曲（关闭软件时保存，下次打开首页显示）
    property Settings lastSongs: Settings {
        category: "LastMedia"
        location: configDir + "/LastMedia.ini"
        property string name: ""
        property string artist: ""
        property string cover: ""
        property string hash: ""
        property int source: -1
    }
    
    signal changeOptions()
}