// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Window
import QtCore
import QueMusic 1.0

Item {
    id: settingsView


    // 账号登录面板展开状态
    property bool neteaseShowLogin: false
    property bool kugouShowLogin: false
    property string neteaseLoginStatus: "等待登录…"
    property string kugouLoginStatus: "等待登录…"

    // 登录成功自动收起面板
    Connections {
        target: accountManager
        function onNeteaseLoginChanged() {
            if (accountManager.neteaseLoggedIn) {
                settingsView.neteaseShowLogin = false;
                neteaseQrDialog.close();
            }
        }
        function onKugouLoginChanged() {
            if (accountManager.kugouLoggedIn) {
                settingsView.kugouShowLogin = false;
                kugouQrDialog.close();
            }
        }
    }

    // 网易云扫码登录弹窗（QOptionDialog + QRCodeView）
    QOptionDialog {
        id: neteaseQrDialog
        title: "网易云音乐 - 扫码登录"
        cancelText: "取消"
        confirmText: "关闭"
        dialogContentHeight: 330
        blurSource: settingsView
        onCancel: { accountManager.cancelNeteaseQrLogin(); settingsView.neteaseShowLogin = false; }
        onConfirm: { accountManager.cancelNeteaseQrLogin(); settingsView.neteaseShowLogin = false; }
        onClosed: { accountManager.cancelNeteaseQrLogin(); settingsView.neteaseShowLogin = false; }

        Column {
            width: parent.width
            spacing: 14
            Rectangle {
                width: 232
                height: 232
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 12
                color: Style.darkis ? Style.themes.secondaryColor : "#ffffff"
                border.color: Style.themes.secondaryColor
                border.width: 1
                QRCodeView {
                    id: neteaseQrCode
                    width: 212
                    height: 212
                    anchors.centerIn: parent
                    qrText: accountManager.neteaseQrText
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: accountManager.neteaseQrMessage
                color: Style.themes.fontColor
                font.pixelSize: Style.settings.textmain
                elide: Text.ElideRight
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }
            QButton {
                visible: accountManager.neteaseQrState === 3 || accountManager.neteaseQrState === 4
                anchors.horizontalCenter: parent.horizontalCenter
                text: "重新获取二维码"
                width: 150
                height: 34
                radius: 17
                shadowEnabled: false
                buttonColor: Style.themes.themeColor
                textColor: Style.themes.primaryColor
                onClicked: accountManager.startNeteaseQrLogin()
            }
        }
    }

    // 酷狗扫码登录弹窗（QOptionDialog + QRCodeView）
    QOptionDialog {
        id: kugouQrDialog
        title: "酷狗音乐 - 扫码登录"
        cancelText: "取消"
        confirmText: "关闭"
        dialogContentHeight: 330
        blurSource: settingsView
        onCancel: { accountManager.cancelKugouQrLogin(); settingsView.kugouShowLogin = false; }
        onConfirm: { accountManager.cancelKugouQrLogin(); settingsView.kugouShowLogin = false; }
        onClosed: { accountManager.cancelKugouQrLogin(); settingsView.kugouShowLogin = false; }

        Column {
            width: parent.width
            spacing: 14
            Rectangle {
                width: 232
                height: 232
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 12
                color: Style.darkis ? Style.themes.secondaryColor : "#ffffff"
                border.color: Style.themes.secondaryColor
                border.width: 1
                QRCodeView {
                    id: kugouQrCode
                    width: 212
                    height: 212
                    anchors.centerIn: parent
                    qrText: accountManager.kugouQrText
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: accountManager.kugouQrMessage
                color: Style.themes.fontColor
                font.pixelSize: Style.settings.textmain
                elide: Text.ElideRight
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }
            QButton {
                visible: accountManager.kugouQrState === 3 || accountManager.kugouQrState === 4
                anchors.horizontalCenter: parent.horizontalCenter
                text: "重新获取二维码"
                width: 150
                height: 34
                radius: 17
                shadowEnabled: false
                buttonColor: Style.themes.themeColor
                textColor: Style.themes.primaryColor
                onClicked: accountManager.startKugouQrLogin()
            }
        }
    }

    FolderDialog {
        id: downloadFolderDialog
        title: "选择默认下载目录"
        currentFolder: Options.settings.downloadFolder || StandardPaths.writableLocation(StandardPaths.MusicLocation)
        onAccepted: {
            var p = downloadFolderDialog.selectedFolder.toString();
            if (p.indexOf("file:///") === 0)
                p = p.substring(8);
            Options.settings.downloadFolder = p;
            mainWarn.tiped("已设置默认下载目录", 1);
        }
    }

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
        color: Style.settings.sidebarColor ? Style.themes.secondaryColor : Style.themes.primaryColor
        Connections {
            target: Style
            function onChangeTheme() {
                if(Style.settings.sidebarStyle === 0) {
                    leftSidebarSettings.choiceColor = Style.themes.hoverColor;
                    leftSidebarSettings.choiceTextColor = Style.themes.fontColor;
                    choicebar1.x = 18;
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
                choicebar1.x = 18;
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
            x: 18
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
            x: 15
            y: 70
            width: 180
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
                        x: 8
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
                        x: 56
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
        id: settingStack
        x: 210
        //x: parent.width > 1410 ? parent.width / 2 - 495 : 210; y: 0
        //width: parent.width > 1410 ? 1200 : parent.width - 210
        width: parent.width - 210
        height: parent.height
        color: Style.themes.secondaryColor
        z: 2
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
                duration: 320
                easing.type: Easing.OutExpo
            }
            NumberAnimation {
                id: setPageAnimey
                property: "y"
                from: 240
                to: 60
                duration: 320
                easing.type: Easing.OutExpo
            }
        }

        property int standWidth: parent.width > 1410 ? 1148 : parent.width - 262
        property int containWidth: parent.width > 1410 ? 1200 : parent.width - 210
        property int containX: parent.width > 1410 ? parent.width / 2 - 705 : 0

        // 通用设置
        QScrollView {
            id: themeset
            width: settingStack.width
            height: settingStack.height - 60
            y: 60
            visible: true
            opacity: 1
            Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
            Behavior on y { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }

            contentChildren: Column {
                id: themeContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX
                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "通用"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                QHead { text: "全局主题" }

                Rectangle {
                    width: settingStack.standWidth
                    height: globalThemeCard.height
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                                    Style.changeTheme();
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
                                    var toColor = Qt.hsva(selectedColor.hsvHue,0.9,0.8,1.0)
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
                                model: ["示例1","示例2","自定义图片"]
                                onTransformed: (choiced) => {
                                    Style.settings.backpic = choiced;
                                    Style.changeUi();
                                    Style.changeTheme();
                                    if(choiced === 2) {
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

                QHead { text: "账号与登录" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        topPadding: 12
                        Component.onCompleted: parent.height = height
                        spacing: 12

                        // 合规说明
                        Rectangle {
                            x: 16
                            width: parent.width - 32
                            height: 46
                            color: Style.themes.hoverColor
                            radius: Style.settings.labelRadius
                            Text {
                                x: 12
                                anchors.verticalCenter: parent.verticalCenter
                                width: 30
                                text: "\uf0b6"
                                font.family: iconFont.name
                                color: Style.themes.textColor
                                font.pixelSize: Style.settings.texticon
                            }
                            Text {
                                x: 44
                                width: parent.width - 56
                                anchors.verticalCenter: parent.verticalCenter
                                text: "扫码登录：用你自己的账号在应用内登录（账号即能力），登录后自动读取登录态，仅保存在本机，随时可退出。"
                                color: Style.themes.textColor
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }

                        Row {
                            x: 16
                            width: parent.width - 32
                            height: 144
                            spacing: 14
                            PlatformCard {
                                text: "酷狗音乐"
                                chooseColor: "#4384F5"
                                chooseColor1: Style.darkis ? "#153A57" : "#CDE8FF"
                                width: settingStack.standWidth / 3 - 20
                                height: 128
                                choose: MusicApi.songSource === 0
                                isLogin: accountManager.kugouLoggedIn
                                header: accountManager.kugouAvatar
                                name: accountManager.kugouNickname
                                onClicked: MusicApi.songSource = 0;
                                onLogined: {
                                    if (accountManager.kugouLoggedIn) {
                                        globalDialog.openSimpleDialog("警告", "是否退出账号？",
                                            function() {
                                                accountManager.logoutKugou();
                                            }
                                        )
                                    } else {
                                        settingsView.kugouShowLogin = true;
                                        kugouQrDialog.open();
                                        accountManager.startKugouQrLogin();
                                    }
                                }
                            }
                            PlatformCard {
                                text: "网易云音乐"
                                chooseColor: "#F54343"
                                chooseColor1: Style.darkis ? "#601515" : "#FFCDCD"
                                width: settingStack.standWidth / 3 - 20
                                height: 128
                                choose: MusicApi.songSource === 1
                                isLogin: accountManager.neteaseLoggedIn
                                header: accountManager.neteaseAvatar
                                name: accountManager.neteaseNickname
                                onClicked: MusicApi.songSource = 1;
                                onLogined: {
                                    if (accountManager.neteaseLoggedIn) {
                                        globalDialog.openSimpleDialog("警告", "是否退出账号？",
                                            function() {
                                                accountManager.logoutNetease();
                                            }
                                        )
                                    } else {
                                        settingsView.neteaseShowLogin = true;
                                        neteaseQrDialog.open();
                                        accountManager.startNeteaseQrLogin();
                                    }
                                }
                            }
                            PlatformCard {
                                text: "QQ音乐"
                                chooseColor: "#3AD630"
                                chooseColor1: Style.darkis ? "#195319" : "#CDFFCD"
                                width: settingStack.standWidth / 3 - 20
                                height: 128
                                choose: MusicApi.songSource === 2
                                isLogin: false
                                name: "暂不支持"
                                onClicked: MusicApi.songSource = 2;
                                onLogined: {
                                    mainWarn.tiped("目前无法使用", 0);
                                }
                            }
                        }
                    }
                }

                QHead { text: "应用程序" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                            controlItem: QButton {
                                anchors.fill: parent
                                shadowEnabled: false
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                text: Options.settings.downloadFolder ? "自定义目录" : "系统音乐文件夹"
                                fontSize: Style.settings.text
                                onClicked: downloadFolderDialog.open()
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
                            label: "自动检查更新(x)"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.autoUpdate
                                onToggled: Options.settings.autoUpdate = !Options.settings.autoUpdate
                            }
                        }

                        SettingItemCard {
                            label: "清除图片缓存"
                            controlItem: QButton {
                                anchors.fill: parent
                                shadowEnabled: false
                                buttonColor: "transparent"
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                text: "清理"
                                onClicked: {
                                    coverHelper.clearCache();
                                    Style.warned("成功清除图片缓存",1);
                                }
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
            height: settingStack.height - 60

            visible: false

            contentChildren: Column {
                id: uiContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "界面"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }
                QHead { text: "界面样式" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "左栏融合背景"
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
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "高质量模糊效果(x)"
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
                            label: "高级动画效果(x)"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.premiumAnime
                                onToggled: Style.settings.premiumAnime = !Style.settings.premiumAnime
                            }
                        }

                        SettingItemCard {
                            label: "全局动画速率(x)"
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
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "首页默认布局(x)"
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
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                            label: "歌词字体"
                            controlItem: QButton {
                                anchors.fill: parent
                                shadowEnabled: false
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                text: Style.settings.fontFamily ? Style.settings.fontFamily : "系统默认"
                                fontSize: Style.settings.text
                                onClicked: lyricFontDialog.open()
                            }
                            FontDialog {
                                id: lyricFontDialog
                                currentFont.family: Style.settings.fontFamily || null
                                onAccepted: Style.settings.fontFamily = lyricFontDialog.selectedFont.family
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
                        }

                        SettingItemCard {
                            label: "自动进入沉浸模式"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.lyricHideGui
                                onToggled: Style.settings.lyricHideGui = !Style.settings.lyricHideGui
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "桌面音乐部件" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "动画速度(x)"
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
            height: settingStack.height - 60

            visible: false

            contentChildren: Column {
                id: funcContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "功能"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                QHead { text: "通用" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "默认缓存位置(x)"
                            controlItem: QInput {
                                anchors.fill: parent
                                inputText: "选择目录"
                            }
                        }

                        SettingItemCard {
                            label: "默认数据存储位置(x)"
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
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                            label: "代理服务器(x)"
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
                            label: "缓存大小/MB(x)"
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
            height: settingStack.height - 60

            visible: false

            contentChildren: Column {
                id: playContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "播放"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                QHead { text: "播放器" }

                Rectangle {
                    width: settingStack.standWidth
                    height: playerColumn.height
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        id: playerColumn
                        width: parent.width
                        padding: 0

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
                            height: visible ? 56 : 0
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
                            label: "音频播放比率/K(x)"
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
                            label: "自动缓冲大小(x)"
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
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                            label: "使用音频快速缓冲(x)"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                onToggled: switchTrue = !switchTrue
                            }
                        }

                        SettingItemCard {
                            label: "播放器播放列表(x)"
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
            height: settingStack.height - 60
            visible: false

            // 动作定义（名称、显示描述、默认键位）
            property var actionDefs: [
                { name: "play", desc: "播放/暂停", default: "Space" },
                { name: "back", desc: "上一首", default: "Left" },
                { name: "forward", desc: "下一首", default: "Right" },
                { name: "playList", desc: "打开/关闭播放列表", default: "Alt" },
                { name: "musicControl", desc: "音乐控制面板", default: "Up" }
                // 如需添加更多，请在此增加条目，并确保 Options.shortCuts 中存在对应属性
            ]
            ListModel {
                id: actionDefs
                ListElement { name: "play"; desc: "播放/暂停"; defau: "Space" }
                ListElement { name: "back"; desc: "上一首"; defau: "Left" }
                ListElement { name: "forward"; desc: "下一首"; defau: "Right" }
                ListElement { name: "playList"; desc: "打开/关闭播放列表"; defau: "Alt" }
                ListElement { name: "musicControl"; desc: "音乐控制面板"; defau: "Up" }
            }

            // 录制状态
            property string recordingAction: ""
            property bool isRecording: false
            property bool oldShortCutState: false

            // 按键转字符串（辅助函数）
            function keyToString(key) {
                if (key >= Qt.Key_F1 && key <= Qt.Key_F35)
                    return "F" + (key - Qt.Key_F1 + 1)
                switch (key) {
                    case Qt.Key_Escape: return "Esc"
                    case Qt.Key_Return: return "Enter"
                    case Qt.Key_Backspace: return "Backspace"
                    case Qt.Key_Tab: return "Tab"
                    case Qt.Key_Space: return "Space"
                    case Qt.Key_Left: return "Left"
                    case Qt.Key_Right: return "Right"
                    case Qt.Key_Up: return "Up"
                    case Qt.Key_Down: return "Down"
                    case Qt.Key_Insert: return "Insert"
                    case Qt.Key_Delete: return "Delete"
                    case Qt.Key_Home: return "Home"
                    case Qt.Key_End: return "End"
                    case Qt.Key_PageUp: return "PageUp"
                    case Qt.Key_PageDown: return "PageDown"
                    default:
                        if (key >= Qt.Key_A && key <= Qt.Key_Z)
                            return String.fromCharCode(key)
                        else if (key >= Qt.Key_0 && key <= Qt.Key_9)
                            return String.fromCharCode(key)
                        else
                            return "" // 不支持的键
                }
            }

            function keyEventToSequence(event) {
                var modifiers = []
                if (event.modifiers & Qt.ControlModifier) modifiers.push("Ctrl")
                if (event.modifiers & Qt.AltModifier) modifiers.push("Alt")
                if (event.modifiers & Qt.ShiftModifier) modifiers.push("Shift")
                var key = event.key
                // 忽略单独的修饰键
                if (key === Qt.Key_Control || key === Qt.Key_Alt || key === Qt.Key_Shift || key === Qt.Key_Meta)
                    return ""
                var keyName = keyToString(key)
                if (!keyName) return ""
                var seq = modifiers.join("+")
                if (seq && keyName) seq += "+"
                seq += keyName
                return seq
            }

            // 开始录制
            function startRecording(action) {
                if (isRecording) return
                recordingAction = action
                isRecording = true
                oldShortCutState = Options.settings.openShortCut
                Options.settings.openShortCut = false   // 暂时禁用全局快捷键，避免干扰
                keyCapture.forceActiveFocus()
                keyCapture.focus = true;
                mainWarn.tiped("按下新的快捷键... (按 Esc 取消)", 0)
            }

            // 停止录制（完成或取消）
            function stopRecording(success, sequence) {
                isRecording = false
                Options.settings.openShortCut = oldShortCutState
                if (success && sequence) {
                    Options.shortCuts[recordingAction] = sequence
                    mainWarn.tiped("已设置快捷键: " + sequence, 1)
                } else {
                    mainWarn.tiped("已取消录制", 1)
                }
                recordingAction = ""
                keyCapture.focus = false
            }

            // 内容
            contentChildren: Column {
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "快捷键"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                // 全局开关
                QHead { text: "全局快捷键" }
                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "启用全局快捷键"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.openShortCut
                                onToggled: Options.settings.openShortCut = !Options.settings.openShortCut
                            }
                            bottomLine: false
                        }
                    }
                }

                // 快捷键列表
                QHead { text: "快捷键列表" }
                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    height: shortCutColumn.height // 自适应高度
                    clip: true

                    Column {
                        id: shortCutColumn
                        width: parent.width
                        padding: 0
                        Repeater {
                            model: actionDefs
                            delegate: Rectangle {
                                width: shortCutColumn.width
                                height: 56
                                color: "transparent"
                                radius: Style.settings.labelRadius
                                Row {
                                    spacing: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: 16

                                    Label {
                                        text: model.desc
                                        width: 150
                                        font.pixelSize: Style.settings.textmain
                                        color: Style.themes.fontColor
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: Options.shortCuts[model.name] || model.defau
                                        width: 120
                                        font.pixelSize: Style.settings.textmain
                                        color: Style.themes.fontColor
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: "默认: " + model.defau
                                        width: 120
                                        font.pixelSize: Style.settings.textmain
                                        color: Qt.rgba(Style.themes.fontColor.r, Style.themes.fontColor.g, Style.themes.fontColor.b, 0.6)
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                }
                                QButton {
                                    x: parent.width - 224
                                    y: 10
                                    width: 96
                                    height: 36
                                    text: "设置"
                                    shadowEnabled: false
                                    radius: Style.settings.labelRadius
                                    borderWidth: 2
                                    buttonColor: "transparent"
                                    onClicked: shortcutset.startRecording(model.name)
                                }

                                QButton {
                                    x: parent.width - 112
                                    y: 10
                                    width: 96
                                    height: 36
                                    text: "重置"
                                    shadowEnabled: false
                                    radius: Style.settings.labelRadius
                                    borderWidth: 2
                                    buttonColor: "transparent"
                                    onClicked: {
                                        Options.shortCuts[model.name] = model.defau
                                        mainWarn.tiped("已恢复默认快捷键", 1)
                                    }
                                }
                            }
                        }
                    }
                }

                // 底部提示
                Text {
                    width: settingStack.standWidth
                    text: "提示：点击「设置」后按下新的组合键（如 Ctrl+Shift+A），按 Esc 取消。"
                    color: Style.themes.textColor
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
            }
            // 按键捕获器（隐藏）
            Rectangle {
                id: keyCapture
                anchors.horizontalCenter: parent.horizontalCenter
                y: parent.height - 56
                height: 40
                width: keyCaptureText.width + 135
                color: Style.themes.fontColor
                border.width: 1
                border.color: Style.themes.sideColor
                radius: 18
                focus: false
                visible: true          // 必须可见才能获得焦点
                opacity: shortcutset.isRecording ? 1 : 0 // 透明，不干扰界面
                enabled: shortcutset.isRecording   // 仅在录制时启用
                Behavior on opacity { NumberAnimation { duration: 240 } }
                Keys.onPressed: (event) => {
                    if (!shortcutset.isRecording) return;
                    // 按 Esc 取消
                    if (event.key === Qt.Key_Escape) {
                        shortcutset.stopRecording(false)
                        event.accepted = true
                        mainWarn.tiped("已取消", 0)
                        return
                    }
                    var seq = shortcutset.keyEventToSequence(event)
                    if (seq) {
                        shortcutset.stopRecording(true, seq)
                        mainWarn.tiped("设置成功！", 1)
                        event.accepted = true
                    }
                    // 如果是无效键（如单独的修饰键），不处理，等待有效组合
                }
                QButton {
                    x: 3
                    y: 3
                    width: 100
                    height: 34
                    text: "取消[Esc]"
                    buttonColor: Style.themes.primaryColor
                    borderWidth: 1
                    onClicked: {
                        shortcutset.stopRecording(false);
                        mainWarn.tiped("已取消", 0);
                    }
                }
                Text {
                    id: keyCaptureText
                    x: 119
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: Style.settings.textmain
                    color: Style.themes.secondaryColor
                    text: "请输入一个键来设置" + shortcutset.recordingAction + "功能的快捷键"
                }
            }
            Connections {
                target: window
                function onExit() {
                    shortcutset.stopRecording(false);
                    mainWarn.tiped("已取消", 0);
                }
            }
        }

        // 插件设置
        Item {
            id: modset
            width: settingStack.width
            height: settingStack.height - 60
            visible: false

            Text {
                x: 24 + settingStack.containX
                y: 24
                width: settingStack.standWidth
                height: 36
                color: Style.themes.fontColor
                verticalAlignment: Text.AlignVCenter
                text: "插件"
                font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
            }

            QBlurTapBar {
                x: 24 + settingStack.containX
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

            Rectangle {
                x: 24 + settingStack.containX
                y: 124
                width: settingStack.standWidth
                height: warnModText.implicitHeight + 48
                color: Style.themes.containColor
                radius: Style.settings.cubeRadius
                border.color: Style.themes.sideColor
                border.width: 1
                Text {
                    x: 24
                    y: 24
                    font.family: iconFont.name
                    height: warnModText.implicitHeight
                    text: "\uf11a"
                    color: Style.themes.themeColor
                    font.pixelSize: Style.settings.texticon
                }
                Text {
                    id: warnModText
                    x: 48
                    y: 24
                    width: parent.width - 64
                    text: "插件功能还未开发完成，等待开发者更新喵"
                    wrapMode: Text.Wrap
                    color: Style.themes.textColor
                    font.bold: false
                    font.pixelSize: Style.settings.textmain
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
            height: settingStack.height - 60
            visible: false
            clip: false

            contentChildren: Column {
                id: aboutCol
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "关于应用"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
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
                            width: 80
                            height: 80
                            source: "qrc:/QueMusic/resources/icon.ico"
                            sourceSize: Qt.size(96, 96)
                        }

                        // App Title
                        Text {
                            height: 80
                            width: implicitWidth + 96
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
                        width: 260
                        height: 50
                        radius: 25
                        color: Qt.rgba(1, 1, 1, 0.5)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.6)

                        Row {
                            anchors.centerIn: parent
                            spacing: 15
                            Label {
                                text: " 版本: " + window.version + "-" + window.versionCode
                                font.pixelSize: Style.settings.textmain
                                color: "black"
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            QButton {
                                height: 36
                                width: 100
                                radius: 18
                                text: "检查更新"
                                anchors.verticalCenter: parent.verticalCenter
                                onClicked: {
                                    updater.checkForUpdate();
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    width: settingStack.standWidth
                    height: warnText.implicitHeight + 40
                    color: Style.themes.containColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.sideColor
                    border.width: 1
                    Text {
                        x: 20
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: iconFont.name
                        height: warnText.implicitHeight
                        text: "\uf11a"
                        color: Style.themes.themeColor
                        font.pixelSize: Style.settings.texticon
                    }
                    Text {
                        id: warnText
                        x: 48
                        y: 20
                        width: parent.width - 108
                        text: "该版本属于开发中Beta版本，是未正式发布的开发中测试版本，部分功能仍未有效，并且稳定性欠佳，非最终质量"
                        wrapMode: Text.Wrap
                        color: Style.themes.fontColor
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

                Rectangle {
                    width: settingStack.standWidth
                    height: warnMoneyText.implicitHeight + 40
                    color: Style.themes.containColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.themes.sideColor
                    border.width: 1
                    Text {
                        x: 20
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: iconFont.name
                        height: warnMoneyText.implicitHeight
                        text: "\uf11a"
                        color: Style.themes.themeColor
                        font.pixelSize: Style.settings.texticon
                    }
                    Text {
                        id: warnMoneyText
                        x: 48
                        y: 20
                        width: parent.width - 108
                        text: "QueMusic Beta（官方版）始终是完全免费且开源的软件，不存在付费，会员，捐献，充值，广告等入口，官方版本不存在Pro，Ultra，高级版等版本，如果你发现软件或软件内有需要付费的内容，请立即与开发者联系。"
                        wrapMode: Text.Wrap
                        color: Style.themes.fontColor
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

                QHead { text: "软件信息" }

                Rectangle {
                    width: settingStack.standWidth
                    height: description.implicitHeight + 48
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Text {
                        id: description
                        anchors.centerIn: parent
                        width: settingStack.standWidth - 48
                        text: "QueMusic是一个基于Qt QML开发的全能音乐播放器，旨在让听歌变得更简单，让每个操作变得简单，QueMusic拥有行业领先的性能，在Qt RHI * QML * C++强大组合下，性能卓越，UI美观丝滑，基于C++的在线音源使其拥有强大的稳定在线体验，QueMusic让听歌变得更简单。"
                        wrapMode: Text.Wrap
                        color: Style.themes.textColor
                        font.pixelSize: 13
                    }
                }

                QHead { text: "主要开发者" }

                Grid {
                    spacing: 24
                    rows: 2
                    width: settingStack.standWidth
                    height: 80
                    AccountCard {
                        source: "qrc:/QueMusic/resources/app/icons/bronekox.jpg"
                        title: "BroNekoX Studio"
                        text: "本项目的主要开发负责人"
                        openUrl: "https://github.com/bronekox"
                    }
                }

                QHead { text: "技术践与版本信息" }

                Rectangle {
                    width: settingStack.standWidth
                    height: 288
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                                text: window.version + " (" + window.versionCode + ")"
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
                                text: "Qt-Community-" + qtRuntimeVersion
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "渲染与主体架构"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "QML Engine/Qt RHI"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "编译架构"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "LLVM-MinGW17 / Clang"
                                color: Style.themes.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "编程语言"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "C++，QML，JS，Sql"
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

                QHead { text: "使用第三方项目与库" }

                Grid {
                    spacing: 24
                    columns: 2
                    rows: 4
                    width: settingStack.standWidth
                    AccountCard {
                        source: "qrc:/QueMusic/resources/app/icons/qwk.png"
                        title: "QWindowKit"
                        text: "实现全平台完美的无边框窗口"
                        openUrl: "https://github.com/stdware/qwindowkit"
                    }
                    AccountCard {
                        source: "qrc:/QueMusic/resources/app/icons/qticon.png"
                        title: "Qt Community"
                        text: "强大的开源软件包框架"
                        openUrl: "https://github.com/qt"
                    }
                    AccountCard {
                        source: "qrc:/QueMusic/resources/app/icons/amll.svg"
                        title: "AMLL Core"
                        text: "使用了AMLL的背景效果部分来实现炫酷的歌词界面背景，使用AGPL-3.0授权"
                        openUrl: "https://github.com/amll-dev/applemusic-like-lyrics"
                    }
                    AccountCard {
                        source: ""
                        title: "Poppins ，Feather"
                        text: "Font库，提供字体与图标的库"
                    }
                    AccountCard {
                        source: ""
                        title: "QCloudMusicApi"
                        text: "网易云音乐第三方API服务框架"
                        openUrl: "https://github.com/s12mmm3/QCloudMusicApi"
                    }
                }

                QHead { text: "联系开发者" }

                Rectangle {
                    width: settingStack.standWidth
                    height: 150
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                                text: "695207057"
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
                        text: "SourceCode"
                        iconCharacter: "\uf0dd"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/bronekox/quemusic");
                        }
                    }

                    QButton {
                        height: 40
                        radius: 20
                        text: "QueMusic网站(未推出)"
                        iconCharacter: "\uf0d7"
                        onClicked: {
                            Qt.openUrlExternally("example.com");
                        }
                    }

                    QButton {
                        height: 40
                        radius: 20
                        text: "Bug反馈"
                        iconCharacter: "\uf06e"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/bronekox/quemusic/issues");
                        }
                    }

                    QButton {
                        height: 40
                        radius: 20
                        text: "开源许可"
                        iconCharacter: "\uf10a"
                        onClicked: {
                            textWatch.active = true;
                        }
                    }
                }
            }
        }

        // Debug
        QScrollView {
            id: debugset
            width: settingStack.width
            height: settingStack.height - 60

            visible: false

            contentChildren: Column {
                id: deBug
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.themes.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "DeBug"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                Text { text: "本页设置仅供调试，可能会出现崩溃甚至软件失效，如要恢复请到软件配置目录删除"; color: Style.themes.fontColor; font.pixelSize: Style.settings.textmain }

                QHead { text: "渲染" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                                onToggled: {
                                    Options.settings.noWindowKit = !Options.settings.noWindowKit;
                                    mainMessage.openSimpleDialog("提示", "重启本应用以生效更改.", null);
                                }
                            }
                        }

                        SettingItemCard {
                            label: "渲染引擎(重启生效)"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Options.settings.gpuRenderMode
                                model: ["系统偏好","OpenGL","Vulkan","Software"]
                                onTransformed: (choiced) => {
                                    Options.settings.gpuRenderMode = choiced;
                                    mainMessage.openSimpleDialog("提示", "重启本应用以生效更改.", null);
                                }
                            }
                        }

                        SettingItemCard {
                            label: "不使用Vsync而使用Timer来驱动界面"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.timerAnimator
                                onToggled: {
                                    Options.settings.timerAnimator = !Options.settings.timerAnimator;
                                    mainMessage.openSimpleDialog("提示", "重启本应用以完全生效更改.", null);
                                }
                            }
                        }

                        SettingItemCard {
                            label: "QML动画引擎"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                text: switchTrue ? "VsyncGui" : "QETimer"
                                switchTrue: Options.settings.qmlAnimator
                                onToggled: {
                                    Options.settings.qmlAnimator = !Options.settings.qmlAnimator;
                                    mainMessage.openSimpleDialog("提示", "重启本应用以完全生效更改.", null);
                                }
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "调试" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
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
                            label: "调试模式(x)"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.debug
                                onToggled: Options.settings.debug = !Options.settings.debug
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "日志" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "启用日志"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: logManager.enabled
                                onToggled: logManager.enabled = !logManager.enabled
                            }
                        }

                        SettingItemCard {
                            label: "记录等级"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: logManager.minimumLevel
                                model: ["调试","信息","警告","错误","致命"]
                                onTransformed: (choiced) => { logManager.minimumLevel = choiced }
                            }
                        }

                        SettingItemCard {
                            label: "打开日志目录"
                            controlItem: QButton {
                                anchors.fill: parent
                                text: "打开"
                                shadowEnabled: false
                                buttonColor: "transparent"
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                onClicked: logManager.openLogFolder()
                            }
                            bottomLine: false
                        }
                    }
                }

                Rectangle {
                    width: settingStack.standWidth
                    height: 320
                    color: Style.themes.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8
                        Text {
                            width: parent.width
                            text: "实时日志预览（完整内容见日志文件）"
                            color: Style.themes.textColor
                            font.pixelSize: Style.settings.text
                        }
                        Flickable {
                            id: logPreviewFlick
                            width: parent.width
                            height: parent.height - 30
                            clip: true
                            contentWidth: width
                            contentHeight: logPreviewText.height
                            onContentHeightChanged: contentY = Math.max(0, contentHeight - height)
                            Text {
                                id: logPreviewText
                                width: parent.width
                                text: logManager.logPreview
                                color: Style.themes.textColor
                                font.pixelSize: Style.settings.text
                                wrapMode: Text.Wrap
                            }
                        }
                    }
                }
            }
        }
    }

    Item {
        id: updater

        // 从文件或宏定义中获取的本地版本号
        property int localVersion: window.versionCode

        // 远程 version.txt 的 URL
        property string remoteVersionUrl: "https://raw.githubusercontent.com/BroNekoX/QueMusic/main/version.txt"
        property int newVersion: window.versionCode

        function checkForUpdate() {
            console.log("正在检查更新...");
            mainWarn.tiped("正在检查更新", 0);

            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState === XMLHttpRequest.DONE) {
                    if (xhr.status === 200) {
                        var remoteVersion = parseInt(xhr.responseText.trim());
                        console.log("远程版本号:", remoteVersion);

                        if (remoteVersion > localVersion) {
                            console.log("发现新版本!");
                            newVersion = remoteVersion;
                            // 显示更新提示对话框
                            updateDialog.open();
                        } else {
                            console.log("当前已是最新版本");
                            // 可选：显示“已是最新”的提示
                            mainWarn.tiped("当前已是最新版本", 1);
                        }
                    } else {
                        console.error("检查更新失败，HTTP状态码:", xhr.status);
                        mainWarn.tiped("检查更新失败，请稍后重试", 2);
                    }
                }
            }
            xhr.open("GET", remoteVersionUrl)
            xhr.send();
        }

        // 更新提示对话框
        QAlertDialog {
            id: updateDialog
            title: "有新版本！"
            message: "有新版本：(v" + updater.newVersion + ")可供下载，是否前往下载？"
            isInput: false
            blurSource: settingsView
            //standardButtons: Dialog.Ok | Dialog.Cancel

            onConfirm: {
                Qt.openUrlExternally("https://github.com/BroNekoX/QueMusic/releases");
            }
        }
    }
}