// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
import 'qrc:/QueMusic/components'

// 左侧边栏
Rectangle {
    z: 1
    id: sidebar
    width: 210
    property color baseColor: "transparent"
    property color choiceColor: Style.themes.hoverColor
    property color choiceTextColor: Style.themes.fontColor
    color: Style.settings.sidebarColor ? Style.themes.secondaryBlurColor : baseColor
    //layer.enabled: true
    //layer.smooth: true
    Connections {
        target: Style
        function onChangeTheme() {
            if(Style.settings.sidebarStyle === 0) {
                sidebar.choiceColor = Style.themes.hoverColor
                sidebar.choiceTextColor = Style.themes.fontColor
                choicebar.x = 16
                choicebar.radius = 2
            } else if(Style.settings.sidebarStyle === 1) {
                sidebar.choiceColor = Style.themes.themeColor
                sidebar.choiceTextColor = Style.themes.primaryColor
                choicebar.x = 0
                choicebar.radius = 0
            }
        }
    }

    function indexed(choice) {
        choicebar.willBarY = choice > 2 ? 44 * choice + 68 : 44 * choice + 80;
        navlistview.choiceIndex = choice;
        if(choice > choicebar.indexOld) {
            downBar.stop();
            upBar.stop();
            downBar.running = true;
        } else if(choice < choicebar.indexOld) {
            upBar.stop();
            downBar.stop();
            upBar.running = true;
        } else {
            window.exit();
        }
        choicebar.indexOld = choice;
        switch(choice) {
        case 0:
            break;
        case 1:
            if(MusicApi.recommendSongs.count === 0) MusicApi.getRecommendSongs(1,24);
            break;
        }
    }

    Connections {
        target: window
        function onExit() {
            if(mainContent.pageIndex === 6 && window.exitIndex <= 1 ) {
                mainContent.contentIndexed(navlistview.choiceIndex)
                MusicApi.searchSongsResults.clear()
            }
        }
    }

    // 选择动画条
    Rectangle {
        id: choicebar
        x: 16
        width: 4
        height: barBottom - y//22
        radius: 2 //Style.settings.labelRadius
        topRightRadius: 2
        bottomRightRadius: 2
        color: Style.themes.themeColor
        y: 80
        property int barBottom: 102
        property int willBarY: 80
        property int indexOld: 0

        ParallelAnimation {
            id: downBar
            NumberAnimation {
                property: "barBottom"
                target: choicebar
                //from: choicebar.barBottom
                to: choicebar.willBarY + 22
                easing.type: Easing.Bezier
                easing.bezierCurve: [ 0.50, 0.00, 0.00, 1.00, 1, 1 ]
                duration: 280
            }
            NumberAnimation {
                property: "y"
                target: choicebar
                //from: choicebar.y
                to: choicebar.willBarY
                easing.type: Easing.Bezier
                easing.bezierCurve: [ 1.00, 0.00, 0.50, 1.00, 1, 1 ]
                duration: 280
            }
        }
        ParallelAnimation {
            id: upBar
            NumberAnimation {
                property: "barBottom"
                target: choicebar
                //from: choicebar.barBottom
                to: choicebar.willBarY + 22
                easing.type: Easing.Bezier
                easing.bezierCurve: [ 1.00, 0.00, 0.50, 1.00, 1, 1 ]
                duration: 280
            }
            NumberAnimation {
                property: "y"
                target: choicebar
                //from: choicebar.y
                to: choicebar.willBarY
                easing.type: Easing.Bezier
                easing.bezierCurve: [ 0.50, 0.00, 0.00, 1.00, 1, 1 ]
                duration: 280
            }
        }
    }

    //分隔条
    Rectangle {
        x: 20
        width: 170
        height: 2
        color: Qt.rgba(0.6,0.6,0.6,0.3)
        y: 173
    }


    // Sidebar Header/Section Title
    Item {
        x: 0
        y: 0
        width: 200
        height: 60

        //icon
        Image {
            y: 18
            x: 25
            width: 24
            height: 24
            source: "qrc:/QueMusic/resources/icon.ico"
            sourceSize: Qt.size(24, 24)
        }

        // App Title
        Text {
            y: 21
            x: 61
            height: 18
            text: "QueMusic"
            font.family: textFont.name
            font.pixelSize: 16
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            color: Style.themes.fontColor

        }

        Rectangle {
            x: 150
            y: 20
            width: 40
            height: 20
            color: Style.themes.themeColor
            radius: 6
            Text {
                anchors.centerIn: parent
                text: "Beta"
                font.pixelSize: 12
                color:  Style.themes.primaryColor

            }
        }
    }

    // Navigation List
    ListModel {
        id: navModel
        ListElement { display: "推荐"; iconChar: "\uf0bf" } // tj
        ListElement { display: "分类"; iconChar: "\uf044" }  // fl
        ListElement { display: ""; iconChar: "" }     // empty
        ListElement { display: "收藏"; iconChar: "\uf0c1" }  // sc
        ListElement { display: "本地"; iconChar: "\uf0f5" }   // bd
        ListElement { display: "下载"; iconChar: "\uf00f" }   // xz
    }

    Column {
        id: navlistview
        x: 10
        y: 70
        width: 190
        height: sidebar.height - 78
        property int choiceIndex: 0

        spacing: 2
        z: 10
        Repeater {
            model: navModel

            delegate: Rectangle {
                id: navDelegate
                width: navlistview.width
                height: 42
                radius: Style.settings.labelRadius
                Component.onCompleted: {
                    if (index === 2) height = 30
                }
                color: isSelected ? sidebar.choiceColor : "transparent"

                readonly property bool isSelected: navlistview.choiceIndex === index

                scale: 1.0
                Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
                Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutCubic } }


                // Hover Background (fades in/out)
                Rectangle {
                    anchors.fill: parent
                    radius: Style.settings.labelRadius
                    color: Style.themes.hoverColor
                    opacity: barMouse.containsMouse ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 80 } }
                }

                Text {
                    x: 12
                    y: 0
                    z: 1
                    width: 42
                    height: 42
                    text: model.iconChar
                    font.family: iconFont.name
                    font.pixelSize: Style.settings.texticon
                    color: navDelegate.isSelected ? sidebar.choiceTextColor : Style.themes.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    Component.onCompleted: if (index === 2) visible = false
                    Behavior on color { ColorAnimation { duration: 120 } }
                }

                Text {
                    x: 60
                    y: 0
                    z: 2
                    width: 140
                    height: 42
                    text: model.display
                    color: navDelegate.isSelected ? sidebar.choiceTextColor : Style.themes.textColor
                    font.bold: navDelegate.isSelected
                    font.pixelSize: Style.settings.textmain
                    verticalAlignment: Text.AlignVCenter
                    Component.onCompleted: if (navDelegate.itemIndex === 2) visible = false
                    Behavior on color { ColorAnimation { duration: 120 } }
                }


                MouseArea {
                    id: barMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    Component.onCompleted: {
                        if(index == 2) visible = false
                    }
                    onPressed: navDelegate.scale = 0.96
                    onReleased: navDelegate.scale = 1.0
                    onCanceled: navDelegate.scale = 1.0
                    onClicked: {
                        sidebar.indexed(index);
                        mainContent.contentIndexed(index);
                    }
                }
            }
        }
    }
}
