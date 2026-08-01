// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Dialogs
import Qt5Compat.GraphicalEffects
import QPlayer 1.0

Item {
    id: fullCenterView
    padding: 24
    Item {
        id: centerTopBar
        x: 0
        y: 0
        width: fullCenterView.width
        height: 80
        QBlurTapBar {
            anchors.centerIn: centerTopBar
            model: ["首页","歌单","收藏","本地","下载"]
            tabWidth: 70
            width: 354
            //rectXy: Qt.rect(0, 10, width, 40)
            //blurSource: fileMain.foldIndex === 0 ? myFile : localFile
            onTabChange:  (index) => {
        }
    }
    }
}