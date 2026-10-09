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

    readonly property bool darkis: Style.theme === 0 ? false : Style.theme === 1 ? true : Qt.application.styleHints.colorScheme === Qt.ColorScheme.Dark
    onDarkisChanged: Options.changeTheme()

    onChangeTheme: {
        const hue = Style.colorList[Style.color].hsvHue;
        Theme.fontColor = darkis ? "#f5f5f7" : "#1d1d1f";
        Theme.textColor = darkis ? "#bebec2" : "#5e5e61";
        Theme.fullColor = darkis ? "#1c1c1e" : "#ffffff";
        Theme.hoverColor = darkis ? "#10ffffff" : "#0a000000";
        Theme.sideColor = darkis ? "#4a4a4e" : "#e9e9ee";
        Theme.sideBlurColor = darkis ? "#954a4a4e" : "#95e9e9ee";

        Theme.containColor = darkis ? Qt.hsva(hue, 0.55, 0.38, 1.0) : Qt.hsva(hue, 0.14, 1.0, 1.0);
        Theme.containOutColor = darkis ? Qt.hsva(hue, 0.2, 1.0, 1.0) : Qt.hsva(hue, 0.9, 0.4, 1.0);
        Theme.themeColor = Style.colorList[Style.color];

        Theme.primaryColor = darkis ? Qt.hsva(hue, 0.10, 0.17, 1.0) : Qt.hsva(hue, 0.01, 1.0, 1.0);
        Theme.primaryBlurColor = darkis ? Qt.hsva(hue, 0.10, 0.17, 0.82) : Qt.hsva(hue, 0.01, 1.0, 0.72);
        Theme.secondaryColor = darkis ? Qt.hsva(hue, 0.10, 0.12, 1.0) : Qt.hsva(hue, 0.015, 0.973, 1.0);
        Theme.secondaryBlurColor = darkis ? Qt.hsva(hue, 0.10, 0.12, 0.82) : Qt.hsva(hue, 0.015, 0.973, 0.72);
        Theme.borderColor = darkis ? Qt.hsva(hue, 0.08, 0.24, 1.0) : Qt.hsva(hue, 0.02, 0.97, 1.0);
        Theme.blurOverlayColor = darkis ? Qt.hsva(hue, 0.08, 0.14, 0.62) : Qt.hsva(hue, 0.01, 1.0, 0.55);
        Theme.blurSecondaryColor = darkis ? Qt.hsva(hue, 0.10, 0.16, 0.62) : Qt.hsva(hue, 0.02, 0.97, 0.55);
        Theme.shadowColor = darkis ? Qt.hsva(hue, 0.9, 0.03, 0.3) : Qt.hsva(hue, 0.9, 0.2, 0.1);
        Theme.themeShadowColor = darkis ? Qt.hsva(hue, 1.0, 0.5, 0.3) : Qt.hsva(hue, 1.0, 0.6, 0.3);
    }
}
