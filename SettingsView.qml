// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Dialogs
import Qt5Compat.GraphicalEffects
import QueMusic 1.0

Item {
    id: settingsView

    Rectangle {
        id: leftSidebarSettings
        x: 0
        y: 0
        z: 5
        height: settingsView.height
        width: 210
        opacity: 1
        property color choiceColor: Style.themes.hoverColor
        property color choiceTextColor: Style.themes.fontColor
        color: Style.settings.sidebarColor ? Style.themes.secondaryBlurColor : Style.themes.primaryBlurColor
        Connections {
            target: Style
            function onChangeTheme() {
                if(Style.settings.sidebarStyle === 0) {
                    leftSidebarSettings.choiceColor = Style.themes.hoverColor;
                    leftSidebarSettings.choiceTextColor = Style.themes.fontColor;
                    choicebar1.x = 16;
                    choicebar1.radius = 2;
                } else if(Style.settings.sidebarStyle === 1) {
                    leftSidebarSettings.choiceColor = Style.themes.themeColor;
                    leftSidebarSettings.choiceTextColor = Style.themes.primaryColor;
                    choicebar1.x = 0;
                    choicebar1.radius = 0;
                }
            }
        }
        Component.onCompleted: {
            index1ed(0);
            if(Style.settings.sidebarStyle === 0) {
                leftSidebarSettings.choiceColor = Style.themes.hoverColor;
                leftSidebarSettings.choiceTextColor = Style.themes.fontColor;
                choicebar1.x = 16;
                choicebar1.radius = 2;
            } else if(Style.settings.sidebarStyle === 1) {
                leftSidebarSettings.choiceColor = Style.themes.themeColor;
                leftSidebarSettings.choiceTextColor = Style.themes.primaryColor;
                choicebar1.x = 0;
                choicebar1.radius = 0;
            }
            leftBarAnime.running = true;
        }

        // 左栏显现动画
        NumberAnimation {
            id: leftBarAnime
            target: leftSidebarSettings
            property: "x"
            from: -210
            to: 0
            duration: 300
            easing.type: Easing.OutExpo
        }

        signal index1ed(int choice)

        Rectangle {
            id: choicebar1
            x: 16
            width: 4
            height: barBottom - y
            y: 80
            topRightRadius: 2
            bottomRightRadius: 2
            radius: 2
            color: Style.themes.themeColor
            opacity: 1
            property int barBottom: 102
            property int willBarY: 80
            property int indexOld: 0
            ParallelAnimation {
                id: downBarS
                NumberAnimation {
                    property: "barBottom"
                    target: choicebar1
                    to: choicebar1.willBarY + 22
                    easing.type: Easing.Bezier
                    easing.bezierCurve: [ 0.50, 0.00, 0.00, 1.00, 1, 1 ]
                    duration: 280
                }
                NumberAnimation {
                    property: "y"
                    target: choicebar1
                    to: choicebar1.willBarY
                    easing.type: Easing.Bezier
                    easing.bezierCurve: [ 1.00, 0.00, 0.50, 1.00, 1, 1 ]
                    duration: 280
                }
            }
            ParallelAnimation {
                id: upBarS
                NumberAnimation {
                    property: "barBottom"
                    target: choicebar1
                    to: choicebar1.willBarY + 22
                    easing.type: Easing.Bezier
                    easing.bezierCurve: [ 1.00, 0.00, 0.50, 1.00, 1, 1 ]
                    duration: 280
                }
                NumberAnimation {
                    property: "y"
                    target: choicebar1
                    to: choicebar1.willBarY
                    easing.type: Easing.Bezier
                    easing.bezierCurve: [ 0.50, 0.00, 0.00, 1.00, 1, 1 ]
                    duration: 280
                }
            }
            Connections {
                target: leftSidebarSettings
                function onIndex1ed(choice) {
                    choicebar1.willBarY = 44 * choice + 80
                    if(choice > choicebar1.indexOld) {
                        downBarS.stop()
                        upBarS.stop()
                        downBarS.running = true
                    } else if(choice < choicebar1.indexOld) {
                        upBarS.stop()
                        downBarS.stop()
                        upBarS.running = true
                    }
                    choicebar1.indexOld = choice
                }
            }
        }

        Item {
            x: 15;y: 12
            height: 36
            width: 180
            QWKButton {
                id: returnButton
                width: 36
                height: 36
                source: Style.darkis ? "qrc:/QueMusic/resources/window-bar/returnd.svg" : "qrc:/QueMusic/resources/window-bar/return.svg"
                background: Rectangle {
                    color: returnButton.hovered ? Style.themes.hoverColor : "transparent"
                    radius: Style.settings.noControlRadius ? Style.settings.labelRadius : 8
                    Behavior on color { ColorAnimation { duration: 50 } }
                }
                onClicked: settingOutAnime.running = true;
                Component.onCompleted: windowAgent.setHitTestVisible(returnButton, true);
            }

            Label {
                x: 56
                y: 0
                width: 120
                height: 36
                text: "应用设置"
                font.pixelSize: 13
                font.bold: false
                verticalAlignment: Text.AlignVCenter
                color: Style.themes.fontColor
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }

        ListModel {
            id: navModelSettings
            ListElement { display: "通用"; iconChar: "\uf038" }
            ListElement { display: "界面"; iconChar: "\uf0a7" }
            ListElement { display: "功能"; iconChar: "\uf094" }
            ListElement { display: "播放"; iconChar: "\uf010" }
            ListElement { display: "快捷键"; iconChar: "\uf0c7" }
            ListElement { display: "插件"; iconChar: "\uf0ff" }
            ListElement { display: "关于"; iconChar: "\uf0b6" }
            ListElement { display: "Debug"; iconChar: "\uf060" }
        }

        Column {
            id: navListViewSettings
            x: 10
            y: 70
            width: 190
            height: settingsView.height - 80
            property int setChoiceIndex: 0

            spacing: 2
            z: 10
            Repeater {
                model: navModelSettings

                delegate: Rectangle {
                    id: navDelegateSettings
                    width: navListViewSettings.width
                    height: 42
                    radius: Style.settings.labelRadius
                    color: isSelected ? leftSidebarSettings.choiceColor : "transparent"

                    property bool isSelected: navListViewSettings.setChoiceIndex === index

                    scale: 1.0
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutCubic } }


                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Style.themes.hoverColor
                        opacity: barMouse.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }

                    Text {
                        x: 12
                        y: 0
                        z: 1
                        width: 42
                        height: 42
                        text: model.iconChar
                        font.family: iconFont.name
                        font.pixelSize: Style.settings.texticon
                        color: navDelegateSettings.isSelected ? leftSidebarSettings.choiceTextColor : Style.themes.textColor
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    Text {
                        x: 60
                        y: 0
                        z: 2
                        width: 140
                        height: 42
                        text: model.display
                        color: navDelegateSettings.isSelected ? leftSidebarSettings.choiceTextColor : Style.themes.textColor
                        font.bold: navDelegateSettings.isSelected
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }


                    MouseArea {
                        id: barMouse
                        anchors.fill: parent
                        hoverEnabled: true

                        onPressed: navDelegateSettings.scale = 0.96
                        onReleased: navDelegateSettings.scale = 1.0
                        onCanceled: navDelegateSettings.scale = 1.0
                        onClicked: {
                            leftSidebarSettings.index1ed(index)
                            navListViewSettings.setChoiceIndex = index
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Style.themes.primaryColor
    }

    Item {
        id: settingStack
        x: 210; y: 60
        width: parent.width - 210
        height: parent.height - 60
        z: 2
        clip: true
        property var setPages: [
            themeset,
            uiset,
            toolset,
            playerset,
            shortcutset,
            modset,
            aboutset,
            debugset
        ]
        property int setPageIndex: 0
        Connections {
            target: leftSidebarSettings
            function onIndex1ed(choice) {
                if(choice !== settingStack.setPageIndex) {
                    setPageAnine.stop()
                    setPageAnimeo.target = settingStack.setPages[choice]
                    setPageAnimey.target = settingStack.setPages[choice]
                    setPageAnine.start()
                    settingStack.setPages[settingStack.setPageIndex].visible = false
                    settingStack.setPages[choice].visible = true
                    settingStack.setPageIndex = choice
                }
            }
        }

        ParallelAnimation {
            id: setPageAnine
            NumberAnimation {
                id: setPageAnimeo
                property: "opacity"
                from: 0
                to: 1
                duration: 300
                easing.type: Easing.OutExpo
            }
            NumberAnimation {
                id: setPageAnimey
                property: "y"
                from: 120
                to: 0
                duration: 300
                easing.type: Easing.OutExpo
            }
        }

        property int standWidth: settingsView.width - 262

        // 通用设置
        QScrollView {
            id: themeset
            width: settingStack.width
            height: settingStack.height

            visible: true

            opacity: 1
            y: 0
            Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
            Behavior on y { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

            contentChildren: Column {
                id: themeContent
                spacing: 16
                padding: 24
                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "通用"
                    font.pixelSize: Style.settings.pageTitle
                }

                QHead { text: "全局主题" }

                Rectangle {
                    width: settingStack.standWidth
                    height: globalThemeCard.height
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        id: globalThemeCard
                        width: parent.width
                        padding: 0
                        //Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "设置全局主题"
                            isBigItem: true
                            controlItem: QWideDrop {
                                anchors.fill: parent
                                model: ["浅色主题","深色主题","跟随系统"]
                                choice: Style.settings.theme
                                onTransformed: (choiced) => {
                                    Style.settings.theme = choiced;
                                }
                            }
                        }

                        SettingItemCard {
                            label: "全局主题色"
                            controlItem: Item {
                                anchors.fill: parent
                                Row {
                                    height: 36
                                    anchors.right: parent.right
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 12

                                    Repeater {
                                        model: Style.settings.colorList
                                        delegate: SButton {
                                            width: 36
                                            height: 36
                                            radius: Style.settings.labelRadius
                                            borderWidth: Style.settings.color === index ? 2 : 0
                                            borderColor: Style.themes.textColor
                                            iconCharacter: ""
                                            buttonColor: modelData
                                            onClicked: {
                                                Style.settings.color = index;
                                                Style.changeTheme();
                                            }
                                        }
                                    }
                                    SButton {
                                        width: 36
                                        height: 36
                                        radius: Style.settings.labelRadius
                                        iconCharacter: "\uf08e"
                                        visible: Style.settings.colorList.length > 4
                                        buttonColor: "#fa6666"
                                        onClicked: {
                                            Style.settings.colorList.length -= 1;
                                        }
                                    }
                                    SButton {
                                        width: 36
                                        height: 36
                                        radius: Style.settings.labelRadius
                                        iconCharacter: "\uf008"
                                        visible: Style.settings.colorList.length < 8
                                        buttonColor: Style.themes.secondaryColor
                                        onClicked: {
                                            themeColorChoose.open();
                                        }
                                    }
                                }
                            }
                            ColorDialog {
                                id: themeColorChoose
                                onAccepted: {
                                    var toColor = Qt.hsva(selectedColor.hsvHue,0.9,0.9,1.0)
                                    Style.settings.colorList.push(toColor);
                                    mainWarn.tiped("成功添加一个主题颜色",1);
                                }
                            }
                        }

                        SettingItemCard {
                            label: "设置应用背景"
                            isBigItem: true
                            controlItem: QWideDrop {
                                anchors.fill: parent
                                model: ["默认","浅主题色","云母材质","图片","模糊窗口"]
                                choice: Style.settings.backmode
                                onTransformed: (choiced) => {
                                    Style.settings.backmode = choiced
                                    Style.changeUi()
                                    Style.changeTheme()
                                }
                            }
                        }

                        SettingItemCard {
                            label: "选择背景图片"
                            visible: Style.settings.backmode == 3
                            height: visible ? 56 : 0
                            Behavior on height { NumberAnimation { duration: 320; easing.type: Easing.OutExpo } }
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Style.settings.backpic
                                model: ["示例1","示例2(星空)","示例3","自定义图片"]
                                onTransformed: (choiced) => {
                                    Style.settings.backpic = choiced;
                                    Style.changeUi();
                                    Style.changeTheme();
                                    if(choiced === 3) {
                                        imagefileDialog.open();
                                    }
                                }
                            }
                            bottomLine: false
                            FileDialog {
                                id: imagefileDialog
                                title: "选择图片文件"
                                fileMode: FileDialog.OpenFile
                                nameFilters: ["图片文件 (*.jpg *.png *.jpeg *.pkm *.svg *.gif *.bmp *.tiff *.xbm *.xpm *.pbm *.pgm *.ppm)"]
                                onAccepted: {
                                    Style.settings.backgroundImage = imagefileDialog.selectedFile;
                                    mainWarn.tiped("成功设置背景图片",1);
                                    Style.changeUi();
                                    Style.changeTheme();
                                }
                            }
                        }
                    }
                }

                QHead { text: "应用程序" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "全局音量"
                            controlItem: QSlider {
                                anchors.fill: parent
                                to: 100
                                stepSize: 1
                                leftText: true
                                valueText: value
                                value: Math.floor(Options.settings.musicVolume * 100)
                                onMoved: {
                                    Options.settings.musicVolume = value / 100
                                }
                            }
                        }

                        SettingItemCard {
                            label: "默认下载目录"
                            controlItem: QInput {
                                anchors.fill: parent
                                inputText: "选择目录"
                                onEntered: {
                                    Options.settings.downloadFolder = inputText
                                }
                            }
                        }

                        SettingItemCard {
                            label: "关闭按钮最小化托盘"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.closeToManage
                                onToggled: Options.settings.closeToManage = !Options.settings.closeToManage
                            }
                        }

                        SettingItemCard {
                            label: "自动检查更新"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.autoUpdate
                                onToggled: Options.settings.autoUpdate = !Options.settings.autoUpdate
                            }
                        }

                        SettingItemCard {
                            label: "清除缓存"
                            controlItem: QButton {
                                anchors.fill: parent
                                shadowEnabled: false
                                buttonColor: "transparent"
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                text: "清理"
                            }
                            bottomLine: false
                        }
                    }
                }
            }
        }

        // 界面设置
        QScrollView {
            id: uiset
            width: settingStack.width
            height: settingStack.height

            visible: false

            contentChildren: Column {
                id: uiContent
                spacing: 16
                padding: 24

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "界面"
                    font.pixelSize: Style.settings.pageTitle
                }
                QHead { text: "界面样式" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "显示左栏背景"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.sidebarColor
                                onToggled: Style.settings.sidebarColor = !Style.settings.sidebarColor
                            }
                        }

                        SettingItemCard {
                            label: "左栏样式"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Style.settings.sidebarStyle
                                model: ["Basic","SquiwaUI"]
                                onTransformed: (choiced) => {
                                    Style.settings.sidebarStyle = choiced;
                                    Style.changeTheme();
                                }
                            }
                        }

                        SettingItemCard {
                            label: "组件圆角大小"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 0
                                to: 20
                                stepSize: 2
                                leftText: true
                                valueText: value
                                value: Style.settings.labelRadius
                                onMoved: {
                                    Style.settings.labelRadius = value
                                }
                            }
                        }

                        SettingItemCard {
                            label: "卡片圆角大小"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 0
                                to: 24
                                stepSize: 2
                                leftText: true
                                valueText: value
                                value: Style.settings.cubeRadius
                                onMoved: {
                                    Style.settings.cubeRadius = value
                                }
                            }
                        }

                        SettingItemCard {
                            label: "不使用控件大圆角"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.noControlRadius
                                onToggled: Style.settings.noControlRadius = !Style.settings.noControlRadius
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "界面效果" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "高质量模糊效果"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.highQualityBlur
                                onToggled: Style.settings.highQualityBlur = !Style.settings.highQualityBlur
                            }
                        }

                        SettingItemCard {
                            label: "组件阴影模糊度"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 8
                                to: 32
                                stepSize: 2
                                leftText: true
                                valueText: value
                                value: Style.settings.shadowSize
                                onMoved: {
                                    Style.settings.shadowSize = value
                                }
                            }
                        }

                        SettingItemCard {
                            label: "组件背景模糊度"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 16
                                to: 64
                                stepSize: 2
                                leftText: true
                                valueText: value
                                value: Style.settings.blurSize
                                onMoved: {
                                    Style.settings.blurSize = value
                                }
                            }
                        }

                        SettingItemCard {
                            label: "高级动画效果"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.premiumAnime
                                onToggled: Style.settings.premiumAnime = !Style.settings.premiumAnime
                            }
                        }

                        SettingItemCard {
                            label: "全局动画速率"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Style.settings.animeSpeed
                                model: ["优雅","默认","效率"]
                                onTransformed: (choiced) => {
                                    Style.settings.animeSpeed = choiced
                                }
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "页面布局" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "首页默认布局"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: 1
                                model: ["默认","竖向","混合"]
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "播放器歌词模式" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "标准歌词大小"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 0
                                to: 20
                                stepSize: 2
                                leftText: true
                                valueText: value
                                value: Style.settings.lyricSize
                                onMoved: {
                                    Style.settings.lyricSize = value
                                }
                            }
                        }

                        SettingItemCard {
                            label: "歌词字重"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 300
                                to: 800
                                stepSize: 50
                                leftText: true
                                valueText: value
                                value: Style.settings.textWidth
                                onMoved: {
                                    Style.settings.textWidth = value
                                }
                            }
                        }

                        SettingItemCard {
                            label: "歌词界面背景"
                            isBigItem: true
                            controlItem: QWideDrop {
                                anchors.fill: parent
                                model: ["动态流体","静态烘培","静态渐变"]
                                choice: Style.settings.backFlowQuality
                                onTransformed: (choiced) => {
                                    Style.settings.backFlowQuality = choiced
                                }
                            }
                        }

                        SettingItemCard {
                            label: "显示音波效果"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.waveDisplay
                                onToggled: Style.settings.waveDisplay = !Style.settings.waveDisplay
                            }
                        }

                        SettingItemCard {
                            label: "歌词渐进模糊"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.maskBlur
                                onToggled: Style.settings.maskBlur = !Style.settings.maskBlur
                            }
                        }

                        SettingItemCard {
                            label: "高级逐行弹簧动画"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.premiumLyricAnime
                                onToggled: Style.settings.premiumLyricAnime = !Style.settings.premiumLyricAnime
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "桌面音乐部件" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "动画速度"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: 1
                                model: ["默认","快","慢"]
                            }
                            bottomLine: false
                        }
                    }
                }
            }
        }

        // 功能设置
        QScrollView {
            id: toolset
            width: settingStack.width
            height: settingStack.height

            visible: false

            contentChildren: Column {
                id: funcContent
                spacing: 16
                padding: 24

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "功能"
                    font.pixelSize: Style.settings.pageTitle
                }

                QHead { text: "通用" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "默认缓存位置"
                            controlItem: QInput {
                                anchors.fill: parent
                                inputText: "选择目录"
                            }
                        }

                        SettingItemCard {
                            label: "默认数据存储位置"
                            controlItem: QInput {
                                anchors.fill: parent
                                inputText: "选择目录"
                            }
                        }

                        SettingItemCard {
                            label: "默认音质"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Options.settings.soundQuality
                                model: ["标准-144k","高清-320k","无损-500+k"]
                                onTransformed: (choiced) => {
                                    Options.settings.soundQuality = choiced
                                }
                            }
                        }

                        SettingItemCard {
                            label: "音频Data偏好"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Options.settings.metaDataSource
                                model: ["标准","高清","超清"]
                                onTransformed: (choiced) => {
                                    Options.settings.metaDataSource = choiced
                                }
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "在线服务" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "默认音乐源"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Options.settings.mainMusicSource
                                model: ["酷狗音乐","网易云音乐","QQ音乐"]
                                onTransformed: (choiced) => {
                                    Options.settings.mainMusicSource = choiced
                                }
                            }
                        }

                        SettingItemCard {
                            label: "代理服务器"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Options.settings.serverAgency
                                model: ["默认","系统http协议","自带协议","自定义"]
                                onTransformed: (choiced) => {
                                    Options.settings.serverAgency = choiced
                                }
                            }
                        }

                        SettingItemCard {
                            label: "缓存大小/MB"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 200
                                to: 1000
                                stepSize: 10
                                leftText: true
                                valueText: value
                                value: Options.settings.cacheSize
                                onMoved: {
                                    Options.settings.cacheSize = value
                                }
                            }
                            bottomLine: false
                        }
                    }
                }
            }
        }

        // 播放设置
        QScrollView {
            id: playerset
            width: settingStack.width
            height: settingStack.height

            visible: false

            contentChildren: Column {
                id: playContent
                spacing: 16
                padding: 24

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "播放"
                    font.pixelSize: Style.settings.pageTitle
                }

                QHead { text: "播放器" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "使用默认输出设备"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.useDefaultDevice
                                onToggled: Options.settings.useDefaultDevice = !Options.settings.useDefaultDevice
                            }
                        }

                        SettingItemCard {
                            label: "音频输出设备"
                            visible: Options.settings.useDefaultDevice === false
                            height: visible ? implicitHeight : 0
                            Behavior on height { NumberAnimation { duration: 320; easing.type: Easing.OutExpo } }
                            controlItem: QDrop {
                                useId: true
                                anchors.fill: parent
                                choice: Options.settings.audioDevice
                                model: musicDevices.audioOutputs
                                onTransformed: (choiced) => {
                                    Options.settings.audioDevice = choiced
                                }
                            }
                        }

                        SettingItemCard {
                            label: "音频播放比率/K"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 100
                                to: 1000
                                stepSize: 20
                                valueText: value
                                leftText: true
                                value: Options.settings.sampleRate
                                onMoved: {
                                    Options.settings.sampleRate = value
                                }
                            }
                        }

                        SettingItemCard {
                            label: "自动缓冲大小"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 100
                                to: 1000
                                stepSize: 20
                                valueText: value
                                leftText: true
                                value: Options.settings.bufferSize
                                onMoved: {
                                    Options.settings.bufferSize = value
                                }
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "播放器控制" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "自动播放"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.autoPlay
                                onToggled: Options.settings.autoPlay = !Options.settings.autoPlay
                            }
                        }

                        SettingItemCard {
                            label: "使用音频快速缓冲"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                onToggled: switchTrue = !switchTrue
                            }
                        }

                        SettingItemCard {
                            label: "播放器播放列表"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: 1
                                model: ["自动添加并使用列表控制","自动添加但控制到文件夹","不添加列表"]
                            }
                            bottomLine: false
                        }
                    }
                }
            }
        }

        // 快捷键设置
        QScrollView {
            id: shortcutset
            width: settingStack.width
            height: settingStack.height

            visible: false

            contentChildren: Column {
                id: shortcutContent
                spacing: 16
                padding: 24

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "快捷键"
                    font.pixelSize: Style.settings.pageTitle
                }

                Text { text: "目前本页的设置项无法使用,请等待更新"; color: Style.themes.fontColor; font.pixelSize: Style.settings.textmain }

                QHead { text: "全局快捷键" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "启用全局快捷键"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                onToggled: switchTrue = !switchTrue
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "快捷键列表" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    height: 400
                    clip: true

                    ListView {
                        id: shortcutListView
                        anchors.fill: parent
                        anchors.margins: 16
                        model: ListModel {
                            id: shortcutModel
                            ListElement { actionName: "play_pause"; description: "播放/暂停"; defaultKey: "Space"; currentKey: "Space" }
                            ListElement { actionName: "prev"; description: "上一首"; defaultKey: "Ctrl+Left"; currentKey: "Ctrl+Left" }
                            ListElement { actionName: "next"; description: "下一首"; defaultKey: "Ctrl+Right"; currentKey: "Ctrl+Right" }
                            ListElement { actionName: "vol_up"; description: "增大音量"; defaultKey: "Ctrl+Up"; currentKey: "Ctrl+Up" }
                        }
                        delegate: Row {
                            width: shortcutListView.width
                            height: 50
                            padding: 5
                            spacing: 20

                            Label {
                                text: model.description
                                width: 150
                                height: 40
                                font.pixelSize: Style.settings.textmain
                                color: Style.themes.fontColor
                                verticalAlignment: Text.AlignVCenter
                            }

                            Label {
                                text: "当前: " + model.currentKey
                                width: 150
                                height: 40
                                font.pixelSize: Style.settings.textmain
                                color: Style.themes.fontColor
                                verticalAlignment: Text.AlignVCenter
                            }

                            Label {
                                text: "默认: " + model.defaultKey
                                width: 150
                                height: 40
                                font.pixelSize: Style.settings.textmain
                                color: Qt.rgba(Style.themes.fontColor.r, Style.themes.fontColor.g, Style.themes.fontColor.b, 0.6)
                                verticalAlignment: Text.AlignVCenter
                            }

                            Item {
                                height: 40
                                visible: false
                                width: settingStack.standWidth - 674
                            }

                            QButton {
                                text: "设置"
                                shadowEnabled: false
                                width: 96
                                height: 40
                                buttonColor: "transparent"
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                onClicked: {
                                    //signalCenter.shortcutChanged(model.actionName, "New Key");
                                }
                            }
                            QButton {
                                text: "重置"
                                shadowEnabled: false
                                width: 96
                                height: 40
                                buttonColor: "transparent"
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                onClicked: {
                                    //signalCenter.shortcutChanged(model.actionName, model.defaultKey);
                                }
                            }
                        }
                    }
                }
            }
        }

        // 插件设置
        Item {
            id: modset
            width: settingStack.width
            height: settingStack.height
            visible: false

            Text {
                x: 24
                y: 24
                width: settingStack.standWidth
                height: 36
                color: Style.themes.fontColor
                verticalAlignment: Text.AlignVCenter
                text: "插件"
                font.pixelSize: Style.settings.pageTitle
            }

            Text { text: "目前本页的设置项无法使用,请等待更新"; color: Style.themes.fontColor; font.pixelSize: Style.settings.textmain }

            QBlurTapBar {
                x: 24
                y: 70
                z: 5
                model: ["外观类","功能类","音源"]
                tabWidth: 100
                width: 304
                rectXy: Qt.rect(0, 10, width, 40)
                blurSource: downloadChildPage
                onTabChange: (index) => {
                    downloadChildPage.stack(index)
                }
            }

            QPages {
                x: 24
                y: 60
                width: parent.width - 48
                height: parent.height - 60
                id: downloadChildPage
                pageList: [uiMod,toolMod,musicMod]
                Item {
                    id: uiMod
                    visible: true
                    width: downloadChildPage.width
                    height: downloadChildPage.height
                    Text {
                        anchors.fill: parent
                        text: "外观类"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        color: Style.themes.textColor
                        font.pixelSize: 14
                    }
                }
                Item {
                    id: toolMod
                    visible: false
                    width: downloadChildPage.width
                    height: downloadChildPage.height
                    Text {
                        anchors.fill: parent
                        text: "功能类"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        color: Style.themes.textColor
                        font.pixelSize: 14
                    }
                }
                Item {
                    id: musicMod
                    visible: false
                    width: downloadChildPage.width
                    height: downloadChildPage.height
                    Text {
                        anchors.fill: parent
                        text: "音源\n并不建议使用音源，以防止出现的版权问题和违规获取"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        color: Style.themes.textColor
                        font.pixelSize: 14
                    }
                }
            }
        }

        // 关于页面
        QScrollView {
            id: aboutset
            width: parent.width
            height: parent.height
            visible: false
            clip: false

            contentChildren: Column {
                id: aboutCol
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 16
                padding: 24

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "关于应用"
                    font.pixelSize: Style.settings.pageTitle
                }

                QPicture {
                    id: logoSection
                    width: settingStack.standWidth
                    height: 420
                    radius: Style.settings.cubeRadius
                    source: "qrc:/QueMusic/resources/pic/back2.jpg"

                    sourceSize: Qt.size(1424,750)

                    Row {
                        anchors.centerIn: parent
                        spacing: 32
                        //icon
                        Image {
                            width: 96
                            height: 96
                            source: "qrc:/QueMusic/resources/icon.ico"
                            sourceSize: Qt.size(96, 96)
                        }

                        // App Title
                        Text {
                            height: 96
                            text: "QueMusic"
                            font.family: textFont.name
                            font.pixelSize: 64
                            font.bold: false
                            verticalAlignment: Text.AlignVCenter
                            color: Style.themes.fullColor
                        }
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 20
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 240
                        height: 50
                        radius: 25
                        color: Qt.rgba(1, 1, 1, 0.3)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.6)

                        Row {
                            anchors.centerIn: parent
                            spacing: 15
                            Label {
                                text: "版本: " + window.version
                                font.pixelSize: Style.settings.textmain
                                color: "white"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QButton {
                                height: 36
                                width: 100
                                radius: 18
                                text: "检查更新"
                                anchors.verticalCenter: parent.verticalCenter
                                onClicked: {
                                    signalCenter.checkForUpdatesRequested();
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    width: settingStack.standWidth
                    height: warnText.implicitHeight + 48
                    color: Style.darkis ? "#888826" : "#fafaaa"
                    radius: Style.settings.cubeRadius
                    border.color: "#bbbb38"
                    border.width: 2
                    Text {
                        x: 24
                        y: 24
                        font.family: iconFont.name
                        height: warnText.implicitHeight
                        text: "\uf11a"
                        color: Style.themes.textColor
                        font.pixelSize: Style.settings.texticon
                    }
                    Text {
                        id: warnText
                        x: 48
                        y: 24
                        text: "该版本属于开发中Beta版本，是未正式发布的开发中测试版本，部分功能仍未有效，并且稳定性欠佳，非最终质量"
                        elide: Text.ElideRight
                        color: Style.themes.textColor
                        font.bold: false
                        font.pixelSize: Style.settings.textmain
                    }
                    SButton {
                        iconCharacter: "\uf025"
                        x: parent.width - 52
                        anchors.verticalCenter: parent.verticalCenter
                        width: 36
                        height: 36
                        radius: 18
                        iconSize: Style.settings.texticon
                        buttonColor: "transparent"
                        shadowEnabled: false
                        onClicked: {
                            parent.visible = false;
                        }
                    }
                }

                QHead { text: "开发者" }
                Row {
                    spacing: 24
                    width: settingStack.standWidth
                    height: 80
                    Rectangle {
                        width: settingStack.standWidth / 2 - 12
                        height: 80
                        radius: 16
                        color: Style.themes.fullColor
                        border.color: Style.themes.secondaryColor
                        border.width: 2
                        Image {
                            x: 15
                            y: 15
                            source: "qrc:/QueMusic/resources/app/header.png"
                            width: 50
                            height: 50
                            sourceSize.width: 50
                            sourceSize.height: 50
                            fillMode: Image.PreserveAspectFit
                        }
                        Text {
                            x: 80
                            y: 15
                            height: 50
                            text: "BroNekoX"
                            color: Style.themes.fontColor
                            verticalAlignment: Text.AlignVCenter
                            font.bold: true
                            font.pixelSize: 18
                        }
                    }
                }

                QHead { text: "版本信息" }

                Rectangle {
                    width: settingStack.standWidth
                    height: 244
                    radius: 16
                    color: Style.themes.fullColor
                    border.color: Style.themes.secondaryColor
                    border.width: 2
                    Column {
                        spacing: 8
                        padding: 16
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "软件版本"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: window.version
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "Qt框架版本"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "Qt-Community-6.9.3"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "MinGW架构版本"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "MinGW-13.1.0-64Bit"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "Cmake版本"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "3.30.5"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "QWindowKit版本"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "1.5.1.0-2606"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                QHead { text: "技术践实现" }

                Rectangle {
                    width: settingStack.standWidth
                    height: 288
                    radius: 16
                    color: Style.themes.fullColor
                    border.color: Style.themes.secondaryColor
                    border.width: 2
                    Column {
                        spacing: 8
                        padding: 16
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "主体架构"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "Qt QML Engine"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "开发工具"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "QtCreator"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "渲染架构"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "Qml RHI graph"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "前端编程语言"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "QML/JS"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "后端编程语言"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "C++/SqlLite"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "编译器与包管理"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "Cmake-MinGW"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                QHead { text: "使用第三方项目与库" }

                Rectangle {
                    width: settingStack.standWidth
                    height: 150
                    radius: 16
                    color: Style.themes.fullColor
                    border.color: Style.themes.secondaryColor
                    border.width: 2
                    Column {
                        spacing: 8
                        padding: 16
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "QWindowKit"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "实现完美和贴近系统的无边框窗口管理"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "Qt6.9.3-Community(开源版)"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "UI和软件框架及后端和媒体都来源于它"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "pako.js"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "用于解码一些加密的歌词"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "Poppins Feather font"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "分别用于字体和图标的font渲染"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                QHead { text: "联系开发者" }

                Rectangle {
                    width: settingStack.standWidth
                    height: 200
                    color: Style.themes.fullColor
                    radius: 16
                    border.color: Style.themes.secondaryColor
                    border.width: 2
                    Column {
                        spacing: 8
                        padding: 16
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "QQ"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "241422517"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "邮箱/Email"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "uihugd@outlook.com"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "Bilibili"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "?????"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "????"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "?????"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Text { text: "期待您的贡献/反馈/加入"; color: Style.themes.fontColor; font.pixelSize: Style.settings.textmain }

                Row {
                    spacing: 20

                    QButton {
                        height: 40
                        radius: 20
                        width: 140
                        text: "Source"
                        iconCharacter: "\uf060"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/bronekox/quemusic");
                        }
                    }

                    QButton {
                        height: 40
                        radius: 20
                        width: 140
                        text: "Website(未推出)"
                        iconCharacter: "\uf0d7"
                        onClicked: {
                            Qt.openUrlExternally("example.com");
                        }
                    }

                    QButton {
                        height: 40
                        radius: 20
                        width: 140
                        text: "Bug反馈"
                        iconCharacter: "\uf117"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/bronekox/quemusic/issues");
                        }
                    }
                }
            }
        }

        // Debug
        QScrollView {
            id: debugset
            width: settingStack.width
            height: settingStack.height

            visible: false

            contentChildren: Column {
                id: deBug
                spacing: 16
                padding: 24

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "DeBug"
                    font.pixelSize: Style.settings.pageTitle
                }

                Text { text: "目前本页的设置项无法使用,请等待更新"; color: Style.themes.fontColor; font.pixelSize: Style.settings.textmain }

                QHead { text: "渲染" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "使用系统标题栏"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.noWindowKit
                                onToggled: Options.settings.noWindowKit = !Options.settings.noWindowKit
                            }
                        }

                        SettingItemCard {
                            label: "界面渲染引擎"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Options.settings.gpuRenderMode
                                model: ["系统偏好","OpenGL","Vulkan","软件"]
                                onTransformed: (choiced) => {
                                    Options.settings.gpuRenderMode = choiced
                                }
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "调试" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.darkis ? Style.themes.secondaryColor : Style.themes.fullColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.secondaryColor
                    border.width: 1
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "显示渲染帧率"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.displayFps
                                onToggled: Options.settings.displayFps = !Options.settings.displayFps
                            }
                        }

                        SettingItemCard {
                            label: "调试模式"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.debug
                                onToggled: Options.settings.debug = !Options.settings.debug
                            }
                        }

                        SettingItemCard {
                            label: "显示信息控制台"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.displayDebugControl
                                onToggled: Options.settings.displayDebugControl = !Options.settings.displayDebugControl
                            }
                            bottomLine: false
                        }
                    }
                }
            }
        }
    }
}