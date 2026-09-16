// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QueMusic 1.0

ListView {
    id: view
    topMargin: 6
    bottomMargin: 24
    rightMargin: 16
    property int scrollToY: view.contentY
    property list<string> headerModel: isList ? ["标题","创建者","曲目","操作"] : ["标题","歌手","时长","操作"]
    property list<string> menuModel: ["下载到本地","分享","歌曲信息"]
    property list<int> selectedIndices: []
    readonly property var selectedSet: new Set(selectedIndices)
    property bool isList: false
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
    function scrollTop() {
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
            color: Style.themes.textColor
            font.pixelSize: Style.settings.text
        }
    }
    Menu {
        id: menu
        title: "Menu"
        parent: Overlay.overlay
        property int index

        background: QBlurCard {
            implicitWidth: 150
            implicitHeight: 40
            shadowEffect: true
            blurSource: mainLayout
            rectXy: Qt.rect(menu.x, menu.y, menu.width, menu.height)
            cardColor: Style.themes.blurSecondaryColor
            borderRadius: Style.settings.labelRadius
        }

        Instantiator {
            model: view.menuModel
            delegate: MenuItem {
                id: menuItem
                background: Rectangle {
                    implicitWidth: 146
                    implicitHeight: 36
                    x: 2
                    y: 2
                    radius: Style.settings.labelRadius - 2
                    width: menuItem.width - 4
                    height: menuItem.height - 4
                    color: menuItem.down || menuItem.highlighted ? Style.themes.hoverColor : "transparent"
                }
                text: modelData
                contentItem: Text {
                    text: menuItem.text
                    color: Style.themes.fontColor//使用项目主题文字色，深浅色主题下都可读
                    font.pixelSize: Style.settings.textmain
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

    rebound: Transition {
        NumberAnimation {
            properties: "y"
            duration: 420
            easing.type: Easing.Bezier
            easing.bezierCurve: [ 0.16, 0.03, 0.00, 1.00, 1, 1 ]
        }
    }
    header: Item {
        width: view.width
        height: 36
        Text {
            x: 80
            height: 36
            text: view.headerModel[0]
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.artistX
            height: 36
            text: view.headerModel[1]
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.width - 76
            height: 36
            text: view.headerModel[2]
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            width: parent.width - 16
            height: 1
            color: Style.themes.sideColor
            opacity: 0.5
            y: 35
        }
    }

    displaced: Transition {
        id: listDisplacedAnime
        SequentialAnimation {
            PauseAnimation {
                duration: (listDisplacedAnime.ViewTransition.index - listDisplacedAnime.ViewTransition.targetIndexes[0]) * 40
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

    delegate: Rectangle {
        id: listDel
        //required property int index
        height: 60
        width: view.width - 16
        color: view.selectedSet.has(index) ? Style.themes.containColor : "#00000000"
        radius: Style.settings.labelRadius
        property int transY: 0
        transform: Translate { y: listDel.transY }

        Rectangle {
            anchors.fill: parent
            radius: Style.settings.labelRadius
            color: Style.themes.hoverColor
            opacity: listArea.containsMouse ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        QPicture {
            y: 8
            x: 8
            width: 44
            height: 44
            radius: 10
            source: model.cover.replace("{size}","64") || "qrc:/QueMusic/resources/app/musicpic.png"
        }


        Text {
            id: title
            x: 80
            y: 16
            width: view.artistX - 110
            height: 28
            text: model.title || "Unknown"
            color: Style.themes.fontColor
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            font.pixelSize: Style.settings.textmain
            verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            color: Style.themes.containColor
            x: title.implicitWidth > title.width ? title.width + 75 : title.implicitWidth + 85
            y: 20
            width: 32
            height: 18
            radius: 9
            visible: model.paytype === 3
            Text {
                text: "VIP"
                anchors.centerIn: parent
                color: Style.themes.themeColor
                font.pixelSize: 9
                font.weight: Font.DemiBold
            }
        }
        Text {
            x: view.artistX
            y: 16
            width: view.artistX - 128
            height: 28
            text: model.artist || "Unknown"
            color: Style.themes.textColor
            font.weight: Font.Normal
            elide: Text.ElideRight
            font.pixelSize: Style.settings.text
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.width - 92
            y: 16
            width: 60
            height: 28
            text: view.isList ? model.duration + "首" : Math.floor(model.duration / 60) + ":" + (model.duration % 60)
            color: Style.themes.textColor
            font.bold: false
            elide: Text.ElideRight
            font.pixelSize: Style.settings.text
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }

        MouseArea {
            id: listArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    view.clicked(index);
                } else {
                    menu.index = index;
                    view.menu.popup();
                }
                forceActiveFocus();
            }

            Row {
                x: view.toolX
                spacing: 2
                y: 12
                height: 36
                opacity: listArea.containsMouse ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 160 } }
                SButton {
                    iconCharacter: "\uf050"
                    width: 36
                    height: 36
                    radius: 36
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    shadowEnabled: false
                    tipText: "更多"
                    onClicked: {
                        menu.index = index
                        view.menu.popup()
                    }
                }
                SButton {
                    iconCharacter: view.toolText1
                    width: 36
                    height: 36
                    radius: 36
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    shadowEnabled: false
                    tipText: "收藏"
                    onClicked: view.toolClicked(index,1)
                }
                SButton {
                    iconCharacter: view.toolText0
                    width: 36
                    height: 36
                    radius: 36
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    shadowEnabled: false
                    tipText: "加入播放列表"
                    onClicked: view.toolClicked(index,0)
                }
            }
        }
    }
}
