// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

Rectangle {
    id: root
    color: Theme.secondaryColor
    property int contantX: width > 1400 ? (width - 1400) / 2 : 0
    property int startPage: 0
    property list<Item> pageid: [page0,page1,page2,page3,page4]
    readonly property int lastPage: pageid.length - 1

    function pageChange(toPage: int): void {
        if(pageAnime.running) return;
        pageAnime.running = false;
        pageAnime.next = toPage > startPage;
        if(pageAnime.outTarget) pageAnime.outTarget.visible = false;
        pageAnime.inTarget = pageid[toPage];
        pageAnime.outTarget = pageid[startPage];
        pageAnime.running = true;
        startPage = toPage;
    }

    component GuideHeader: Column {
        id: header
        x: 32 + root.contantX
        y: 32
        property string eyebrow: ""
        property string title: ""
        property string desc: ""
        spacing: 10
        /*Text {
            width: header.width
            text: header.eyebrow
            color: Theme.themeColor
            font.pixelSize: Style.textTip
            font.weight: Font.DemiBold
            font.letterSpacing: 1.2
            horizontalAlignment: Text.AlignHCenter
        }*/
        Text {
            //width: header.width
            text: header.title
            color: Theme.fontColor
            font.pixelSize: Style.pageTitle
            font.bold: true
            //wrapMode: Text.WordWrap
            //horizontalAlignment: Text.AlignHCenter
        }
        Text {
            //width: header.width
            text: header.desc
            color: Theme.textColor
            font.pixelSize: Style.textmain
            lineHeight: 1.4
            //wrapMode: Text.WordWrap
            //horizontalAlignment: Text.AlignHCenter
        }
    }

    component GuideSwitchRow: Item {
        id: switchRow
        property string label: ""
        property string tip: ""
        property bool value: false
        signal toggled()
        height: 58
        Text {
            id: rowLabel
            y: 9
            width: parent.width - 140
            text: switchRow.label
            color: Theme.fontColor
            font.pixelSize: Style.textmain
        }
        Text {
            anchors.top: rowLabel.bottom
            anchors.topMargin: 3
            width: parent.width - 140
            text: switchRow.tip
            color: Theme.textColor
            font.pixelSize: Style.textTip
        }
        QSwitch {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 120
            height: 36
            switchTrue: switchRow.value
            onToggled: switchRow.toggled()
        }
    }

    component FeatureCard: Rectangle {
        id: card
        property string number: ""
        property string title: ""
        property string text: ""
        height: 164
        radius: Style.cubeRadius
        color: Theme.primaryColor
        border.width: 1
        border.color: Theme.borderColor
        Column {
            x: 20
            y: 20
            width: card.width - 40
            spacing: 8
            Text {
                text: card.number
                color: Theme.themeColor
                font.pixelSize: Style.textTip
                font.bold: true
                font.letterSpacing: 1
            }
            Text {
                width: parent.width
                text: card.title
                color: Theme.fontColor
                font.pixelSize: Style.textH2
                font.bold: true
            }
            Text {
                width: parent.width
                text: card.text
                color: Theme.textColor
                font.pixelSize: Style.text
                lineHeight: 1.4
                wrapMode: Text.WordWrap
            }
        }
    }

    Item {
        y: 10
        width: 180
        height: 40
        visible: !Options.isMacOS
        opacity: root.startPage > 0 ? 1 : 0
        x: 110 - opacity * 100
        Behavior on opacity { NumberAnimation { duration: 240; easing.type: Easing.OutExpo } }
        Image {
            y: 8
            x: 15
            width: 24
            height: 24
            source: "qrc:/QueMusic/resources/icon.png"
            sourceSize: Qt.size(48, 48)
        }
        Text {
            y: 9
            x: 51
            height: 20
            text: "QueMusic"
            font.family: Fonts.text
            font.pixelSize: 17
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            color: Theme.fontColor
        }
    }

    SequentialAnimation {
        id: pageAnime
        property Item outTarget
        property Item inTarget
        property bool next: true
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                target: pageAnime.outTarget
                to: 0
                duration: 160
            }
            NumberAnimation {
                property: "x"
                target: pageAnime.outTarget
                from: 0
                to: pageAnime.next ? -360 : 360
                duration: 160
                easing.type: Easing.InCubic
            }
        }
        ScriptAction { script: { pageAnime.outTarget.visible = false; pageAnime.inTarget.visible = true } }
        ParallelAnimation {
            NumberAnimation {
                property: "opacity"
                target: pageAnime.inTarget
                from: 0
                to: 1
                duration: 240
                easing.type: Easing.OutCubic
            }
            NumberAnimation {
                property: "x"
                target: pageAnime.inTarget
                from: pageAnime.next ? 360 : -360
                to: 0
                duration: 360
                easing.type: Easing.OutExpo
            }
        }
    }

    Item {
        id: page0
        y: 60
        width: root.width
        height: root.height - 140
        Component.onCompleted: {
            quemainAnime.start();
        }

        SequentialAnimation {
            id: quemainAnime
            PauseAnimation { duration: 1000 }
            ScriptAction { script: appTextRow.allText = ["Q","u","e","M","u","s","i","c"," ","P","r","o","j","e","c","t"] }
            NumberAnimation {
                target: appIcon
                property: "x"
                from: page0.width / 2 - 32
                to: page0.width / 2 - appText.width / 2 - 40
                duration: 480
                easing.type: Easing.InOutQuart
            }
            ScriptAction { script: appIcon.x = Qt.binding(function() { return page0.width / 2 - appText.width / 2 - 40 }) }
            NumberAnimation {
                target: controlBar
                property: "y"
                from: root.height
                to: root.height - 78
                duration: 480
                easing.type: Easing.OutExpo
            }
            ScriptAction { script: controlBar.y = Qt.binding(function() { return root.height - 78 }) }
        }

        Image {
            id: appIcon
            x: page0.width / 2 - 32
            y: page0.height / 2 - 32
            width: 64
            height: 64
            source: "qrc:/QueMusic/resources/icon.png"
            sourceSize: Qt.size(64, 64)
        }
        Text {
            id: appText
            height: 64
            text: "QueMusic Project"
            font.family: Fonts.text
            font.pixelSize: 36
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            color: Theme.fontColor
            visible: false
        }
        Row {
            id: appTextRow
            y: page0.height / 2 - 32
            x: page0.width / 2 - width / 2 + 40
            height: 64
            property list<string> allText
            Repeater {
                model: appTextRow.allText
                delegate: Text {
                    id: textChar
                    required property string modelData
                    required property int index
                    text: modelData
                    height: 64
                    font.family: Fonts.text
                    font.pixelSize: 36
                    font.bold: true
                    color: Theme.fontColor
                    verticalAlignment: Text.AlignVCenter
                    opacity: 0
                    SequentialAnimation {
                        Component.onCompleted: running = true;
                        PauseAnimation {
                            duration: textChar.index * 24 + 240
                        }
                        ParallelAnimation {
                            NumberAnimation {
                                target: textChar
                                properties: "y"
                                from: 64
                                to: 0
                                duration: 640
                                easing.type: Easing.OutExpo
                            }
                            NumberAnimation {
                                target: textChar
                                properties: "opacity"
                                from: 0
                                to: 1
                                duration: 420
                                easing.type: Easing.OutCubic
                            }
                        }
                    }
                }
            }
        }
        Text {
            id: appTip
            x: page0.width / 2 - width / 2
            y: page0.height / 2 + 60
            text: "Powered by QtQuick Engine"
            color: Theme.textColor
            font.pixelSize: Style.textmain
            font.family: Fonts.text
            opacity: 0
            SequentialAnimation {
                Component.onCompleted: start()
                PauseAnimation { duration: 1900 }
                NumberAnimation {
                    target: appTip
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 520
                    easing.type: Easing.OutCubic
                }
            }
        }
    }
    Item {
        id: page1
        y: 60
        visible: false
        width: root.width
        height: root.height - 140

        GuideHeader {
            //width: parent.width
            eyebrow: "01  认识 QueMusic"
            title: "一个窗口，装下你的音乐"
            desc: "永久开源免费，从在线音源到本地曲库，原生渲染让每一次操作都跟得上耳朵。"
        }
        Column {
            id: page1Body
            width: Math.min(page1.width - 96, 960)
            anchors.centerIn: parent
            spacing: 26

            Row {
                width: parent.width
                spacing: 16
                FeatureCard {
                    width: (page1Body.width - 32) / 3
                    number: "01"
                    title: "在线音源"
                    text: "网易云、酷狗、B 站等平台搜索即播，歌单与收藏一并管理。"
                }
                FeatureCard {
                    width: (page1Body.width - 32) / 3
                    number: "02"
                    title: "本地曲库"
                    text: "扫描本地文件夹，自动读取标签、封面与内嵌歌词。"
                }
                FeatureCard {
                    width: (page1Body.width - 32) / 3
                    number: "03"
                    title: "沉浸歌词"
                    text: "逐行动画、桌面歌词与频谱可视化，听歌更有氛围。"
                }
            }

            Text {
                width: parent.width
                text: "均衡器 · 歌单收藏 · 插件市场 · 桌面歌词 · 快捷键"
                color: Theme.textColor
                font.pixelSize: Style.text
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    Item {
        id: page2
        y: 60
        visible: false
        width: root.width
        height: root.height - 140

        GuideHeader {
            //width: parent.width
            eyebrow: "02  自定义外观"
            title: "让它更像你"
            desc: "挑一个顺眼的主题与配色，改动立刻生效，并会自动保存。"
        }
        Column {
            id: page2Body
            width: Math.min(page2.width - 96, 720)
            anchors.centerIn: parent
            spacing: 20

            Rectangle {
                width: parent.width
                height: 198
                radius: Style.cubeRadius
                color: Theme.primaryColor
                border.width: 1
                border.color: Theme.borderColor

                Text {
                    x: 24
                    y: 24
                    width: 80
                    height: 42
                    text: "主题模式"
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                }
                QWideDrop {
                    x: 120
                    y: 24
                    width: parent.width - 144
                    height: 42
                    model: ["浅色", "深色", "跟随系统"]
                    choice: Style.theme
                    onTransformed: (choiced) => {
                        Style.theme = choiced;
                        Options.changeTheme();
                    }
                }
                Text {
                    x: 24
                    y: 84
                    width: 80
                    height: 36
                    text: "配色方案"
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                }
                QDrop {
                    x: 120
                    y: 84
                    width: parent.width - 144
                    height: 36
                    choice: Style.colorStyle
                    model: ["默认", "Material", "FluentUI", "QuietUI", "Material You"]
                    onTransformed: (choiced) => {
                        Style.colorStyle = choiced;
                        Options.changeTheme();
                    }
                }
                Text {
                    x: 24
                    y: 138
                    width: 80
                    height: 36
                    text: "主题色"
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                }
                Row {
                    x: 120 + (parent.width - 144 - width) / 2
                    y: 138
                    height: 36
                    spacing: 8
                    Repeater {
                        model: Style.colorList
                        delegate: SButton {
                            required property int index
                            required property color modelData
                            width: 36
                            height: 36
                            radius: 10
                            buttonColor: modelData
                            borderWidth: Style.color === index ? 2 : 0
                            borderColor: Theme.fontColor
                            onClicked: {
                                Style.color = index;
                                Options.changeTheme();
                            }
                        }
                    }
                }
            }

            Text {
                width: parent.width
                text: "之后可以在「设置 - 通用」里继续微调"
                color: Theme.textColor
                font.pixelSize: Style.textTip
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    Item {
        id: page3
        y: 60
        visible: false
        width: root.width
        height: root.height - 140

        GuideHeader {
            //width: parent.width
            eyebrow: "03  播放与歌词"
            title: "按你的方式聆听"
            desc: "这几个开关会直接影响播放页与歌词页的表现。"
        }
        Column {
            id: page3Body
            width: Math.min(page3.width - 96, 720)
            anchors.centerIn: parent
            spacing: 20

            Rectangle {
                width: parent.width
                height: 210
                radius: Style.cubeRadius
                color: Theme.primaryColor
                border.width: 1
                border.color: Theme.borderColor

                GuideSwitchRow {
                    x: 24
                    y: 16
                    width: parent.width - 48
                    label: "显示音波效果"
                    tip: "播放页和歌词页的实时频谱"
                    value: Style.waveDisplay
                    onToggled: Style.waveDisplay = !Style.waveDisplay
                }
                Rectangle {
                    x: 24
                    y: 75
                    width: parent.width - 48
                    height: 1
                    color: Theme.sideColor
                }
                GuideSwitchRow {
                    x: 24
                    y: 76
                    width: parent.width - 48
                    label: "高级逐行弹簧动画"
                    tip: "歌词切行时更柔和的弹簧过渡"
                    value: Style.premiumLyricAnime
                    onToggled: Style.premiumLyricAnime = !Style.premiumLyricAnime
                }
                Rectangle {
                    x: 24
                    y: 135
                    width: parent.width - 48
                    height: 1
                    color: Theme.sideColor
                }
                GuideSwitchRow {
                    x: 24
                    y: 136
                    width: parent.width - 48
                    label: "高级界面动画"
                    tip: "页面切换与卡片展开的额外动效"
                    value: Style.premiumAnime
                    onToggled: Style.premiumAnime = !Style.premiumAnime
                }
            }

            Text {
                width: parent.width
                text: "更多选项在「设置 - 界面」里"
                color: Theme.textColor
                font.pixelSize: Style.textTip
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    Item {
        id: page4
        y: 60
        visible: false
        width: root.width
        height: root.height - 140

        GuideHeader {
            //width: parent.width
            eyebrow: "04  准备就绪"
            title: "开始之前，记住这几个键"
            desc: "默认快捷键如下，全部都可以在「设置 - 快捷键」里重新绑定。"
        }
        Column {
            id: page4Body
            width: Math.min(page4.width - 96, 760)
            anchors.centerIn: parent
            spacing: 20

            Rectangle {
                width: parent.width
                height: 204
                radius: Style.cubeRadius
                color: Theme.primaryColor
                border.width: 1
                border.color: Theme.borderColor

                Grid {
                    id: keyGrid
                    x: 20
                    y: 20
                    width: parent.width - 40
                    columns: 2
                    columnSpacing: 24
                    rowSpacing: 12

                    Repeater {
                        model: [
                            { "k": "Space", "v": "播放 / 暂停" },
                            { "k": "← / →", "v": "上一首 / 下一首" },
                            { "k": "Ctrl+↑", "v": "音量增加" },
                            { "k": "Ctrl+↓", "v": "音量减少" },
                            { "k": "Ctrl+←", "v": "快退" },
                            { "k": "Ctrl+→", "v": "快进" },
                            { "k": "Ctrl+D", "v": "收藏当前歌曲" },
                            { "k": "Ctrl+M", "v": "静音切换" }
                        ]
                        delegate: Row {
                            id: keyRow
                            required property var modelData
                            width: keyGrid.width / 2 - 12
                            height: 32
                            spacing: 12
                            Rectangle {
                                width: 96
                                height: 30
                                radius: 8
                                color: Theme.sideColor
                                Text {
                                    anchors.centerIn: parent
                                    text: keyRow.modelData.k
                                    color: Theme.fontColor
                                    font.pixelSize: Style.text
                                    font.bold: true
                                }
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: keyRow.modelData.v
                                color: Theme.textColor
                                font.pixelSize: Style.textmain
                            }
                        }
                    }
                }
            }

            Text {
                width: parent.width
                text: "下次想再看这份指引，可以在「设置 - 通用」里重新打开"
                color: Theme.textColor
                font.pixelSize: Style.textTip
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
    Rectangle {
        id: controlBar
        y: root.height
        height: 78
        width: root.width
        color: Theme.primaryColor

        Row {
            anchors.centerIn: parent
            height: 6
            spacing: 8
            Repeater {
                model: root.pageid.length
                delegate: Rectangle {
                    required property int index
                    width: index === root.startPage ? 18 : 6
                    height: 6
                    radius: 3
                    color: index === root.startPage ? Theme.themeColor : Theme.sideColor
                    Behavior on width { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                    Behavior on color { ColorAnimation { duration: 180 } }
                }
            }
        }

        QButton {
            x: 24 + root.contantX
            y: 21
            height: 36
            width: 100
            text: "暂时跳过"
            radius: 12
            buttonColor: Theme.sideColor
            shadowEnabled: false
            onClicked: {
                jumpDialog.open();
            }
        }
        QButton {
            x: parent.width - 240 - root.contantX
            y: 21
            height: 36
            width: 100
            text: "上一步"
            radius: 12
            visible: root.startPage > 0
            buttonColor: Theme.sideColor
            shadowEnabled: false
            onClicked: {
                root.pageChange(root.startPage - 1);
            }
        }
        QButton {
            x: parent.width - 124 - root.contantX
            y: 21
            height: 36
            width: 100
            text: root.startPage === root.lastPage ? "开始使用" : "下一步"
            radius: 12
            buttonColor: Theme.themeColor
            textColor: Theme.primaryColor
            iconColor: Theme.primaryColor
            shadowEnabled: false
            onClicked: {
                if(root.startPage === root.lastPage) {
                    finishAnime.running = true;
                } else {
                    root.pageChange(root.startPage + 1);
                }
            }
        }
    }
    QAlertDialog {
        id: jumpDialog
        title: "是否跳过"
        blurSource: root
        message: "这将跳过指引页面，之后可在设置页面重新打开"
        onConfirm: {
            finishAnime.running = true;
        }
    }
    ParallelAnimation {
        id: finishAnime
        onStarted: mainLayout.visible = true;
        NumberAnimation {
            property: "opacity"
            target: root
            to: 0
            duration: 160
        }
        NumberAnimation {
            property: "scale"
            target: root
            from: 1
            to: 1.2
            duration: 160
            easing.type: Easing.InCubic
        }
        onFinished: settingsView.readyStart();
    }
}