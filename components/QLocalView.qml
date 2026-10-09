// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QueMusic 1.0

ListView {
    id: view
    topMargin: 6
    bottomMargin: 24
    rightMargin: 16
    acceptedButtons: Qt.NoButton
    property int scrollToY: view.contentY
    property list<string> headerModel: ["标题","歌手","","菜单"]
    property list<string> menuModel: ["立即播放","下一首播放","添加到播放列表","音频详情","编辑元数据","移除该音乐"]
    property list<int> selectedIndices: []
    readonly property var selectedSet: new Set(selectedIndices)
    property int artistX: width / 2 - 32
    property int toolX: width - 210
    property string toolText0: "\uf095"
    property string toolText1: "\uf0c8"
    property alias menu: menu
    property bool isEnd: false
    contentWidth: view.width - 16
    synchronousDrag: true
    reuseItems: true
    onDraggingChanged: view.scrollToY = view.contentY

    // 回到顶部并同步 scrollToY，防止滚轮动画把 contentY 拉回过期位置
    function scrollTop(): void {
        listViewAnime.stop();
        scrollToY = view.originY - view.topMargin;
        contentY = view.originY - view.topMargin;
    }
    signal clicked(int index)
    signal menuClicked(int index,int choice)
    signal toolClicked(int index,int tool)//从右往左2（菜单)，1（喜欢），0（通用）
    signal ended()

    onAtYEndChanged: {
        if (atYEnd && !MusicApi.loadState) ended();
    }
    footer: Item {
        width: view.width
        height: 32
        visible: view.isEnd
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            text: "没有更多了~"
            color: Theme.textColor
            font.pixelSize: Style.text
        }
    }
    Menu {
        id: menu
        title: "Menu"
        parent: Overlay.overlay
        property int index

        background: Rectangle {
            implicitWidth: 160
            implicitHeight: 40
            color: Theme.primaryColor
            radius: Style.labelRadius
            RectangularShadow {
                anchors.fill: parent
                z: -1
                offset.x: 0
                offset.y: 5
                radius: parent.radius
                blur: 20
                spread: 0
                color: Theme.shadowColor
            }
        }

        Instantiator {
            model: view.menuModel
            delegate: MenuItem {
                id: menuItem
                required property string modelData
                required property int index
                background: Rectangle {
                    implicitWidth: 146
                    implicitHeight: 36
                    x: 2
                    y: 2
                    radius: Style.labelRadius - 2
                    width: menuItem.width - 4
                    height: menuItem.height - 4
                    color: menuItem.down || menuItem.highlighted ? Theme.hoverColor : "transparent"
                }
                text: modelData
                contentItem: Text {
                    text: menuItem.text
                    color: Theme.fontColor//使用项目主题文字色，深浅色主题下都可读
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                    elide: Text.ElideRight//保证超长歌手名不会撑破菜单项
                }
                onTriggered: view.menuClicked(menu.index,index)
            }
            onObjectAdded: (i, obj) => menu.insertItem(i, obj)
            onObjectRemoved: (i, obj) => menu.removeItem(obj)
        }


        enter: Transition {
            NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1 }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; duration: 120; to: 0 }
        }
    }

    ScrollBar.vertical: ScrollBar {
        id: viewBar
        parent: view
        anchors.top: view.top
        anchors.right: view.right
        anchors.bottom: view.bottom
        onPressedChanged: {
            view.scrollToY = view.contentY
        }
    }
    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        readonly property real wheelHeightCount: Qt.application.styleHints.wheelScrollLines * 0.25
        readonly property int scrollBottom: view.contentHeight - view.height + view.bottomMargin + view.originY
        onWheel: (event) => {
            listViewAnime.running = false;
            view.scrollToY = Math.max(view.originY - view.topMargin, Math.min( view.scrollToY - (event.angleDelta.y * wheelHeightCount), scrollBottom));
            viewBar.active = true;
            event.accepted = true;
            listViewAnime.running = true;
        }
    }

    NumberAnimation {
        id: listViewAnime
        target: view
        property: "contentY"
        duration: 240
        to: view.scrollToY
        easing.type: Easing.OutCubic
        onFinished: viewBar.active = false
    }

    header: Item {
        width: view.width
        height: 36
        Text {
            x: 72
            height: 36
            text: view.headerModel[0]
            color: Theme.textColor
            font.pixelSize: Style.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.artistX
            height: 36
            text: view.headerModel[1]
            color: Theme.textColor
            font.pixelSize: Style.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.width - 76
            height: 36
            text: view.headerModel[2]
            color: Theme.textColor
            font.pixelSize: Style.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            width: parent.width - 16
            height: 1
            color: Theme.sideColor
            opacity: 0.5
            y: 35
        }
    }

    displaced: Transition {
        id: listDisplacedAnime
        SequentialAnimation {
            PauseAnimation {
                duration: {
                    const vt = listDisplacedAnime.ViewTransition;
                    const ti = vt.targetIndexes;
                    const base = (ti && ti.length > 0) ? ti[0] : vt.index;
                    return Math.min(Math.abs(vt.index - base), 8) * 30;
                }
            }
            NumberAnimation {
                properties: "y"
                duration: 240
                easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ]
            }
        }
    }
    add: Transition {
        ParallelAnimation {
            NumberAnimation {
                properties: "transY"
                from: 60
                to: 0
                duration: 320
                easing.type: Easing.OutQuint
            }
            NumberAnimation {
                properties: "opacity"
                from: 0
                to: 1
                duration: 280
                easing.type: Easing.OutCubic
            }
        }
    }
}
