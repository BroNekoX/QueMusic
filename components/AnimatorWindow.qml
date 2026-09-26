// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
import QtQuick
import QueMusic 1.0

Item {
    id: root
    z: 20
    visible: false
    property string image: ""
    anchors.fill: parent
    property int winIndex: 1
    property string title: "MusicFolder"
    property Item mainTarget
    property bool haveControl: true
    property real transX: 0
    property int originX: 0
    property list<ParallelAnimation> animeOnList: [openAnime0,openAnime1]
    property list<ParallelAnimation> animeOutList: [closeAnime0,closeAnime1]
    property int animeType: Style.settings.animeType
    transform: Translate { x: root.transX }
    Component.onCompleted: originX = mainTarget.x;


    property int songSource: MusicApi.songSource
    default property alias content: loadWidget.sourceComponent
    function opened(title: string, image: string): void {
        root.title = title;
        root.image = image;
        loadWidget.active = true;
    }
    function closed(title: string, image: string): void {
        animeOnList[animeType].running = false;
        mainTarget.visible = true;
        animeOutList[animeType].running = true;
        window.exitIndex -= 1;
    }
    Connections {
        target: window
        enabled: root.visible
        function onExit(): void {
            if(window.exitIndex <= root.winIndex) {
                root.animeOnList[root.animeType].running = false;
                root.mainTarget.visible = true;
                root.animeOutList[root.animeType].running = true;
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
        x: 136
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
        asynchronous: true
        onLoaded: {
            root.visible = true;
            root.animeOnList[root.animeType].start();
        }
    }

    ParallelAnimation {
        id: openAnime0
        onStarted: {
            root.transX = 0;
            root.mainTarget.x = root.originX;
        }

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
            target: root.mainTarget
            property: "scale"
            from: 1
            to: 1.1
            duration: 100
        }
        NumberAnimation {
            target: root.mainTarget
            property: "opacity"
            from: 1
            to: 0
            duration: 100
        }
        onFinished: root.mainTarget.visible = false
    }
    ParallelAnimation {
        id: closeAnime0
        onStarted: {
            root.transX = 0;
            root.mainTarget.x = root.originX;
        }
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
            target: root.mainTarget
            property: "scale"
            from: 1.2
            to: 1
            easing.type: Easing.OutExpo
            duration: 280
        }
        NumberAnimation {
            target: root.mainTarget
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 280
        }
        onFinished: {
            loadWidget.active = false
            root.visible = false
        }
    }

    ParallelAnimation {
        id: openAnime1
        onStarted: {
            root.scale = 1;
            root.mainTarget.scale = 1;
        }
        NumberAnimation {
            target: root
            property: "transX"
            from: 180
            to: 0
            easing.type: Easing.OutExpo
            duration: 420
        }
        NumberAnimation {
            target: root
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 420
        }
        NumberAnimation {
            target: root.mainTarget
            property: "x"
            from: root.originX
            to: root.originX - 180
            easing.type: Easing.InCubic
            duration: 120
        }
        NumberAnimation {
            target: root.mainTarget
            property: "opacity"
            from: 1
            to: 0
            easing.type: Easing.InCubic
            duration: 120
        }
        onFinished: root.mainTarget.visible = false
    }
    ParallelAnimation {
        id: closeAnime1
        onStarted: {
            root.scale = 1;
            root.mainTarget.scale = 1;
        }
        NumberAnimation {
            target: root
            property: "transX"
            from: 0
            to: 180
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
            target: root.mainTarget
            property: "x"
            from: root.originX - 180
            to: root.originX
            easing.type: Easing.OutExpo
            duration: 320
        }
        NumberAnimation {
            target: root.mainTarget
            property: "opacity"
            from: 0
            to: 1
            easing.type: Easing.OutExpo
            duration: 320
        }
        onFinished: {
            loadWidget.active = false
            root.visible = false
        }
    }
}
