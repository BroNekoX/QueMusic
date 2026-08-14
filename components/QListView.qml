// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

ListView {
    id: view
    topMargin: 6
    bottomMargin: 24
    rightMargin: 16
    property int scrollToY: view.contentY
    property list<string> headerModel: isList ? ["标题","创建者","曲目","操作"] : ["标题","歌手","时长","操作"]//text,x
    property list<string> menuModel: ["下载到本地","分享","歌曲信息"]
    property list<int> selectedIndices: []
    property bool isList: false
    property int artistX: width / 2 - 50
    property int toolX: width - 210
    property string toolText0: "\uf095"
    property string toolText1: "\uf0c8"
    property alias menu: menu
    contentWidth: view.width - 16
    synchronousDrag: true
    reuseItems: true
    onDraggingChanged: view.scrollToY = view.contentY
    signal clicked(int index)
    signal menuClicked(int index,int choice)
    signal toolClicked(int index,int tool)//从右往左2（菜单)，1（喜欢），0（通用）
    Menu {
        id: menu
        title: "Menu"
        //parent: Overlay.overlay
        parent: Overlay.overlay
        property int index
        //closePolicy: Popup.CloseOnEscape

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
        //anchors.leftMargin: 8
        anchors.bottom: view.bottom
        onPressedChanged: {
            view.scrollToY = view.contentY
        }
    }
    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: (event) => {
            view.scrollToY = Math.max( -32 - view.topMargin, Math.min( view.scrollToY - (event.angleDelta.y * 0.25 * Qt.application.styleHints.wheelScrollLines), view.contentHeight - view.height + view.bottomMargin));
            listViewAnime.running = false;
            listViewAnime.running = true;
            script: viewBar.active = true;
            event.accepted = true;
        }
    }
    SequentialAnimation {
        id: listViewAnime
        NumberAnimation {
            target: view
            property: "contentY"
            duration: 240
            to: view.scrollToY
            easing.type: Easing.OutCubic
        }
        ScriptAction {
            script: viewBar.active = true
        }
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
            x: view.toolX + 16
            height: 36
            text: view.headerModel[2]
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.toolX + 70
            height: 36
            text: view.headerModel[3]
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
                properties: "x"
                from: 400
                to: 0
                duration: 350
                easing.type: Easing.OutExpo
            }
            NumberAnimation {
                properties: "opacity"
                from: 0
                to: 1
                duration: 350
                easing.type: Easing.OutExpo
            }
        }
    }

    delegate: Rectangle {
        id: listDel
        height: 60
        width: view.width - 16
        color: view.selectedIndices.indexOf(index) !== -1 ? Style.themes.containColor : "#00000000"
        radius: Style.settings.labelRadius

        //radius: Style.settings.labelRadius
        //color: index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"

        Rectangle {
            anchors.fill: parent
            radius: Style.settings.labelRadius
            color: Style.themes.hoverColor
            opacity: listArea.containsMouse ? 1 : 0
            z: 1
            Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        QPicture {
            y: 8
            x: 8
            z: 4
            width: 44
            height: 44
            radius: 10
            source: model.cover.replace("{size}","64") || "qrc:/QueMusic/resources/app/musicpic.png"
        }


        Text {
            id: title
            x: 80
            y: 16
            z: 3
            width: view.artistX - 110
            height: 28
            text: model.title || "Unknown"
            color: Style.themes.fontColor
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            font.pixelSize: Style.settings.textmain
            verticalAlignment: Text.AlignVCenter
            visible: true
            Behavior on color { ColorAnimation { duration: 120 } }
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
            z: 3
            width: view.artistX - 100
            height: 28
            text: model.artist || "Unknown"
            color: Style.themes.textColor
            font.weight: Font.Normal
            elide: Text.ElideRight
            font.pixelSize: Style.settings.text
            verticalAlignment: Text.AlignVCenter
            visible: true
            Behavior on color { ColorAnimation { duration: 120 } }
        }
        Text {
            x: view.toolX
            y: 16
            z: 3
            width: 60
            height: 28
            text: model.duration + "首"
            color: Style.themes.textColor
            font.bold: false
            elide: Text.ElideRight
            font.pixelSize: Style.settings.text
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            visible: true
            Behavior on color { ColorAnimation { duration: 120 } }
            Component.onCompleted: {
                if(!view.isList) {
                    text = Math.floor(model.duration / 60) + ":" + (model.duration % 60)
                }
            }
        }

        MouseArea {
            id: listArea
            anchors.fill: parent
            hoverEnabled: true
            z: 5
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    onClicked: view.clicked(index)
                } else {
                    menu.index = index
                    view.menu.popup()
                }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
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
                    onClicked: view.toolClicked(index,0)
                }
            }
        }
    }
}
