// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//

pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

QScrollView {
    id: page
    property int standWidth: 0
    property int containWidth: 0
    property int containX: 0
    property color cardColor: Options.cardColor

    y: 60
    width: parent ? parent.width : 0
    height: parent ? parent.height - 60 : 0
    visible: false

    Column {
        spacing: 16
        padding: 24
        width: page.containWidth
        x: page.containX

        Text {
            width: page.standWidth
            height: 40
            color: Theme.fontColor
            verticalAlignment: Text.AlignVCenter
            text: "音频"
            font.pixelSize: Style.pageTitle
            font.weight: Font.DemiBold
            font.letterSpacing: -0.3
        }

        Loader {
            width: page.standWidth
            active: page.visible
            sourceComponent: EqualizerPanel {
                width: page.standWidth
                controlWidth: page.standWidth
                cardColor: page.cardColor
            }
        }
    }
}
