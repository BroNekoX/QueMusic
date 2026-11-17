// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Templates
import QueMusic 1.0

ScrollBar {
    id: root

    implicitWidth: Math.max(implicitContentWidth + leftPadding + rightPadding,
                            implicitBackgroundWidth + leftInset + rightInset)
    implicitHeight: Math.max(implicitContentHeight + topPadding + bottomPadding,
                             implicitBackgroundHeight + topInset + bottomInset)

    padding: 2
    visible: policy !== ScrollBar.AlwaysOff
    minimumSize: orientation === Qt.Horizontal ? height / width : width / height

    contentItem: Rectangle {
        id: handle
        implicitWidth: 6
        implicitHeight: 6
        radius: width / 2
        color: root.pressed ? Theme.fontColor : Theme.textColor
        opacity: 0.0

        states: State {
            name: "active"
            when: root.policy === ScrollBar.AlwaysOn || (root.active && root.size < 1.0)
            PropertyChanges { handle.opacity: 0.5 }
        }

        transitions: Transition {
            from: "active"
            SequentialAnimation {
                PauseAnimation { duration: 450 }
                NumberAnimation { target: handle; duration: 200; property: "opacity"; to: 0.0 }
            }
        }
    }
}
