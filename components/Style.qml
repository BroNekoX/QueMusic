// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QtCore

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
        var baseColor = settings.colorList[settings.color];
        themes.fontColor = darkis ? "#f6f6f8" : "#1d1d1f";
        themes.textColor = darkis ? "#cfd0d4" : "#4d4f56";
        themes.fullColor = darkis ? "#0c0d10" : "#ffffff";
        themes.hoverColor = darkis ? "#14ffffff" : "#0c000000";
        themes.sideColor = darkis ?  "#484848" : "#eaeaea";
        themes.sideBlurColor = darkis ? "#88484848" : "#88eaeaea";

        themes.containColor = darkis ? Qt.hsva(baseColor.hsvHue,0.9,0.4,1.0) : Qt.hsva(baseColor.hsvHue,0.2,1.0,1.0);
        themes.containOutColor = darkis ? Qt.hsva(baseColor.hsvHue,0.2,1.0,1.0) : Qt.hsva(baseColor.hsvHue,0.9,0.4,1.0);
        themes.themeColor = baseColor;

        themes.primaryColor = darkis ? Qt.hsva(baseColor.hsvHue,0.12,0.16,1.0) : Qt.hsva(baseColor.hsvHue,0.01,1.0,1.0);
        themes.primaryBlurColor = darkis ? Qt.hsva(baseColor.hsvHue,0.12,0.16,0.8) : Qt.hsva(baseColor.hsvHue,0.01,1.0,0.7);
        themes.secondaryColor = darkis ? Qt.hsva(baseColor.hsvHue,0.1,0.12,1.0) : Qt.hsva(baseColor.hsvHue,0.02,0.97,1.0);
        themes.secondaryBlurColor = darkis ? Qt.hsva(baseColor.hsvHue,0.1,0.12,0.8) : Qt.hsva(baseColor.hsvHue,0.02,0.97,0.7);
        themes.borderColor = darkis ? Qt.hsva(baseColor.hsvHue,0.1,0.2,1.0) : Qt.hsva(baseColor.hsvHue,0.02,0.97,1.0);
        themes.blurOverlayColor = darkis ? Qt.hsva(baseColor.hsvHue,0.1,0.12,0.6) : Qt.hsva(baseColor.hsvHue,0.01,1.0,0.5);
        themes.blurSecondaryColor = darkis ? Qt.hsva(baseColor.hsvHue,0.12,0.18,0.6) : Qt.hsva(baseColor.hsvHue,0.02,0.97,0.5);
        themes.shadowColor = darkis ? Qt.hsva(baseColor.hsvHue,1.0,0.05,0.2) : Qt.hsva(baseColor.hsvHue,1.0,0.12,0.1);
        themes.themeShadowColor = darkis ? Qt.hsva(baseColor.hsvHue,1.0,0.5,0.3) : Qt.hsva(baseColor.hsvHue,1.0,0.6,0.3);
    }

    // 主题色板同样抽成独立的 StyleThemes 类型（详见 StyleThemes.qml 说明）
    property StyleThemes themes: StyleThemes {}
}
