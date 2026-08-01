// QPages.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root
    property var pageList: []
    property int lastIndex: 0
    function stack(index) {
        pageInAnime.stop()
        opacityAnime.target = root.pageList[index]
        yAnime.target = root.pageList[index]
        pageInAnime.start()
        root.pageList[root.lastIndex].visible = false
        root.pageList[index].visible = true
        root.lastIndex = index
    }
    
    ParallelAnimation {
        id: pageInAnime
        NumberAnimation {
            id: opacityAnime
            property: "opacity"
            from: 0
            to: 1
            duration: 360
            easing.type: Easing.OutExpo
        }
        NumberAnimation {
            id: yAnime
            property: "scale"
            from: 0.9
            to: 1.0
            duration: 360
            easing.type: Easing.OutExpo
        }
    }

}