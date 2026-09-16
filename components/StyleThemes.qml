// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 主题色板类型。
import QtQuick

QtObject {
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
    property color borderColor: "#f3f5f8"
    property color secondaryColor: "#f3f5f8"
    property color secondaryBlurColor: "#c4f3f5f8"
    property color blurOverlayColor: "#88fdfdff"
    property color blurSecondaryColor: "#88f3f5f8"
    property color shadowColor: "#31001020"
    property color themeShadowColor: "#660f5888"
}
