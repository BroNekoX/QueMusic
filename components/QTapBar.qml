// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

Item {
    id: tabs
    height: 32
    property int spacing: 12
    property int index: 0
    property list<string> model: []
    Component.onCompleted: tabsBar.width = tabsRepeat.itemAt(0).width;
    onIndexChanged: barAnime.running = true;
    Rectangle {
        id: tabsBar
        anchors.bottom: parent.bottom
        x: 0
        height: 2
        radius: 1
        color: Theme.themeColor
    }
    ParallelAnimation {
        id: barAnime
        NumberAnimation {
            target: tabsBar
            easing.overshoot: 1.2
            property: "x"
            to: tabsRepeat.count > 0 ? tabsRepeat.itemAt(tabs.index).x : 0
            duration: 320
            easing.type: Easing.OutBack
        }
        NumberAnimation {
            target: tabsBar
            property: "width"
            easing.overshoot: 1.2
            to: tabsRepeat.count > 0 ? tabsRepeat.itemAt(tabs.index).width : 0
            duration: 320
            easing.type: Easing.OutBack
        }
    }

    Row {
        spacing: tabs.spacing
        anchors.fill: parent
        Repeater {
            id: tabsRepeat
            model: tabs.model
            delegate: Item {
                id: tabItem
                required property int index
                required property string modelData
                width: tabLabel.implicitWidth + 12
                height: tabs.height
                Text {
                    id: tabLabel
                    anchors.centerIn: parent
                    text: tabItem.modelData
                    font.pixelSize: Style.textmain
                    font.bold: tabs.index === tabItem.index
                    color: tabs.index === tabItem.index ? Theme.themeColor : Theme.textColor
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        barAnime.running = false;
                        tabs.index = tabItem.index;
                    }
                }
            }
        }
    }
}