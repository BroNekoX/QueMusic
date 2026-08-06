// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QtCore

QtObject {
    // Style.settings.name
    //property alias settings: settings
    // Style.themes.name
    readonly property bool darkis: settings.theme === 0 ? false : settings.theme === 1 ? true : Qt.application.styleHints.colorScheme === Qt.ColorScheme.Dark
    //readonly property var themes: darkis ? darkThemes[settings.color] : lightThemes[settings.color]
    onDarkisChanged: {
        Style.changeTheme();
    }

    // 配置存储，后续也可以存储在服务器数据库中
    property Settings settings: Settings {
        //id: settings
        category: "Style"
        location: configDir + "/Style.ini"
        
        // 全局主题-参考设置页面
        property int theme: 0 //主题样式
        property int color: 0 //主题色
        property list<color> colorList: ["#3481fa","#34fa4a","#faad34","#ad34fa"]
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
        property real borderDepth: 0.2 //边框透明度
        property bool layerEnabled: true
        property bool premiumAnime: true //高级动画
        property int animeSpeed: 1

        // UI设置
        property bool sidebarColor: false
        property int sidebarStyle: 0
        property int menutheme: 1
        //1.material 2.fluent
        property int glmode: 0
        property int uilevel: 0
        
        // 歌词界面
        property int lyricSize: 10
        property int backFlowQuality: 0
        property bool waveDisplay: true //显示音波效果
        property bool premiumLyricAnime: true //高级逐行弹簧动画
        property int textWidth: 600
        property bool maskBlur: true
        property bool lyricHideGui: true

        // 背景图片
        property string backgroundImage: "qrc:/QueMusic/resources/pic/back2.jpg"

    }
    
    signal changeUi()
    signal changeTheme()
    signal warned(string text,int type)
    onChangeTheme: {
        //var darkis = Style.darkis;
        var baseColor = settings.colorList[settings.color];
        themes.fontColor = darkis ? "#ffffff" : "#000000";
        themes.textColor = darkis ? "#eaeaea" : "#333333";
        themes.fullColor = darkis ? "#000000" : "#ffffff";
        themes.hoverColor = darkis ? "#1affffff" : "#1a000000";
        themes.sideColor = darkis ?  "#383838" : "#eaeaea";
        themes.sideBlurColor = darkis ? "#88383838" : "#88eaeaea";

        themes.containColor = darkis ? Qt.hsva(baseColor.hsvHue,0.9,0.4,1.0) : Qt.hsva(baseColor.hsvHue,0.2,1.0,1.0);
        themes.containOutColor = darkis ? Qt.hsva(baseColor.hsvHue,0.2,1.0,1.0) : Qt.hsva(baseColor.hsvHue,0.9,0.4,1.0);
        themes.themeColor = baseColor
        themes.primaryColor = darkis ? Qt.hsva(baseColor.hsvHue,0.1,0.1,1.0) : Qt.hsva(baseColor.hsvHue,0.01,1.0,1.0);
        themes.primaryBlurColor = darkis ? Qt.hsva(baseColor.hsvHue,0.1,0.1,0.7) : Qt.hsva(baseColor.hsvHue,0.01,1.0,0.7);
        themes.secondaryColor = darkis ? Qt.hsva(baseColor.hsvHue,0.13,0.16,1.0) : Qt.hsva(baseColor.hsvHue,0.02,0.97,1.0);
        themes.secondaryBlurColor = darkis ? Qt.hsva(baseColor.hsvHue,0.13,0.16,0.7) : Qt.hsva(baseColor.hsvHue,0.02,0.97,0.7);
        themes.blurOverlayColor = darkis ? Qt.hsva(baseColor.hsvHue,0.1,0.1,0.5) : Qt.hsva(baseColor.hsvHue,0.01,1.0,0.5);
        themes.blurSecondaryColor = darkis ? Qt.hsva(baseColor.hsvHue,0.13,0.16,0.5) : Qt.hsva(baseColor.hsvHue,0.02,0.97,0.5);
        themes.shadowColor = darkis ? Qt.hsva(baseColor.hsvHue,1.0,0.05,0.20) : Qt.hsva(baseColor.hsvHue,1.0,0.14,0.16);
        themes.themeShadowColor = darkis ? Qt.hsva(baseColor.hsvHue,1.0,0.5,0.3) : Qt.hsva(baseColor.hsvHue,1.0,0.6,0.3);
    }

    property QtObject themes: QtObject {
        //id: allThemes
        //主色
        property color themeColor: "#3481fa"
        // 不随颜色改变属性
        property color fontColor: "#000000"
        property color textColor: "#333333"
        property color fullColor: "#ffffff"
        property color hoverColor: "#1a000000"
        property color sideColor: "#eaeaea"
        property color sideBlurColor: "#88eaeaea"
        // 随颜色改变
        property color containColor: "#cde0fe"
        property color containOutColor: "#022760"
        property color primaryColor: "#fdfdff"
        property color primaryBlurColor: "#c4fdfdff"
        property color secondaryColor: "#f3f5f8"
        property color secondaryBlurColor: "#c4f3f5f8"
        property color blurOverlayColor: "#88fdfdff"
        property color blurSecondaryColor: "#88f3f5f8"
        property color shadowColor: "#31001024"
        property color themeShadowColor: "#660f58a6"
    }
}