// AnimatorWindow.qml
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: root
    //color: Style.themes.primaryColor
    z: 20
    //border.color: Style.themes.secondaryColor
    visible: false
    property string image: ""
    anchors.fill: parent
    property int winIndex: 1
    property string title: "MusicFolder"
    property var mainTarget
    property bool haveControl: true

    property int songSource: MusicApi.songSource
    default property alias content: loadWidget.sourceComponent
    function opened(title,image) {
        root.title = title;
        root.image = image;
        loadWidget.active = true;
    }
    function closed(title,image) {
        windowOpenAnime.running = false;
        mainTarget.visible = true;
        windowCloseAnime.running = true;
        window.exitIndex -= 1;
    }
    Connections {
        target: window
        enabled: root.visible
        function onExit() {
            if(window.exitIndex <= root.winIndex) {
                windowOpenAnime.running = false;
                root.mainTarget.visible = true;
                windowCloseAnime.running = true;
            }
        }
    }

    SequentialAnimation {
        id: windowOpenAnime
    ParallelAnimation {
        NumberAnimation {
            target: root
            property: "scale"
            from: 0.8
            to: 1
            easing.type: Easing.OutExpo
            duration: 360
        }
        NumberAnimation {
            target: root
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 360
        }
        NumberAnimation {
            target: mainTarget
            property: "scale"
            from: 1
            to: 1.1
            duration: 100
        }
        NumberAnimation {
            target: mainTarget
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }
    }
    ScriptAction {
        script: root.mainTarget.visible = false
    }
    }
    SequentialAnimation {
        id: windowCloseAnime
    ParallelAnimation {
        NumberAnimation {
            target: root
            property: "scale"
            from: 1
            to: 0.9
            duration: 100
        }
        NumberAnimation {
            target: root
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }
        NumberAnimation {
            target: mainTarget
            property: "scale"
            from: 1.2
            to: 1
            easing.type: Easing.OutExpo
            duration: 280
        }
        NumberAnimation {
            target: mainTarget
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 280
        }
    }
    ScriptAction {
        script: {
            loadWidget.active = false
            root.visible = false
        }
    }
    }

    QPicture {
        id: headPic
        y: 16
        x: 16
        z: 4
        width: 96
        height: 96
        radius: 16
        source: root.image || "qrc:/QueMusic/resources/app/musicpic.png"
    }

    Text {
        id: headTitle
        x: 144
        y: root.haveControl ? 16 : 34
        width: 200
        height: 60
        color: Style.themes.fontColor
        text: root.title
        font.pixelSize: Style.settings.textH1
        verticalAlignment: Text.AlignVCenter
    }

    Loader {
        id: loadWidget
        active: false
        onLoaded: {
            root.visible = true
            windowOpenAnime.start()
        }
    }
}