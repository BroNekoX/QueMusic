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
    property FavoriteFolderDialog favPicker
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
    readonly property string version: "0.7.0"
    readonly property int versionCode: 70
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
        // 深色角色：secondaryColor=页面/侧栏，primaryColor=卡片/弹层（同时作为强调色上的前景），
        // fullColor=最深表面；卡片必须比页面亮，否则层级会塌陷。
        const hue = Style.colorList[Style.color].hsvHue;
        const seedSat = Style.colorList[Style.color].hsvSaturation;
        switch(Style.colorStyle) {
        case 0:
            Theme.fontColor = darkis ? "#e9eaee" : "#1d1d1f";
            Theme.textColor = darkis ? "#a6abb6" : "#5e5e61";
            Theme.fullColor = darkis ? Qt.hsva(hue, 0.04, 0.065, 1.0) : "#ffffff";
            Theme.hoverColor = darkis ? "#12ffffff" : "#0a000000";
            Theme.sideColor = darkis ? Qt.hsva(hue, 0.05, 0.26, 1.0) : "#e9e9ee";
            Theme.sideBlurColor = darkis ? Qt.hsva(hue, 0.05, 0.26, 0.55) : "#95e9e9ee";

            // 深色：页面 < 卡片 < 分割线；强调色取亮色，保证选中项用深色文字时仍有对比
            Theme.containColor = darkis ? Qt.hsva(hue, seedSat * 0.46, 0.29, 1.0) : Qt.hsva(hue, 0.14, 1.0, 1.0);
            Theme.containOutColor = darkis ? Qt.hsva(hue, seedSat * 0.63, 0.90, 1.0) : Qt.hsva(hue, 0.9, 0.4, 1.0);
            Theme.themeColor = darkis ? Qt.hsva(hue, Math.min(seedSat * 0.63, 0.55), 0.96, 1.0) : Style.colorList[Style.color];

            Theme.primaryColor = darkis ? Qt.hsva(hue, 0.06, 0.185, 1.0) : Qt.hsva(hue, 0.01, 1.0, 1.0);
            Theme.primaryBlurColor = darkis ? Qt.hsva(hue, 0.06, 0.185, 0.82) : Qt.hsva(hue, 0.01, 1.0, 0.72);
            Theme.secondaryColor = darkis ? Qt.hsva(hue, 0.05, 0.115, 1.0) : Qt.hsva(hue, 0.015, 0.973, 1.0);
            Theme.secondaryBlurColor = darkis ? Qt.hsva(hue, 0.05, 0.115, 0.82) : Qt.hsva(hue, 0.015, 0.973, 0.72);
            Theme.borderColor = darkis ? "#18ffffff" : Qt.hsva(hue, 0.02, 0.97, 1.0);
            Theme.blurOverlayColor = darkis ? Qt.hsva(hue, 0.05, 0.115, 0.65) : Qt.hsva(hue, 0.01, 1.0, 0.55);
            Theme.blurSecondaryColor = darkis ? Qt.hsva(hue, 0.06, 0.185, 0.62) : Qt.hsva(hue, 0.02, 0.97, 0.55);
            Theme.shadowColor = darkis ? Qt.rgba(0, 0, 0, 0.42) : Qt.hsva(hue, 0.9, 0.2, 0.1);
            Theme.themeShadowColor = darkis ? Qt.hsva(hue, Math.min(seedSat * 0.63, 0.55), 0.96, 0.22) : Qt.hsva(hue, 1.0, 0.6, 0.3);
            break;

        case 1: {
            // Material 3
            const seed = Style.colorList[Style.color];
            const seedHue = seed.hsvHue;
            const seedSat = seed.hsvSaturation;

            // M3 强调色：浅色 T40，深色 T80
            const m3PrimaryLight = Qt.hsva(seedHue, seedSat * 0.9, 0.40, 1.0);
            const m3PrimaryDark  = Qt.hsva(seedHue, seedSat * 0.35, 0.96, 1.0);
            const m3PrimaryContainerDark = Qt.hsva(seedHue, seedSat * 0.50, 0.30, 1.0);

            // Neutral 色（浅色）
            const m3NeutralLight        = Qt.hsva(seedHue, 0.04, 0.97, 1.0);
            const m3NeutralVariantLight = Qt.hsva(seedHue, 0.08, 0.90, 1.0);

            // 深色使用 M3 tonal 层级：Surface(页面) < SurfaceContainer(卡片) < OutlineVariant(分隔)
            const m3SurfaceDark          = Qt.hsva(seedHue, 0.03, 0.075, 1.0); // Surface
            const m3SurfaceContainerDark = Qt.hsva(seedHue, 0.04, 0.14, 1.0);  // SurfaceContainer
            const m3SurfaceLowestDark    = Qt.hsva(seedHue, 0.03, 0.05, 1.0);   // SurfaceContainerLowest
            const m3OutlineVariantDark   = Qt.hsva(seedHue, 0.06, 0.27, 1.0);   // OutlineVariant

            Theme.themeColor = darkis ? m3PrimaryDark : m3PrimaryLight;

            if (darkis) {
                Theme.fullColor = m3SurfaceLowestDark;
                Theme.primaryColor = m3SurfaceContainerDark;
                Theme.primaryBlurColor = Qt.rgba(m3SurfaceContainerDark.r, m3SurfaceContainerDark.g, m3SurfaceContainerDark.b, 0.82);
                Theme.secondaryColor = m3SurfaceDark;
                Theme.secondaryBlurColor = Qt.rgba(m3SurfaceDark.r, m3SurfaceDark.g, m3SurfaceDark.b, 0.82);

                Theme.sideColor = m3OutlineVariantDark;
                Theme.sideBlurColor = Qt.rgba(m3OutlineVariantDark.r, m3OutlineVariantDark.g, m3OutlineVariantDark.b, 0.58);
                Theme.hoverColor = Qt.rgba(1, 1, 1, 0.08);

                // onSurface / onSurfaceVariant
                Theme.fontColor = Qt.hsva(seedHue, 0.03, 0.91, 1.0);
                Theme.textColor = Qt.hsva(seedHue, 0.06, 0.78, 1.0);

                // Outline
                Theme.borderColor = Qt.hsva(seedHue, 0.06, 0.36, 1.0);

                // PrimaryContainer T30 / onPrimaryContainer T90
                Theme.containColor = m3PrimaryContainerDark;
                Theme.containOutColor = Qt.hsva(seedHue, seedSat * 0.18, 0.90, 1.0);

                Theme.blurOverlayColor = Qt.rgba(m3SurfaceDark.r, m3SurfaceDark.g, m3SurfaceDark.b, 0.62);
                Theme.blurSecondaryColor = Qt.rgba(m3SurfaceContainerDark.r, m3SurfaceContainerDark.g, m3SurfaceContainerDark.b, 0.62);

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
                // WinUI 3 深色层：Deep #181818 < Base #202020 < Card #2C2C2C
                const accent = Qt.hsva(hue, 0.52, 0.96, 1.0);
                Theme.themeColor = accent;
                Theme.fontColor = Qt.rgba(1, 1, 1, 0.94);
                Theme.textColor = Qt.rgba(1, 1, 1, 0.74);
                Theme.fullColor = "#181818";
                Theme.primaryColor = "#2C2C2C";
                Theme.secondaryColor = "#202020";
                Theme.sideColor = "#2A2A2A";
                Theme.hoverColor = Qt.rgba(1, 1, 1, 0.06);
                Theme.borderColor = Qt.rgba(1, 1, 1, 0.08);
                Theme.containColor = Qt.hsva(hue, 0.38, 0.28, 1.0);

                Theme.primaryBlurColor = Qt.rgba(0.173, 0.173, 0.173, 0.82);
                Theme.secondaryBlurColor = Qt.rgba(0.125, 0.125, 0.125, 0.82);
                Theme.sideBlurColor = Qt.rgba(0.165, 0.165, 0.165, 0.58);
                Theme.blurOverlayColor = Qt.rgba(0.125, 0.125, 0.125, 0.62);
                Theme.blurSecondaryColor = Qt.rgba(0.173, 0.173, 0.173, 0.62);

                Theme.containOutColor = accent;
                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.35);
                Theme.themeShadowColor = Qt.rgba(accent.r, accent.g, accent.b, 0.22);
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
                // Quiet：低饱和、低对比；页面 0.105 < 卡片 0.16 < 分隔 0.21
                Theme.themeColor = Qt.hsva(hue, 0.20, 0.78, 1.0);
                Theme.fontColor = Qt.hsva(hue, 0.04, 0.90, 1.0);
                Theme.textColor = Qt.hsva(hue, 0.06, 0.70, 1.0);
                Theme.fullColor = Qt.hsva(hue, 0.04, 0.075, 1.0);
                Theme.primaryColor = Qt.hsva(hue, 0.06, 0.16, 1.0);
                Theme.secondaryColor = Qt.hsva(hue, 0.05, 0.105, 1.0);
                Theme.sideColor = Qt.hsva(hue, 0.06, 0.21, 1.0);
                Theme.hoverColor = Qt.rgba(1, 1, 1, 0.05);
                Theme.borderColor = Qt.rgba(1, 1, 1, 0.08);
                Theme.containColor = Qt.hsva(hue, 0.20, 0.25, 1.0);

                Theme.primaryBlurColor = Qt.hsva(hue, 0.06, 0.16, 0.82);
                Theme.secondaryBlurColor = Qt.hsva(hue, 0.05, 0.105, 0.82);
                Theme.sideBlurColor = Qt.hsva(hue, 0.06, 0.21, 0.58);
                Theme.blurOverlayColor = Qt.hsva(hue, 0.05, 0.105, 0.62);
                Theme.blurSecondaryColor = Qt.hsva(hue, 0.06, 0.16, 0.62);

                Theme.containOutColor = Qt.hsva(hue, 0.18, 0.78, 1.0);
                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.32);
                Theme.themeShadowColor = Qt.hsva(hue, 0.20, 0.78, 0.18);
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

            // Primary：深色 T80，保留比 MD3 略高的 seed 色相存在感
            const myPrimary = darkis
                ? Qt.hsva(hue, mySeedSat * 0.42, 0.96, 1.0)
                : Qt.hsva(hue, mySeedSat * 0.95, 0.44, 1.0);

            // Neutral / Surface Container（浅色）
            const myNeutralLight         = Qt.hsva(hue, 0.06, 0.98, 1.0);
            const myNeutralVariantLight  = Qt.hsva(hue, 0.09, 0.88, 1.0);
            const mySurfaceContainerL    = Qt.hsva(hue, 0.05, 0.95, 1.0);
            const mySurfaceContainerHL   = Qt.hsva(hue, 0.06, 0.92, 1.0);

            // 深色：页面 < 卡片 < 分隔；表面带 seed 色相
            const myNeutralDark          = Qt.hsva(hue, 0.06, 0.085, 1.0); // 页面
            const mySurfaceContainerD    = Qt.hsva(hue, 0.07, 0.155, 1.0); // 卡片
            const mySurfaceLowestD       = Qt.hsva(hue, 0.05, 0.06, 1.0);  // 最深表面
            const myNeutralVariantDark   = Qt.hsva(hue, 0.09, 0.29, 1.0);  // 侧栏/分隔

            Theme.themeColor = myPrimary;

            if (darkis) {
                Theme.fullColor = mySurfaceLowestD;
                Theme.primaryColor = mySurfaceContainerD;
                Theme.primaryBlurColor = Qt.rgba(mySurfaceContainerD.r, mySurfaceContainerD.g, mySurfaceContainerD.b, 0.82);
                Theme.secondaryColor = myNeutralDark;
                Theme.secondaryBlurColor = Qt.rgba(myNeutralDark.r, myNeutralDark.g, myNeutralDark.b, 0.82);

                Theme.sideColor = myNeutralVariantDark;
                Theme.sideBlurColor = Qt.rgba(myNeutralVariantDark.r, myNeutralVariantDark.g, myNeutralVariantDark.b, 0.58);
                Theme.hoverColor = Qt.rgba(1, 1, 1, 0.08);

                Theme.fontColor = Qt.hsva(hue, 0.04, 0.92, 1.0);
                Theme.textColor = Qt.hsva(hue, 0.07, 0.80, 1.0);

                Theme.borderColor = Qt.hsva(hue, 0.08, 0.39, 1.0);

                // PrimaryContainer T30
                Theme.containColor = Qt.hsva(hue, mySeedSat * 0.52, 0.31, 1.0);
                Theme.containOutColor = Qt.hsva(hue, mySeedSat * 0.20, 0.91, 1.0);

                Theme.blurOverlayColor = Qt.rgba(myNeutralDark.r, myNeutralDark.g, myNeutralDark.b, 0.62);
                Theme.blurSecondaryColor = Qt.rgba(mySurfaceContainerD.r, mySurfaceContainerD.g, mySurfaceContainerD.b, 0.62);

                Theme.shadowColor = Qt.rgba(0, 0, 0, 0.45);
                Theme.themeShadowColor = Qt.rgba(myPrimary.r, myPrimary.g, myPrimary.b, 0.30);
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
