// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QtCore
import QueMusic 1.0

QtObject {
    // Style.settings.name
    // Style.themes.name
    readonly property bool darkis: settings.theme === 0 ? false : settings.theme === 1 ? true : Qt.application.styleHints.colorScheme === Qt.ColorScheme.Dark
    //全局过渡动画时长：关闭高级动画时统一降为轻量时长
    readonly property int animeDuration: settings.premiumAnime
        ? (settings.animeSpeed === 0 ? 480 : settings.animeSpeed === 2 ? 180 : 320)
        : 120
    //readonly property var themes: darkis ? darkThemes[settings.color] : lightThemes[settings.color]
    onDarkisChanged: {
        Style.changeTheme();
    }

    // 配置存储，后续也可以存储在服务器数据库中
    property StyleSettings settings: StyleSettings {}

    signal changeUi()
    signal changeTheme()
    signal warned(string text,int type)
    onChangeTheme: {
        const hue = settings.colorList[settings.color].hsvHue;
        themes.fontColor = darkis ? "#f5f5f7" : "#1d1d1f";
        themes.textColor = darkis ? "#bebec2" : "#5e5e61";
        themes.fullColor = darkis ? "#1c1c1e" : "#ffffff";
        themes.hoverColor = darkis ? "#10ffffff" : "#0a000000";
        themes.sideColor = darkis ?  "#3a3a3e" : "#e9e9ee";
        themes.sideBlurColor = darkis ? "#953a3a3e" : "#95e9e9ee";

        themes.containColor = darkis ? Qt.hsva(hue,0.55,0.38,1.0) : Qt.hsva(hue,0.14,1.0,1.0);
        themes.containOutColor = darkis ? Qt.hsva(hue,0.2,1.0,1.0) : Qt.hsva(hue,0.9,0.4,1.0);
        themes.themeColor = settings.colorList[settings.color];

        themes.primaryColor = darkis ? Qt.hsva(hue,0.10,0.17,1.0) : Qt.hsva(hue,0.01,1.0,1.0);
        themes.primaryBlurColor = darkis ? Qt.hsva(hue,0.10,0.17,0.82) : Qt.hsva(hue,0.01,1.0,0.72);
        themes.secondaryColor = darkis ? Qt.hsva(hue,0.10,0.12,1.0) : Qt.hsva(hue,0.015,0.973,1.0);
        themes.secondaryBlurColor = darkis ? Qt.hsva(hue,0.10,0.12,0.82) : Qt.hsva(hue,0.015,0.973,0.72);
        themes.borderColor = darkis ? Qt.hsva(hue,0.08,0.24,1.0) : Qt.hsva(hue,0.02,0.97,1.0);
        themes.blurOverlayColor = darkis ? Qt.hsva(hue,0.08,0.14,0.62) : Qt.hsva(hue,0.01,1.0,0.55);
        themes.blurSecondaryColor = darkis ? Qt.hsva(hue,0.10,0.16,0.62) : Qt.hsva(hue,0.02,0.97,0.55);
        themes.shadowColor = darkis ? Qt.hsva(hue,0.9,0.03,0.3) : Qt.hsva(hue,0.9,0.2,0.1);
        themes.themeShadowColor = darkis ? Qt.hsva(hue,1.0,0.5,0.3) : Qt.hsva(hue,1.0,0.6,0.3);
    }

    // 主题色板同样抽成独立的 StyleThemes 类型（详见 StyleThemes.qml 说明）
    property StyleThemes themes: StyleThemes {}
}
