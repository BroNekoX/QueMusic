// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0
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
    }
    property real viewh: tipview.contentHeight

    function dialog(name: string, text: string, icon: string): void {
        tipModel.append({ icontype: icon, name: name, text: text });
    }

    add: Transition {
        ParallelAnimation{
            NumberAnimation {
                properties: "x"
                from: 400
                to: 0
                duration: 420
                easing.type: Easing.OutExpo
            }
        }
    }

    displaced: Transition {
        ParallelAnimation{
            NumberAnimation {
                properties: "y"
                duration: 320
                easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ]
            }
        }
    }

    delegate: Rectangle {
        id: amessage
        required property int index
        required property var model
        width: ListView.view.width
        height: 80
        radius: Style.labelRadius
        color: Theme.fontColor
        border.width: 1
        border.color: Theme.textColor

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

        NumberAnimation {
            id: removeAnimation
            target: amessage
            properties: "x"
            to: 350
            duration: 320
            easing.type: Easing.InExpo
            onFinished: tipModel.remove(amessage.index)
        }

        RectangularShadow {
            anchors.fill: amessage
            z: -1
            offset.x: 2
            offset.y: 2
            radius: Style.labelRadius
            blur: 24
            spread: 0
            visible: true
            color: Theme.shadowColor
        }


        Rectangle {
            x: 24
            y: 24
            height: 32
            width: 32
            radius: 8
            color: Theme.textColor
            z: 4
            clip: false
            Text {
                anchors.fill: parent
                text: amessage.model.icontype
                font.family: Fonts.icon
                font.pixelSize: Style.texticon
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                color: Theme.sideColor

                Behavior on color { ColorAnimation { duration: 150 } }
            }
        }

        Text {
            x: 80
            y: 20
            width: 230
            height: 20
            z: 8
            text: amessage.model.name
            font.pixelSize: 14
            font.bold: true
            color: Theme.primaryColor
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            x: 80
            y: 40
            width: 230
            height: 20
            z: 6
            text: amessage.model.text
            font.pixelSize: 12
            font.bold: false
            color: Theme.sideColor
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
        }
    }
}
