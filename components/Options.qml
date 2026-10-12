// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0

QtObject {
    // 跨文件共享对象：main.qml 启动时注入
    property QueueModel queue
    property QAlertDialog dialog
    property DesktopPlayer desktop
    property SmtcBridge smtc
    readonly property bool isMacOS: Qt.platform.os === "osx"

    property Item mainLayout
    property GetWave getWave
    property Loader textWatch
    property QOptionDialog picWatch
    property SongInfoDialog songInfo
    property CoverHelper coverHelper
    property Loader desktopPlayerLoader
    property Loader desktopLyricsLoader
    property NumberAnimation openMaxLyric
    property NumberAnimation settingOutAnime
    property TextField mainSearchInput

    // main状态
    property int exitIndex: 0          // 要求主界面切到的页
    property bool homeLoaded: false    // 首页是否已加载
    property bool playlistLoaded: false  // 分类是否已加载
    property bool recordingShortCut: false
    readonly property string version: "0.6.0"
    readonly property int versionCode: 60
    readonly property int pluginApi: 10
    property string searchText: ""

    // 最后播放的歌曲（关闭软件时保存，下次打开首页显示）
    property OptionsLastSongs lastSongs: OptionsLastSongs {}

    signal changeOptions()
    signal exit()
    signal changeUi()
    signal changeTheme()
    signal warned(string text, int type)
    signal reSearch()

    readonly property bool darkis: Style.theme === 0 ? false : Style.theme === 1 ? true : Qt.application.styleHints.colorScheme === Qt.ColorScheme.Dark
    onDarkisChanged: Options.changeTheme()

    onChangeTheme: {
        const hue = Style.colorList[Style.color].hsvHue;
        switch(Style.colorStyle) {
        case 0:
            Theme.fontColor = darkis ? "#e8e8ec" : "#1d1d1f";
            Theme.textColor = darkis ? "#a0a0a6" : "#5e5e61";
            Theme.fullColor = darkis ? "#17171a" : "#ffffff";
            Theme.hoverColor = darkis ? "#12ffffff" : "#0a000000";
            Theme.sideColor = darkis ? "#3a3a40" : "#e9e9ee";
            Theme.sideBlurColor = darkis ? "#953a3a40" : "#95e9e9ee";

            Theme.containColor = darkis ? Qt.hsva(hue, 0.40, 0.42, 1.0) : Qt.hsva(hue, 0.14, 1.0, 1.0);
            Theme.containOutColor = darkis ? Qt.hsva(hue, 0.20, 1.0, 1.0) : Qt.hsva(hue, 0.9, 0.4, 1.0);
            Theme.themeColor = Style.colorList[Style.color];

            Theme.primaryColor = darkis ? Qt.hsva(hue, 0.08, 0.22, 1.0) : Qt.hsva(hue, 0.01, 1.0, 1.0);
            Theme.primaryBlurColor = darkis ? Qt.hsva(hue, 0.08, 0.22, 0.82) : Qt.hsva(hue, 0.01, 1.0, 0.72);
            Theme.secondaryColor = darkis ? Qt.hsva(hue, 0.08, 0.16, 1.0) : Qt.hsva(hue, 0.015, 0.973, 1.0);
            Theme.secondaryBlurColor = darkis ? Qt.hsva(hue, 0.08, 0.16, 0.82) : Qt.hsva(hue, 0.015, 0.973, 0.72);
            Theme.borderColor = darkis ? Qt.hsva(hue, 0.06, 0.32, 1.0) : Qt.hsva(hue, 0.02, 0.97, 1.0);
            Theme.blurOverlayColor = darkis ? Qt.hsva(hue, 0.07, 0.14, 0.62) : Qt.hsva(hue, 0.01, 1.0, 0.55);
            Theme.blurSecondaryColor = darkis ? Qt.hsva(hue, 0.08, 0.17, 0.62) : Qt.hsva(hue, 0.02, 0.97, 0.55);
            Theme.shadowColor = darkis ? Qt.hsva(hue, 0.9, 0.03, 0.40) : Qt.hsva(hue, 0.9, 0.2, 0.1);
            Theme.themeShadowColor = darkis ? Qt.hsva(hue, 1.0, 0.5, 0.30) : Qt.hsva(hue, 1.0, 0.6, 0.3);
            break;

        case 1: {
            // Material 3
            const seed = Style.colorList[Style.color];
            const seedHue = seed.hsvHue;
            const seedSat = seed.hsvSaturation;

            // M3 强调色：浅色 T40，深色 T80
            const m3PrimaryLight = Qt.hsva(seedHue, seedSat * 0.9, 0.40, 1.0);
            const m3PrimaryDark  = Qt.hsva(seedHue, seedSat * 0.70, 0.80, 1.0);

            // Neutral 色（浅色）
            const m3NeutralLight        = Qt.hsva(seedHue, 0.04, 0.97, 1.0);
            const m3NeutralVariantLight = Qt.hsva(seedHue, 0.08, 0.90, 1.0);

            // Neutral 色（深色）——按 M3 的 Surface T10 / Container T15 / High T20 分层
            const m3NeutralDark          = Qt.hsva(seedHue, 0.04, 0.11, 1.0); // Surface (背景)
            const m3NeutralDarkContainer = Qt.hsva(seedHue, 0.05, 0.16, 1.0); // Container (卡片)
            const m3NeutralDarkHigh      = Qt.hsva(seedHue, 0.06, 0.22, 1.0); // High (浮层/悬浮)
            const m3NeutralVariantDark   = Qt.hsva(seedHue, 0.07, 0.26, 1.0); // 侧栏

            Theme.themeColor = darkis ? m3PrimaryDark : m3PrimaryLight;

            if (darkis) {
                // 深色：背景最暗，卡片更亮，浮层最亮，形成正向明度层级
                Theme.fullColor = m3NeutralDark;
                Theme.primaryColor = m3NeutralDarkContainer;
                Theme.primaryBlurColor = Qt.rgba(m3NeutralDarkContainer.r, m3NeutralDarkContainer.g, m3NeutralDarkContainer.b, 0.82);
                Theme.secondaryColor = m3NeutralDarkHigh;
                Theme.secondaryBlurColor = Qt.rgba(m3NeutralDarkHigh.r, m3NeutralDarkHigh.g, m3NeutralDarkHigh.b, 0.82);

                Theme.sideColor = m3NeutralVariantDark;
                Theme.sideBlurColor = Qt.rgba(m3NeutralVariantDark.r, m3NeutralVariantDark.g, m3NeutralVariantDark.b, 0.58);
                Theme.hoverColor = Qt.rgba(1, 1, 1, 0.08);

                // onSurface / onSurfaceVariant
                Theme.fontColor = Qt.hsva(seedHue, 0.03, 0.92, 1.0);
                Theme.textColor = Qt.hsva(seedHue, 0.06, 0.78, 1.0);

                // Outline
                Theme.borderColor = Qt.hsva(seedHue, 0.08, 0.30, 1.0);

                // Primary Container T30（降低饱和度，避免暗背景上过艳）
                Theme.containColor = Qt.hsva(seedHue, 0.26, 0.30, 1.0);
                Theme.containOutColor = m3PrimaryDark;

                Theme.blurOverlayColor = Qt.rgba(m3NeutralDark.r, m3NeutralDark.g, m3NeutralDark.b, 0.62);
                Theme.blurSecondaryColor = Qt.rgba(m3NeutralDarkContainer.r, m3NeutralDarkContainer.g, m3NeutralDarkContainer.b, 0.62);

                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.45);
                Theme.themeShadowColor = Qt.rgba(m3PrimaryDark.r, m3PrimaryDark.g, m3PrimaryDark.b, 0.30);
            } else {
                // 浅色（保持原逻辑）
                Theme.fullColor = m3NeutralLight;
                Theme.primaryColor = m3NeutralLight;
                Theme.primaryBlurColor = Qt.rgba(m3NeutralLight.r, m3NeutralLight.g, m3NeutralLight.b, 0.72);
                Theme.secondaryColor = Qt.hsva(seedHue, 0.04, 0.94, 1.0);
                Theme.secondaryBlurColor = Qt.hsva(seedHue, 0.04, 0.94, 0.72);

                Theme.sideColor = m3NeutralVariantLight;
                Theme.sideBlurColor = Qt.rgba(m3NeutralVariantLight.r, m3NeutralVariantLight.g, m3NeutralVariantLight.b, 0.58);
                Theme.hoverColor = Qt.rgba(0, 0, 0, 0.06);

                Theme.fontColor = Qt.hsva(seedHue, 0.04, 0.12, 1.0);
                Theme.textColor = Qt.hsva(seedHue, 0.08, 0.30, 1.0);

                Theme.borderColor = Qt.hsva(seedHue, 0.08, 0.72, 1.0);

                Theme.containColor = Qt.hsva(seedHue, 0.20, 0.88, 1.0);
                Theme.containOutColor = m3PrimaryLight;

                Theme.blurOverlayColor = Qt.rgba(m3NeutralLight.r, m3NeutralLight.g, m3NeutralLight.b, 0.55);
                Theme.blurSecondaryColor = Qt.rgba(m3NeutralVariantLight.r, m3NeutralVariantLight.g, m3NeutralVariantLight.b, 0.55);

                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.10);
                Theme.themeShadowColor = Qt.rgba(m3PrimaryLight.r, m3PrimaryLight.g, m3PrimaryLight.b, 0.30);
            }
            break;
        }

        case 2:
            // Fluent WinUI 3
            if (darkis) {
                // 强调色饱和度略降，深色下不刺眼
                Theme.themeColor = Qt.hsva(hue, 0.55, 0.90, 1.0);
                Theme.fontColor = Qt.rgba(1, 1, 1, 0.92);
                Theme.textColor = Qt.rgba(1, 1, 1, 0.62);
                // 正向层级：窗口 #1F1F1F < 内容 #272727 < 卡片/浮层 #323232
                Theme.fullColor = "#272727";
                Theme.primaryColor = "#1F1F1F";
                Theme.secondaryColor = "#323232";
                Theme.sideColor = "#232323";
                Theme.hoverColor = Qt.rgba(1, 1, 1, 0.06);
                Theme.borderColor = Qt.rgba(1, 1, 1, 0.08);
                Theme.containColor = Qt.hsva(hue, 0.80, 0.62, 1.0);

                Theme.primaryBlurColor = Qt.rgba(0.122, 0.122, 0.122, 0.82);
                Theme.secondaryBlurColor = Qt.rgba(0.196, 0.196, 0.196, 0.82);
                Theme.sideBlurColor = Qt.rgba(0.137, 0.137, 0.137, 0.58);
                Theme.blurOverlayColor = Qt.rgba(0.122, 0.122, 0.122, 0.62);
                Theme.blurSecondaryColor = Qt.rgba(0.196, 0.196, 0.196, 0.62);

                Theme.containOutColor = Qt.hsva(hue, 0.55, 0.90, 1.0);
                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.40);
                Theme.themeShadowColor = Qt.rgba(0.38, 0.80, 1.0, 0.25);
            } else {
                // 浅色（保持原逻辑）
                Theme.themeColor = Qt.hsva(hue, 1.0, 0.72, 1.0);
                Theme.fontColor = Qt.rgba(0, 0, 0, 0.90);
                Theme.textColor = Qt.rgba(0, 0, 0, 0.60);
                Theme.fullColor = "#FFFFFF";
                Theme.primaryColor = "#F3F3F3";
                Theme.secondaryColor = "#F9F9F9";
                Theme.sideColor = "#EAEAEA";
                Theme.hoverColor = Qt.rgba(0, 0, 0, 0.04);
                Theme.borderColor = Qt.rgba(0, 0, 0, 0.06);
                Theme.containColor = Qt.hsva(hue, 0.62, 1.0, 1.0);

                Theme.primaryBlurColor = Qt.rgba(0.95, 0.95, 0.95, 0.72);
                Theme.secondaryBlurColor = Qt.rgba(0.98, 0.98, 0.98, 0.72);
                Theme.sideBlurColor = Qt.rgba(0.92, 0.92, 0.92, 0.58);
                Theme.blurOverlayColor = Qt.rgba(0.95, 0.95, 0.95, 0.55);
                Theme.blurSecondaryColor = Qt.rgba(0.98, 0.98, 0.98, 0.55);

                Theme.containOutColor = Qt.hsva(hue, 1.0, 0.72, 1.0);
                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.08);
                Theme.themeShadowColor = Qt.rgba(0, 0.37, 0.72, 0.20);
            }
            break;

        case 3:
            // Quiet UI
            if (darkis) {
                // 深色：背景 0.09 → 卡片 0.18 → 浮层 0.13，纠正了原来 primary 比背景还暗的问题
                Theme.themeColor = Qt.hsva(hue, 0.20, 0.68, 1.0);
                Theme.fontColor = Qt.hsva(hue, 0.04, 0.90, 1.0);
                Theme.textColor = Qt.hsva(hue, 0.06, 0.72, 1.0);
                Theme.fullColor = Qt.hsva(hue, 0.06, 0.09, 1.0);        // 基础背景（最暗）
                Theme.primaryColor = Qt.hsva(hue, 0.08, 0.18, 1.0);     // 卡片（更亮，可辨识）
                Theme.secondaryColor = Qt.hsva(hue, 0.06, 0.13, 1.0);   // 次级表面
                Theme.sideColor = Qt.hsva(hue, 0.07, 0.15, 1.0);        // 侧栏
                Theme.hoverColor = Qt.rgba(1, 1, 1, 0.06);
                Theme.borderColor = Qt.rgba(1, 1, 1, 0.10);
                Theme.containColor = Qt.hsva(hue, 0.22, 0.26, 1.0);

                Theme.primaryBlurColor = Qt.hsva(hue, 0.08, 0.18, 0.82);
                Theme.secondaryBlurColor = Qt.hsva(hue, 0.06, 0.13, 0.82);
                Theme.sideBlurColor = Qt.hsva(hue, 0.07, 0.15, 0.58);
                Theme.blurOverlayColor = Qt.hsva(hue, 0.07, 0.11, 0.62);
                Theme.blurSecondaryColor = Qt.hsva(hue, 0.06, 0.13, 0.62);

                Theme.containOutColor = Qt.hsva(hue, 0.20, 0.68, 1.0);
                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.35);
                Theme.themeShadowColor = Qt.hsva(hue, 0.20, 0.68, 0.20);
            } else {
                // 浅色（保持原逻辑）
                Theme.themeColor = Qt.hsva(hue, 0.22, 0.52, 1.0);
                Theme.fontColor = Qt.hsva(hue, 0.06, 0.16, 1.0);
                Theme.textColor = Qt.hsva(hue, 0.08, 0.42, 1.0);
                Theme.fullColor = Qt.hsva(hue, 0.02, 1.0, 1.0);
                Theme.primaryColor = Qt.hsva(hue, 0.03, 0.97, 1.0);
                Theme.secondaryColor = Qt.hsva(hue, 0.04, 0.94, 1.0);
                Theme.sideColor = Qt.hsva(hue, 0.06, 0.90, 1.0);
                Theme.hoverColor = Qt.rgba(0, 0, 0, 0.04);
                Theme.borderColor = Qt.rgba(0, 0, 0, 0.06);
                Theme.containColor = Qt.hsva(hue, 0.16, 0.88, 1.0);

                Theme.primaryBlurColor = Qt.hsva(hue, 0.03, 0.97, 0.72);
                Theme.secondaryBlurColor = Qt.hsva(hue, 0.04, 0.94, 0.72);
                Theme.sideBlurColor = Qt.hsva(hue, 0.06, 0.90, 0.58);
                Theme.blurOverlayColor = Qt.hsva(hue, 0.03, 0.97, 0.55);
                Theme.blurSecondaryColor = Qt.hsva(hue, 0.04, 0.94, 0.55);

                Theme.containOutColor = Qt.hsva(hue, 0.22, 0.52, 1.0);
                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.06);
                Theme.themeShadowColor = Qt.hsva(hue, 0.22, 0.52, 0.18);
            }
            break;

        case 4:
            // Material You
            const mySeedSat = Style.colorList[Style.color].hsvSaturation;

            // Primary：深色 T80，饱和度略低于 MD3
            const myPrimary = darkis
                ? Qt.hsva(hue, mySeedSat * 0.65, 0.82, 1.0)
                : Qt.hsva(hue, mySeedSat * 0.95, 0.44, 1.0);

            // Neutral / Surface Container（浅色）
            const myNeutralLight         = Qt.hsva(hue, 0.06, 0.98, 1.0);
            const myNeutralVariantLight  = Qt.hsva(hue, 0.09, 0.88, 1.0);
            const mySurfaceContainerL    = Qt.hsva(hue, 0.05, 0.95, 1.0);
            const mySurfaceContainerHL   = Qt.hsva(hue, 0.06, 0.92, 1.0);

            // Neutral / Surface Container（深色）——补上卡片/浮层分层
            const myNeutralDark          = Qt.hsva(hue, 0.07, 0.11, 1.0); // 背景
            const mySurfaceContainerD    = Qt.hsva(hue, 0.08, 0.16, 1.0); // 卡片
            const mySurfaceContainerHD   = Qt.hsva(hue, 0.08, 0.22, 1.0); // 浮层/悬浮
            const myNeutralVariantDark   = Qt.hsva(hue, 0.09, 0.26, 1.0); // 侧栏

            Theme.themeColor = myPrimary;

            if (darkis) {
                // 深色
                Theme.fullColor = myNeutralDark;
                Theme.primaryColor = mySurfaceContainerD;
                Theme.primaryBlurColor = Qt.rgba(mySurfaceContainerD.r, mySurfaceContainerD.g, mySurfaceContainerD.b, 0.82);
                Theme.secondaryColor = mySurfaceContainerHD;
                Theme.secondaryBlurColor = Qt.rgba(mySurfaceContainerHD.r, mySurfaceContainerHD.g, mySurfaceContainerHD.b, 0.82);

                Theme.sideColor = myNeutralVariantDark;
                Theme.sideBlurColor = Qt.rgba(myNeutralVariantDark.r, myNeutralVariantDark.g, myNeutralVariantDark.b, 0.58);
                Theme.hoverColor = Qt.rgba(1, 1, 1, 0.08);

                Theme.fontColor = Qt.hsva(hue, 0.05, 0.92, 1.0);
                Theme.textColor = Qt.hsva(hue, 0.07, 0.78, 1.0);

                Theme.borderColor = Qt.hsva(hue, 0.08, 0.32, 1.0);

                // Primary Container T30（比 MD3 稍高一点活力，但不过饱和）
                Theme.containColor = Qt.hsva(hue, 0.30, 0.32, 1.0);
                Theme.containOutColor = myPrimary;

                Theme.blurOverlayColor = Qt.rgba(myNeutralDark.r, myNeutralDark.g, myNeutralDark.b, 0.62);
                Theme.blurSecondaryColor = Qt.rgba(mySurfaceContainerHD.r, mySurfaceContainerHD.g, mySurfaceContainerHD.b, 0.62);

                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.45);
                Theme.themeShadowColor = Qt.rgba(myPrimary.r, myPrimary.g, myPrimary.b, 0.32);
            } else {
                // 浅色（保持原逻辑）
                Theme.fullColor = Qt.hsva(hue, 0.02, 0.995, 1.0);
                Theme.primaryColor = myNeutralLight;
                Theme.primaryBlurColor = Qt.rgba(myNeutralLight.r, myNeutralLight.g, myNeutralLight.b, 0.72);
                Theme.secondaryColor = mySurfaceContainerL;
                Theme.secondaryBlurColor = Qt.rgba(mySurfaceContainerL.r, mySurfaceContainerL.g, mySurfaceContainerL.b, 0.72);

                Theme.sideColor = myNeutralVariantLight;
                Theme.sideBlurColor = Qt.rgba(myNeutralVariantLight.r, myNeutralVariantLight.g, myNeutralVariantLight.b, 0.58);
                Theme.hoverColor = Qt.rgba(0, 0, 0, 0.06);

                Theme.fontColor = Qt.hsva(hue, 0.06, 0.12, 1.0);
                Theme.textColor = Qt.hsva(hue, 0.08, 0.34, 1.0);

                Theme.borderColor = Qt.hsva(hue, 0.08, 0.68, 1.0);

                Theme.containColor = Qt.hsva(hue, 0.24, 0.90, 1.0);
                Theme.containOutColor = myPrimary;

                Theme.blurOverlayColor = Qt.rgba(myNeutralLight.r, myNeutralLight.g, myNeutralLight.b, 0.55);
                Theme.blurSecondaryColor = Qt.rgba(mySurfaceContainerHL.r, mySurfaceContainerHL.g, mySurfaceContainerHL.b, 0.55);

                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.10);
                Theme.themeShadowColor = Qt.rgba(myPrimary.r, myPrimary.g, myPrimary.b, 0.28);
            }
            break;
        }
    }
}
