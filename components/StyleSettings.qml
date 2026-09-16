// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 样式配置存储。
import QtQuick
import QtCore

Settings {
    category: "Style"

    // 全局主题-参考设置页面
    property int theme: 0 //主题样式
    property int color: 0 //主题色
    property list<color> colorList: ["#3481fa", "#34fa4a", "#faad34", "#ad34fa"]
    property int backmode: 0 //背景模式
    property int backpic: 0 //背景图片选择

    // 阴影,模糊,发光效果
    property real shadowBlur: 1.0 //阴影模糊
    property int shadowXOffset: 5
    property int shadowYOffset: 5
    property int shadowSize: 16
    property int blurSize: 48
    property int lightEffect: 16 //光渲染大小
    property bool highQualityBlur: false //高质量模糊

    // 文字统一
    property int textH1: 22
    property int textH2: 17
    property int textmain: 13
    property int text: 12
    property int textTip: 11 //小提示
    property int texticon: 16
    property int pageTitle: 26 //页头
    property int texticonH: 22

    // 全局UI控件
    property int labelRadius: 12
    property int cubeRadius: 16
    property bool noControlRadius: false
    property real borderDepth: 0.08 //边框透明度
    property bool layerEnabled: true
    property bool premiumAnime: true //高级动画
    property int animeSpeed: 1
    property int homeLayout: 0 //首页布局 0.默认 1.竖向 2.混合
    property int spotSpeed: 1 //桌面部件动画速度 0.快 1.默认 2.慢

    // UI设置
    property bool sidebarColor: false
    property int sidebarStyle: 1
    property int menutheme: 1
    //1.material 2.fluent
    property int glmode: 0
    property int uilevel: 0

    // 歌词界面
    property int lyricSize: 10
    property int flowStyle: 1 // 流体背景算法：0=Fluid 1=Classic 2=静态渐变
    property bool waveDisplay: true //显示音波效果
    property bool premiumLyricAnime: true //高级逐行弹簧动画
    property int textWidth: 600
    property bool maskBlur: true
    property bool lyricHideGui: true

    property string fontFamily

    // 背景图片
    property string backgroundImage: "qrc:/QueMusic/resources/pic/back2.jpg"
}
