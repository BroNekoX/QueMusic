// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick

Item {
    id: root
    property list<Item> pageList: []
    property int lastIndex: 0
    function stack(index: int): void {
        pageInAnime.running = false;
        pageOutAnime.running = false;
        pageInAnime.target = root.pageList[index];
        pageOutAnime.target = root.pageList[root.lastIndex];
        pageOutAnime.running = true;
        pageInAnime.running = true;
        root.pageList[index].visible = true;
        root.lastIndex = index;
    }
    
    ParallelAnimation {
        id: pageInAnime
        property Item target
        NumberAnimation {
            property: "opacity"
            target: pageInAnime.target
            from: 0
            to: 1
            duration: 360
            easing.type: Easing.OutExpo
        }
        NumberAnimation {
            property: "scale"
            target: pageInAnime.target
            from: 0.8
            to: 1.0
            duration: 360
            easing.type: Easing.OutExpo
        }
    }
    ParallelAnimation {
        id: pageOutAnime
        property Item target
        NumberAnimation {
            property: "opacity"
            target: pageOutAnime.target
            to: 0
            duration: 120
        }
        NumberAnimation {
            property: "scale"
            target: pageOutAnime.target
            to: 0.9
            duration: 120
        }
        onFinished: target.visible = false;
    }

}