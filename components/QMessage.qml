// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects

ListView {
    id: tipview
    x: parent.width - 320
    y: parent.height - height - 20
    width: 300
    height: tipModel.count * 100 - 20
    spacing: 20
    verticalLayoutDirection: ListView.BottomToTop
    clip: false
    visible: tipModel.count !== 0
    parent: Overlay.overlay
    z: 6
    model: tipModel
    interactive: false
    property alias messageModel: tipModel
    ListModel {
        id: tipModel

        //ListElement { icontype: ""; name: ""; text: "" }
    }
    property real viewh: tipview.contentHeight

    function dialog(name,text,icon) {
        tipModel.append({ icontype: icon, name: name, text: text });
    }

    // add过渡动画（新增Item触发）
    add: Transition {
        ParallelAnimation{
            NumberAnimation {
                properties: "x"
                from: 400
                to: 0
                duration: 350
                easing.type: Easing.OutExpo
            }
        }
    }

    //model变化
    displaced: Transition {
        ParallelAnimation{
            NumberAnimation {
                properties: "y"
                duration: 240
                easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ]
            }
        }
    }

    /*结束动画
        remove: Transition {
                NumberAnimation {
                    properties: "x"
                    to: 400
                    duration: 300
                    easing.type: Easing.InCubic
                }
        }
        */

    delegate: Rectangle {
        id: amessage
        width: ListView.view.width
        height: 80
        radius: Style.settings.labelRadius
        color: Style.themes.fontColor
        border.width: 1
        border.color: Style.themes.textColor

        scale: 1.0
        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutQuad } }

        ListView.onAdd:{
            messageTimer.start()
        }
        Timer {
            id: messageTimer
            interval: 2000
            onTriggered: {
                removeAnimation.start()
            }
        }

        SequentialAnimation {
            id: removeAnimation
            NumberAnimation {
                target: amessage
                properties: "x"
                to: 350
                duration: 240
                easing.type: Easing.InExpo
            }
            ScriptAction {
                script: tipModel.remove(index);
            }
        }

        //Component.onCompleted: {
        //    metip.rectXy = Qt.rect( maintip.x + 20, amessage.mapToItem(tipview, 0, 0).y - 60, 310, 80)
        //}

        // === 阴影效果 ===
        RectangularShadow {
            anchors.fill: amessage
            z: -1
            offset.x: 2
            offset.y: 2
            radius: Style.settings.labelRadius
            blur: 24
            spread: 0
            visible: true
            color: Style.themes.shadowColor
        }


        //显示区
        Rectangle {
            x: 24
            y: 24
            height: 32
            width: 32
            radius: 8
            color: Style.themes.textColor
            //border.width: 1
            //border.color: Style.themes.sideColor
            z: 4
            clip: false
            Text {
                anchors.fill: parent
                text: model.icontype
                font.family: iconFont.name
                font.pixelSize: Style.settings.texticon
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: Style.themes.sideColor

                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        Label {
            x: 80
            y: 20
            width: 230
            height: 20
            z: 8
            text: model.name
            font.pixelSize: 14
            font.bold: true
            color: Style.themes.primaryColor
            verticalAlignment: Text.AlignVCenter
        }

        Label {
            x: 80
            y: 40
            width: 230
            height: 20
            z: 6
            text: model.text
            font.pixelSize: 12
            font.bold: false
            color: Style.themes.sideColor
            verticalAlignment: Text.AlignVCenter
        }

        SButton {
            text: ""
            iconCharacter: "\uf025"
            x: 264
            y: 10
            z: 12
            width: 26
            height: 26
            radius: 8
            buttonColor: "transparent"
            hoverColor: Qt.rgba(0,0,0,0.2)
            shadowEnabled: false
            onClicked: {
                removeAnimation.start();
            }
        }



        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: amessage.scale = 1.04
            onExited: amessage.scale = 1.0
            onPressed: amessage.scale = 0.96
            onReleased: amessage.scale = 1.0
            onCanceled: amessage.scale = 1.0
            onClicked: {
            }
        }
    }
}

