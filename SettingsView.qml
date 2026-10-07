// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtCore
import QueMusic 1.0

Item {
    id: settingsView

    // 宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id
    readonly property AudioEngine player: Playback.player

    // 账号登录面板展开状态
    property bool neteaseShowLogin: false
    property bool kugouShowLogin: false
    property bool neteaseAltVisible: false
    property string neteasePhone: ""
    property string neteaseCaptcha: ""
    readonly property bool neteasePhoneReady: neteasePhone !== "" && neteaseCaptcha !== ""
    property string neteaseLoginStatus: "等待登录…"
    property string kugouLoginStatus: "等待登录…"

    Image {
        id: settingsBackGround
        z: 0
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        source: backGround.source
        visible: Style.settings.backmode === 2 || Style.settings.backmode === 3
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
    }

    // 登录成功自动收起面板
    Connections {
        target: AccountManager
        function onNeteaseLoginChanged(): void {
            if (AccountManager.neteaseLoggedIn) {
                settingsView.neteaseShowLogin = false;
                neteaseQrDialog.close();
            }
        }
        function onKugouLoginChanged(): void {
            if (AccountManager.kugouLoggedIn) {
                settingsView.kugouShowLogin = false;
                kugouQrDialog.close();
            }
        }
    }

    // 网易云扫码登录弹窗（QOptionDialog + QRCodeView）
    QOptionDialog {
        id: neteaseQrDialog
        title: "网易云音乐 - 扫码登录"
        // 手机号和验证码都填好后，左下的「取消」变成「确认登录」：
        // 手机号输入区在可滚动区域里，这排按钮才是始终看得见的确认入口
        cancelText: settingsView.neteasePhoneReady ? "确认登录" : "取消"
        cancelIcon: settingsView.neteasePhoneReady ? "\uf0e7" : "\uf10f"
        cancelCloses: !settingsView.neteasePhoneReady
        confirmText: "关闭"
        blurSource: settingsView
        onCancel: {
            if (settingsView.neteasePhoneReady) {
                // 登录中/失败都不关弹窗，好让提示留在眼前（成功后由 neteaseLoginChanged 收起）
                AccountManager.loginNeteaseWithCellphone(settingsView.neteasePhone,
                                                         settingsView.neteaseCaptcha);
                return;
            }
            AccountManager.cancelNeteaseQrLogin();
            settingsView.neteaseShowLogin = false;
        }
        onConfirm: { AccountManager.cancelNeteaseQrLogin(); settingsView.neteaseShowLogin = false; }
        onClosed: { AccountManager.cancelNeteaseQrLogin(); settingsView.neteaseShowLogin = false; }

        options: Column {
            width: parent.width
            spacing: 14
            Rectangle {
                width: 232
                height: 232
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 12
                color: Style.darkis ? Style.secondaryColor : "#ffffff"
                border.color: Style.secondaryColor
                border.width: 1
                QRCodeView {
                    id: neteaseQrCode
                    width: 212
                    height: 212
                    anchors.centerIn: parent
                    qrText: AccountManager.neteaseQrText
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: AccountManager.neteaseQrMessage
                color: Style.fontColor
                font.pixelSize: Style.settings.textmain
                elide: Text.ElideRight
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }
            // 账号安全风险拦截：网易云给了验证页链接就让用户直接打开处理
            QButton {
                visible: AccountManager.neteaseVerifyUrl !== ""
                anchors.horizontalCenter: parent.horizontalCenter
                text: "去完成安全验证"
                width: 150
                height: 34
                radius: 17
                shadowEnabled: false
                buttonColor: Style.themeColor
                textColor: Style.primaryColor
                onClicked: AccountManager.openNeteaseVerifyUrl()
            }
            QButton {
                visible: AccountManager.neteaseQrState === 3 || AccountManager.neteaseQrState === 4
                anchors.horizontalCenter: parent.horizontalCenter
                text: "重新获取二维码"
                width: 150
                height: 34
                radius: 17
                shadowEnabled: false
                buttonColor: Style.themeColor
                textColor: Style.primaryColor
                onClicked: AccountManager.startNeteaseQrLogin()
            }

            // ---- 备用登录方式（扫码提示"环境异常"时用）----
            Text {
                width: parent.width
                text: "手机号登录被网易云风控拦截时，可改用：粘贴浏览器里的 Cookie（最稳）"
                color: Style.fontColor
                opacity: 0.7
                font.pixelSize: Style.settings.textmain - 3
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            QButton {
                anchors.horizontalCenter: parent.horizontalCenter
                text: settingsView.neteaseAltVisible ? "收起其他登录方式" : "其他登录方式（手机号 / 粘贴 Cookie）"
                width: 250
                height: 34
                radius: 17
                shadowEnabled: false
                buttonColor: Style.secondaryColor
                textColor: Style.fontColor
                onClicked: settingsView.neteaseAltVisible = !settingsView.neteaseAltVisible
            }

            // 手机号 + 验证码登录
            Column {
                width: parent.width
                spacing: 10
                visible: settingsView.neteaseAltVisible

                // 上报输入内容：弹窗外层看不到本组件里的 id，靠这两个绑定驱动左下按钮的形态
                Binding {
                    target: settingsView
                    property: "neteasePhone"
                    value: neteasePhoneInput.text.trim()
                }
                Binding {
                    target: settingsView
                    property: "neteaseCaptcha"
                    value: neteaseCaptchaInput.text.trim()
                }

                TextField {
                    id: neteasePhoneInput
                    width: parent.width
                    height: 36
                    leftPadding: 12
                    placeholderText: "手机号"
                    color: Style.textColor
                    font.pixelSize: Style.settings.textmain
                    verticalAlignment: Text.AlignVCenter
                    background: Rectangle {
                        radius: 8
                        color: Style.primaryColor
                        border.color: Style.secondaryColor
                        border.width: 1
                    }
                }

                Row {
                    width: parent.width
                    spacing: 10

                    TextField {
                        id: neteaseCaptchaInput
                        width: parent.width - 130
                        height: 36
                        leftPadding: 12
                        placeholderText: "短信验证码"
                        color: Style.textColor
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        background: Rectangle {
                            radius: 8
                            color: Style.primaryColor
                            border.color: Style.secondaryColor
                            border.width: 1
                        }
                    }

                    QButton {
                        width: 120
                        height: 36
                        radius: Style.settings.labelRadius
                        shadowEnabled: false
                        text: "发送验证码"
                        buttonColor: Style.secondaryColor
                        textColor: Style.fontColor
                        onClicked: AccountManager.sendNeteaseCaptcha(neteasePhoneInput.text)
                    }
                }

                QButton {
                    width: 150
                    height: 36
                    radius: Style.settings.labelRadius
                    shadowEnabled: false
                    text: "手机号登录"
                    buttonColor: Style.themeColor
                    textColor: Style.primaryColor
                    onClicked: AccountManager.loginNeteaseWithCellphone(neteasePhoneInput.text,
                                                                      neteaseCaptchaInput.text)
                }

                // 发送验证码/登录的进度与失败原因：弹窗上方那条提示滚上去就看不见了，这里就近再显示一份
                Text {
                    width: parent.width
                    text: AccountManager.neteaseQrMessage
                    color: Style.fontColor
                    opacity: 0.7
                    font.pixelSize: Style.settings.textmain - 3
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }

                QButton {
                    visible: AccountManager.neteaseVerifyUrl !== ""
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "去完成安全验证"
                    width: 150
                    height: 36
                    radius: Style.settings.labelRadius
                    shadowEnabled: false
                    buttonColor: Style.themeColor
                    textColor: Style.primaryColor
                    onClicked: AccountManager.openNeteaseVerifyUrl()
                }
            }

            // 粘贴 Cookie 登录
            Column {
                width: parent.width
                spacing: 10
                visible: settingsView.neteaseAltVisible

                TextField {
                    id: neteaseCookieInput
                    width: parent.width
                    height: 36
                    leftPadding: 12
                    placeholderText: "粘贴含 MUSIC_U 的 Cookie"
                    color: Style.textColor
                    font.pixelSize: Style.settings.textmain
                    verticalAlignment: Text.AlignVCenter
                    background: Rectangle {
                        radius: 8
                        color: Style.primaryColor
                        border.color: Style.secondaryColor
                        border.width: 1
                    }
                }

                QButton {
                    width: 150
                    height: 36
                    radius: Style.settings.labelRadius
                    shadowEnabled: false
                    text: "用 Cookie 登录"
                    buttonColor: Style.themeColor
                    textColor: Style.primaryColor
                    onClicked: AccountManager.loginNeteaseWithCookie(neteaseCookieInput.text)
                }
            }
        }
    }

    // 酷狗扫码登录弹窗（QOptionDialog + QRCodeView）
    QOptionDialog {
        id: kugouQrDialog
        title: "酷狗音乐 - 扫码登录"
        cancelText: "取消"
        confirmText: "关闭"
        blurSource: settingsView
        onCancel: { AccountManager.cancelKugouQrLogin(); settingsView.kugouShowLogin = false; }
        onConfirm: { AccountManager.cancelKugouQrLogin(); settingsView.kugouShowLogin = false; }
        onClosed: { AccountManager.cancelKugouQrLogin(); settingsView.kugouShowLogin = false; }

        options: Column {
            width: parent.width
            spacing: 14
            Rectangle {
                width: 232
                height: 232
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 12
                color: Style.darkis ? Style.secondaryColor : "#ffffff"
                border.color: Style.secondaryColor
                border.width: 1
                QRCodeView {
                    id: kugouQrCode
                    width: 212
                    height: 212
                    anchors.centerIn: parent
                    qrText: AccountManager.kugouQrText
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: AccountManager.kugouQrMessage
                color: Style.fontColor
                font.pixelSize: Style.settings.textmain
                elide: Text.ElideRight
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
            }
            QButton {
                visible: AccountManager.kugouQrState === 3 || AccountManager.kugouQrState === 4
                anchors.horizontalCenter: parent.horizontalCenter
                text: "重新获取二维码"
                width: 150
                height: 34
                radius: 17
                shadowEnabled: false
                buttonColor: Style.themeColor
                textColor: Style.primaryColor
                onClicked: AccountManager.startKugouQrLogin()
            }
        }
    }

    FolderDialog {
        id: downloadFolderDialog
        title: "选择默认下载目录"
        currentFolder: Options.settings.downloadFolder || StandardPaths.writableLocation(StandardPaths.MusicLocation)
        onAccepted: {
            let p = downloadFolderDialog.selectedFolder.toString();
            if (p.indexOf("file:///") === 0)
                p = p.substring(8);
            Options.settings.downloadFolder = p;
            Options.warn.tiped("已设置默认下载目录", 1);
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
        property color choiceColor: Style.hoverColor
        property color choiceTextColor: Style.fontColor
        color: sidebar.color
        Connections {
            target: Style
            function onChangeTheme(): void {
                if(Style.settings.sidebarStyle === 0) {
                    leftSidebarSettings.choiceColor = Style.hoverColor;
                    leftSidebarSettings.choiceTextColor = Style.fontColor;
                    choicebar1.x = 18;
                    choicebar1.radius = 2;
                } else if(Style.settings.sidebarStyle === 1) {
                    leftSidebarSettings.choiceColor = Style.themeColor;
                    leftSidebarSettings.choiceTextColor = Style.primaryColor;
                    choicebar1.x = 0;
                    choicebar1.radius = 0;
                }
            }
        }
        Component.onCompleted: {
            index1ed(0);
            if(Style.settings.sidebarStyle === 0) {
                leftSidebarSettings.choiceColor = Style.hoverColor;
                leftSidebarSettings.choiceTextColor = Style.fontColor;
                choicebar1.x = 18;
                choicebar1.radius = 2;
            } else if(Style.settings.sidebarStyle === 1) {
                leftSidebarSettings.choiceColor = Style.themeColor;
                leftSidebarSettings.choiceTextColor = Style.primaryColor;
                choicebar1.x = 0;
                choicebar1.radius = 0;
            }
        }

        function index1ed(choice: int): void {
            if(window.isMacOS) {
                choicebar1.willBarY = 44 * choice + 100
            } else {
                choicebar1.willBarY = 44 * choice + 80
            }
            if(choice > choicebar1.indexOld) {
                downBarS.stop();
                upBarS.stop();
                downBarS.running = true;
            } else if(choice < choicebar1.indexOld) {
                upBarS.stop();
                downBarS.stop();
                upBarS.running = true;
            }
            choicebar1.indexOld = choice;
            if(choice !== settingStack.setPageIndex) {
                setPageAnine.stop();
                setPageAnimeo.target = settingStack.setPages[choice];
                setPageAnimey.target = settingStack.setPages[choice];
                setPageAnine.start();
                settingStack.setPages[settingStack.setPageIndex].visible = false;
                settingStack.setPages[choice].visible = true;
                settingStack.setPageIndex = choice;
            }
        }

        Rectangle {
            id: choicebar1
            x: 18
            width: 4
            height: barBottom - y
            y: window.isMacOS ? 100 : 80
            topRightRadius: 2
            bottomRightRadius: 2
            radius: 2
            color: Style.themeColor
            opacity: 1
            property int barBottom: window.isMacOS ? 122 : 102
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
        }

        Item {
            x: 16;y: window.isMacOS ? 42 : 12
            height: 36
            width: 180
            QWKButton {
                id: returnButton
                width: 36
                height: 36
                source: Style.darkis ? "qrc:/QueMusic/resources/window-bar/returnd.svg" : "qrc:/QueMusic/resources/window-bar/return.svg"
                background: Rectangle {
                    color: returnButton.hovered ? Style.hoverColor : "transparent"
                    radius: Style.settings.noControlRadius ? Style.settings.labelRadius : 8
                    Behavior on color { ColorAnimation { duration: 50 } }
                }
                onClicked: settingOutAnime.running = true;
                Component.onCompleted: Options.agent.setHitTestVisible(returnButton, true);
            }

            Label {
                x: 50
                y: 0
                width: 120
                height: 36
                text: "应用设置"
                font.pixelSize: 14
                font.bold: false
                verticalAlignment: Text.AlignVCenter
                color: Style.fontColor
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }

        ListModel {
            id: navModelSettings
            ListElement { display: "通用"; iconChar: "\uf038" }
            ListElement { display: "界面"; iconChar: "\uf0a7" }
            ListElement { display: "播放器"; iconChar: "\uf010" }
            ListElement { display: "音频"; iconChar: "\uf044" }
            ListElement { display: "快捷键"; iconChar: "\uf0c7" }
            ListElement { display: "插件"; iconChar: "\uf0ff" }
            ListElement { display: "关于"; iconChar: "\uf0b6" }
            ListElement { display: "Debug"; iconChar: "\uf060" }
        }

        Column {
            id: navListViewSettings
            x: 15
            y: window.isMacOS ? 90 : 70
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
                        color: Style.hoverColor
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
                        font.family: IconFont.name
                        font.pixelSize: Style.settings.texticon
                        color: navDelegateSettings.isSelected ? leftSidebarSettings.choiceTextColor : Style.textColor
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
                        color: navDelegateSettings.isSelected ? leftSidebarSettings.choiceTextColor : Style.textColor
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
        width: parent.width - 210
        height: parent.height
        color: mainContent.color
        z: 2
        property color cardColor: Style.settings.backmode === 0 ? Style.primaryColor : Style.primaryBlurColor
        property list<Item> setPages: [
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

            Column {
                id: themeContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX
                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.fontColor
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
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        id: globalThemeCard
                        width: parent.width
                        padding: 0

                        SettingItemCard {
                            label: "全局主题"
                            tip: "设置应用的全局主题"
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
                            tip: "设置应用的全局主要配色"
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
                                            borderColor: Style.textColor
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
                                        buttonColor: Style.secondaryColor
                                        onClicked: {
                                            themeColorChoose.openColor(Style.themeColor);
                                        }
                                    }
                                }
                            }
                            ColorPickerDialog {
                                id: themeColorChoose
                                blurSource: settingsView
                                onAccepted: {
                                    Style.settings.colorList.push(Qt.hsva(selectedColor.hsvHue,0.9,0.8,1.0));
                                    Options.warn.tiped("已添加主题颜色", 1);
                                }
                            }
                        }

                        SettingItemCard {
                            label: "应用背景"
                            tip: "设置在背景显示的样式"
                            isBigItem: true
                            controlItem: QWideDrop {
                                anchors.fill: parent
                                model: ["默认","浅主题色","云母材质","图片","模糊窗口"]
                                choice: Style.settings.backmode
                                onTransformed: (choiced) => {
                                    Style.settings.backmode = choiced
                                    Style.changeUi()
                                    Style.changeTheme()
                                    if(choiced === 4) {
                                        mainMessage.openSimpleDialog("提示", "重启本应用以生效模糊窗口功能.", null);
                                    }
                                }
                            }
                        }

                        SettingItemCard {
                            label: "选择背景图片"
                            tip: "设置应用背景壁纸显示的图片"
                            visible: Style.settings.backmode == 3
                            height: visible ? 64 : 0
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
                                    Options.warn.tiped("已设置背景图片", 1);
                                    Style.changeUi();
                                    Style.changeTheme();
                                }
                            }
                        }
                    }
                }

                QHead { text: "在线平台与账户" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
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
                            color: Style.hoverColor
                            radius: Style.settings.labelRadius
                            Text {
                                x: 12
                                anchors.verticalCenter: parent.verticalCenter
                                width: 30
                                text: "\uf0b6"
                                font.family: IconFont.name
                                color: Style.textColor
                                font.pixelSize: Style.settings.texticon
                            }
                            Text {
                                x: 44
                                width: parent.width - 56
                                anchors.verticalCenter: parent.verticalCenter
                                text: "扫码登录：用你自己的账号在应用内登录（账号即能力），登录后自动读取登录态，仅保存在本机，随时可退出。"
                                color: Style.textColor
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }

                        Grid {
                            x: 16
                            width: parent.width - 32
                            height: 144
                            columns: 3
                            columnSpacing: 14
                            rowSpacing: 14
                            PlatformCard {
                                text: "酷狗音乐"
                                chooseColor: "#4384F5"
                                chooseColor1: Style.darkis ? "#153A57" : "#CDE8FF"
                                width: settingStack.standWidth / 3 - 20
                                height: 128
                                choose: MusicApi.songSource === 0
                                isLogin: AccountManager.kugouLoggedIn
                                header: AccountManager.kugouAvatar
                                name: AccountManager.kugouNickname
                                onClicked: MusicApi.songSource = 0;
                                onLogined: {
                                    if (AccountManager.kugouLoggedIn) {
                                        Options.dialog.openSimpleDialog("警告", "是否退出账号？",
                                            function() {
                                                AccountManager.logoutKugou();
                                            }
                                        )
                                    } else {
                                        settingsView.kugouShowLogin = true;
                                        kugouQrDialog.open();
                                        AccountManager.startKugouQrLogin();
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
                                isLogin: AccountManager.neteaseLoggedIn
                                header: AccountManager.neteaseAvatar
                                name: AccountManager.neteaseNickname
                                onClicked: MusicApi.songSource = 1;
                                onLogined: {
                                    if (AccountManager.neteaseLoggedIn) {
                                        Options.dialog.openSimpleDialog("警告", "是否退出账号？",
                                            function() {
                                                AccountManager.logoutNetease();
                                            }
                                        )
                                    } else {
                                        settingsView.neteaseShowLogin = true;
                                        neteaseQrDialog.open();
                                        AccountManager.startNeteaseQrLogin();
                                    }
                                }
                            }
                            PlatformCard {
                                text: "哔哩哔哩"
                                chooseColor: "#FB7299"
                                chooseColor1: Style.darkis ? "#5C2338" : "#FFD9E6"
                                width: settingStack.standWidth / 3 - 20
                                height: 128
                                choose: MusicApi.songSource === 2
                                showLogin: false
                                idleText: "无需登录"
                                onClicked: MusicApi.songSource = 2;
                            }
                        }
                    }
                }

                QHead { text: "应用程序" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "全局音量"
                            tip: "设置本应用音乐播放单的音量"
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
                            label: "下载目录"
                            tip: "设置音乐文件默认下载的目录"
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
                            label: "最小化托盘"
                            tip: "开启后点击关闭按钮将隐藏窗口（任务栏不再显示），可从托盘图标恢复或退出"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.closeToManage
                                onToggled: Options.settings.closeToManage = !Options.settings.closeToManage
                            }
                        }

                        SettingItemCard {
                            label: "记住窗口"
                            tip: "下次启动将恢复原来的窗口位置和大小"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.rememberWindow
                                onToggled: Options.settings.rememberWindow = !Options.settings.rememberWindow
                            }
                        }

                        SettingItemCard {
                            label: "自动检查更新"
                            tip: "在每次启动应用程序时将检查是否有新版本"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.autoUpdate
                                onToggled: Options.settings.autoUpdate = !Options.settings.autoUpdate
                            }
                        }

                        SettingItemCard {
                            label: "默认缓存位置"
                            tip: "设置封面等图片缓存文件的存放目录"
                            controlItem: QButton {
                                anchors.fill: parent
                                shadowEnabled: false
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                text: Options.settings.cacheUrl ? "自定义目录" : "系统默认"
                                fontSize: Style.settings.text
                                onClicked: cacheFolderDialog.open()
                            }
                            FolderDialog {
                                id: cacheFolderDialog
                                title: "选择缓存目录"
                                onAccepted: {
                                    let p = cacheFolderDialog.selectedFolder.toString();
                                    if (p.indexOf("file:///") === 0)
                                        p = p.substring(8);
                                    Options.settings.cacheUrl = p;
                                    coverHelper.setCacheDir(p);
                                    Options.warn.tiped("已设置封面缓存目录", 1);
                                }
                            }
                        }

                        SettingItemCard {
                            label: "自动缓存图片"
                            tip: "加载的图片将自动缓存到内存中，可以提高性能，但会占用更多内存"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.picCache
                                onToggled: Options.settings.picCache = !Options.settings.picCache
                            }
                        }

                        SettingItemCard {
                            label: "清除图片缓存"
                            tip: "清除缓存到磁盘的图片文件"
                            controlItem: QButton {
                                anchors.fill: parent
                                shadowEnabled: false
                                buttonColor: "transparent"
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                text: "清理"
                                onClicked: {
                                    coverHelper.clearCache();
                                    // 清缓存时 DB 引用已作废，重载列表以丢弃界面上的死链路径
                                    Songs.reload();
                                    Style.warned("已清除图片缓存", 1);
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

            Column {
                id: uiContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "界面"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }
                QHead { text: "界面样式" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "左栏沉浸"
                            tip: "左侧的页面切换栏将使用与背景一致的色彩，提高观感"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.sidebarColor
                                onToggled: Style.settings.sidebarColor = !Style.settings.sidebarColor
                            }
                        }

                        SettingItemCard {
                            label: "左栏样式"
                            tip: "左侧的页面切换栏样式"
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
                            label: "不透明底栏"
                            tip: "播放器控制底栏将不再透明"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.noOpacityControl
                                onToggled: Style.settings.noOpacityControl = !Style.settings.noOpacityControl
                            }
                        }

                        SettingItemCard {
                            label: "卡片圆角大小"
                            tip: "设置全局大型卡片的圆角大小"
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
                            label: "控件圆角大小"
                            tip: "设置全局控件的圆角大小"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 0
                                to: 24
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
                            label: "不使用控件大圆角"
                            tip: "(废弃)将部分完全圆角的控件取消完全圆角"
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
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "高级材质"
                            tip: "使用更高质量的背景模糊提升观感"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.highQualityBlur
                                onToggled: Style.settings.highQualityBlur = !Style.settings.highQualityBlur
                            }
                        }

                        SettingItemCard {
                            label: "卡片阴影模糊度"
                            tip: "设置全局卡片的阴影模糊程度"
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
                            label: "卡片背景模糊度"
                            tip: "设置全局卡片的背景模糊程度"
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
                            label: "子页面切入切出动画"
                            tip: "在打开或关闭子页面所使用的动画效果"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Style.settings.animeType
                                model: ["放大缩小","左右渐入"]
                                onTransformed: (choiced) => {
                                    Style.settings.animeType = choiced;
                                }
                            }
                        }

                        SettingItemCard {
                            label: "高级动画效果"
                            tip: "启动更精细的动画效果"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.premiumAnime
                                onToggled: Style.settings.premiumAnime = !Style.settings.premiumAnime
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "页面布局" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "首页默认布局"
                            tip: "设置首页面的卡片布局"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Style.settings.homeLayout
                                model: ["默认","竖向","混合"]
                                onTransformed: (choiced) => Style.settings.homeLayout = choiced
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "播放器歌词模式" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: settingStack.standWidth
                        padding: 0
                        Component.onCompleted: {
                            parent.height = height
                        }
                        SettingItemCard {
                            label: "标准歌词大小"
                            tip: "设置歌词显示字体大小(会跟随窗口大小改变)"
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
                            tip: "设置歌词显示的字重"
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
                            tip: "设置歌词显示的字体"
                            controlItem: QButton {
                                anchors.fill: parent
                                shadowEnabled: false
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                text: Style.settings.fontFamily ? Style.settings.fontFamily : "系统默认"
                                fontSize: Style.settings.text
                                onClicked: lyricFontDialog.openFamily(Style.settings.fontFamily)
                            }
                            FontFamilyDialog {
                                id: lyricFontDialog
                                blurSource: settingsView
                                currentFamily: Style.settings.fontFamily
                                onAccepted: Style.settings.fontFamily = lyricFontDialog.selectedFamily
                            }
                        }

                        SettingItemCard {
                            label: "背景着色器样式"
                            tip: "设置沉浸播放页的流体背景算法，静态渐变最省性能"
                            isBigItem: true
                            controlItem: QWideDrop {
                                anchors.fill: parent
                                model: ["Fluid", "Classic", "静态渐变"]
                                choice: Style.settings.flowStyle
                                onTransformed: (choiced) => {
                                    Style.settings.flowStyle = choiced
                                }
                            }
                        }

                        SettingItemCard {
                            label: "显示音波效果"
                            tip: "在沉浸播放页显示实时音波频谱"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.waveDisplay
                                onToggled: Style.settings.waveDisplay = !Style.settings.waveDisplay
                            }
                        }

                        SettingItemCard {
                            label: "歌词渐进模糊"
                            tip: "让非当前行歌词渐进模糊，突出正在播放的一句"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.maskBlur
                                onToggled: Style.settings.maskBlur = !Style.settings.maskBlur
                            }
                        }

                        SettingItemCard {
                            label: "高级逐行弹簧动画"
                            tip: "歌词逐行切换时使用更细腻的弹簧动画"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Style.settings.premiumLyricAnime
                                onToggled: Style.settings.premiumLyricAnime = !Style.settings.premiumLyricAnime
                            }
                        }

                        SettingItemCard {
                            label: "自动进入沉浸模式"
                            tip: "进入沉浸播放页后自动隐藏界面控件"
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
            }
        }

        // 播放器设置
        QScrollView {
            id: toolset
            width: settingStack.width
            height: settingStack.height - 60

            visible: false

            Column {
                id: funcContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "播放器"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                QHead { text: "在线功能" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "默认音质"
                            tip: "设置在线音乐默认播放的音质等级"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Options.settings.soundQuality
                                model: ["标准-128k","高清-320k","无损-500+k"]
                                onTransformed: (choiced) => {
                                    Options.settings.soundQuality = choiced
                                }
                            }
                        }

                        SettingItemCard {
                            label: "默认音乐源"
                            tip: "设置在线音乐默认使用的音源平台"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: Options.settings.mainMusicSource
                                model: ["酷狗音乐","网易云音乐","哔哩哔哩","QQ音乐"]
                                onTransformed: (choiced) => {
                                    Options.settings.mainMusicSource = choiced
                                }
                            }
                        }

                        SettingItemCard {
                            label: "缓存大小/MB"
                            tip: "设置图片缓存的磁盘占用上限，超出后自动清理"
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
                                    coverHelper.pruneCache(value)
                                }
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "通用" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "使用默认输出设备"
                            tip: "跟随系统当前默认的音频输出设备播放"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.useDefaultDevice
                                onToggled: Options.settings.useDefaultDevice = !Options.settings.useDefaultDevice
                            }
                        }

                        SettingItemCard {
                            label: "音频输出设备"
                            tip: "指定播放时使用的音频输出设备"
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
                            label: "自动播放"
                            tip: "启动应用后自动开始播放"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.autoPlay
                                onToggled: Options.settings.autoPlay = !Options.settings.autoPlay
                            }
                        }

                        SettingItemCard {
                            label: "音量步长"
                            tip: "每次滚轮或快捷键调节音量的幅度"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 1
                                to: 20
                                stepSize: 1
                                leftText: true
                                valueText: value.toString() + " %"
                                value: Options.settings.volumeStep
                                onMoved: Options.settings.volumeStep = value
                            }
                            bottomLine: false
                        }
                    }
                }

                QHead { text: "播放控制" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "精确跳转步长"
                            tip: "快捷键快进/快退一次跳转的秒数"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: [3,5,10,15,30].indexOf(Options.settings.seekStep)
                                model: ["3 秒","5 秒","10 秒","15 秒","30 秒"]
                                onTransformed: (choiced) => Options.settings.seekStep = [3,5,10,15,30][choiced]
                            }
                        }

                        SettingItemCard {
                            label: "播放淡入淡出"
                            tip: "切歌或暂停时对音量淡入淡出，避免爆音"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.fadeEnabled
                                onToggled: Options.settings.fadeEnabled = !Options.settings.fadeEnabled
                            }
                        }

                        SettingItemCard {
                            label: "淡变时长"
                            tip: "淡入淡出动画持续的毫秒数"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 0
                                to: 500
                                stepSize: 50
                                leftText: true
                                valueText: value.toString() + " ms"
                                value: Options.settings.fadeMs
                                onMoved: Options.settings.fadeMs = value
                            }
                        }

                        SettingItemCard {
                            label: "睡眠定时默认"
                            tip: "睡眠定时的默认倒计时分钟数"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 1
                                to: 120
                                stepSize: 1
                                leftText: true
                                valueText: value.toString() + " 分"
                                value: Options.settings.sleepMinutes
                                onMoved: Options.settings.sleepMinutes = value
                            }
                        }

                        SettingItemCard {
                            label: "随机避免最近"
                            tip: "随机播放时避免重复最近播放的歌曲数量"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 0
                                to: 5
                                stepSize: 1
                                leftText: true
                                valueText: value.toString() + " 首"
                                value: Options.settings.shuffleAvoid
                                onMoved: Options.settings.shuffleAvoid = value
                            }
                        }

                        SettingItemCard {
                            label: "恢复上次播放列表"
                            tip: "下次启动时恢复退出前的播放列表"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.autoRestoreQueue
                                onToggled: Options.settings.autoRestoreQueue = !Options.settings.autoRestoreQueue
                            }
                        }

                        SettingItemCard {
                            label: "播放历史上限"
                            tip: "最多保留的播放历史条数"
                            controlItem: QSlider {
                                anchors.fill: parent
                                from: 20
                                to: 500
                                stepSize: 20
                                leftText: true
                                valueText: value.toString() + " 条"
                                value: Options.settings.historyLimit
                                onMoved: Options.settings.historyLimit = value
                            }
                        }

                        SettingItemCard {
                            label: "清空播放历史"
                            tip: "清空全部播放历史记录"
                            controlItem: QButton {
                                anchors.fill: parent
                                anchors.verticalCenter: parent.verticalCenter
                                shadowEnabled: false
                                fontSize: Style.settings.textTip
                                text: "清空"
                                buttonColor: "#fa4642"
                                textColor: "#ffffff"
                                onClicked: Options.dialog.openSimpleDialog("清空", "将清空全部播放历史，是否继续？",
                                    function() {
                                        Playback.clearHistory()
                                        Options.warn.tiped("已清空播放历史", 1)
                                    })
                            }
                            bottomLine: false
                        }
                    }
                }
            }
        }

        // 音频设置
        QScrollView {
            id: playerset
            width: settingStack.width
            height: settingStack.height - 60

            visible: false

            Column {
                id: playContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "音频"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                EqualizerPanel {
                    width: settingStack.standWidth
                    engine: player
                    cardColor: settingStack.cardColor
                }
            }
        }

        // 快捷键设置
        QScrollView {
            id: shortcutset
            width: settingStack.width
            height: settingStack.height - 60
            visible: false

            // 录制状态
            property string recordingAction: ""
            property bool isRecording: false
            property bool oldShortCutState: false

            // 根据 action 名拼出 Options.settings 里的"该功能是否全局生效"属性名
            function globalPropertyName(actionName: string): string {
                return "globalShortcut" + actionName.charAt(0).toUpperCase() + actionName.slice(1)
            }

            // 按键转字符串（辅助函数）
            function keyToString(key: int): string {
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

            function keyEventToSequence(event: var): string {
                const modifiers = []
                if (event.modifiers & Qt.ControlModifier) modifiers.push("Ctrl")
                if (event.modifiers & Qt.AltModifier) modifiers.push("Alt")
                if (event.modifiers & Qt.ShiftModifier) modifiers.push("Shift")
                const key = event.key
                // 忽略单独的修饰键
                if (key === Qt.Key_Control || key === Qt.Key_Alt || key === Qt.Key_Shift || key === Qt.Key_Meta)
                    return ""
                const keyName = keyToString(key)
                if (!keyName) return ""
                let seq = modifiers.join("+")
                if (seq && keyName) seq += "+"
                seq += keyName
                return seq
            }

            // 开始录制
            function startRecording(action: string): void {
                if (isRecording) return
                recordingAction = action
                isRecording = true
                oldShortCutState = Options.settings.openShortCut
                Options.settings.openShortCut = false   // 关闭总开关（兼容旧逻辑）
                Options.recordingShortCut = true   // 录制期间屏蔽所有全局快捷键
                keyCapture.forceActiveFocus()
                keyCapture.focus = true;
                Options.warn.tiped("按下新的快捷键... (按 Esc 取消)", 0)
            }

            // 停止录制（完成或取消）
            function stopRecording(success: bool, sequence: string): void {
                isRecording = false
                Options.settings.openShortCut = oldShortCutState
                Options.recordingShortCut = false
                if (success && sequence) {
                    Options.shortCuts[recordingAction] = sequence
                    Options.warn.tiped("已设置快捷键: " + sequence, 1)
                } else {
                    Options.warn.tiped("已取消录制", 1)
                }
                recordingAction = ""
                keyCapture.focus = false
            }

            // 动作定义（名称、显示描述、默认键位）
            ListModel {
                id: actionDefs
                ListElement { name: "play"; desc: "播放/暂停"; defau: "Space" }
                ListElement { name: "back"; desc: "上一首"; defau: "Left" }
                ListElement { name: "forward"; desc: "下一首"; defau: "Right" }
                ListElement { name: "playList"; desc: "打开/关闭播放列表"; defau: "Alt" }
                ListElement { name: "musicControl"; desc: "音乐控制面板"; defau: "Up" }
                ListElement { name: "volumeUp"; desc: "音量增大"; defau: "Ctrl+Up" }
                ListElement { name: "volumeDown"; desc: "音量减小"; defau: "Ctrl+Down" }
                ListElement { name: "seekBack"; desc: "快退"; defau: "Ctrl+Left" }
                ListElement { name: "seekForward"; desc: "快进"; defau: "Ctrl+Right" }
                ListElement { name: "mute"; desc: "静音切换"; defau: "Ctrl+M" }
                ListElement { name: "abLoop"; desc: "A-B 片段循环"; defau: "Ctrl+B" }
                ListElement { name: "favorite"; desc: "收藏当前曲目"; defau: "Ctrl+D" }
                ListElement { name: "playerOptions"; desc: "播放器选项"; defau: "Ctrl+T" }
            }

            // 内容
            Column {
                id: shortcutContent
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "快捷键"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                }

                // 全局开关
                QHead { text: "全局快捷键" }

                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "启用全局快捷键"
                            tip: "开启后可在其他窗口用快捷键控制播放"
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

                // 按键捕获器（隐藏）
                Rectangle {
                    id: keyCapture
                    height: shortcutset.isRecording ? 60 : 0
                    width: settingStack.standWidth
                    color: Style.fontColor
                    border.width: 1
                    border.color: Style.sideColor
                    radius: 16
                    focus: false
                    visible: true          // 必须可见才能获得焦点
                    opacity: shortcutset.isRecording ? 1 : 0 // 透明，不干扰界面
                    enabled: shortcutset.isRecording   // 仅在录制时启用
                    Behavior on opacity { NumberAnimation { duration: 120 } }
                    Behavior on height { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
                    Keys.onPressed: (event) => {
                        if (!shortcutset.isRecording) return;
                        // 按 Esc 取消
                        if (event.key === Qt.Key_Escape) {
                            shortcutset.stopRecording(false)
                            event.accepted = true
                            Options.warn.tiped("已取消", 0)
                            return
                        }
                        const seq = shortcutset.keyEventToSequence(event)
                        if (seq) {
                            shortcutset.stopRecording(true, seq)
                            Options.warn.tiped("设置已保存", 1)
                            event.accepted = true
                        }
                        // 如果是无效键（如单独的修饰键），不处理，等待有效组合
                    }
                    Row {
                        anchors.centerIn: parent
                        spacing: 8
                        QButton {
                            width: 100
                            height: 34
                            text: "取消[Esc]"
                            buttonColor: Style.primaryColor
                            borderWidth: 1
                            onClicked: {
                                shortcutset.stopRecording(false);
                                Options.warn.tiped("已取消", 0);
                            }
                        }
                        Text {
                            id: keyCaptureText
                            anchors.verticalCenter: parent.verticalCenter
                            font.pixelSize: Style.settings.textmain
                            color: Style.secondaryColor
                            text: "正在键位录制状态，请输入一个键来设置" + shortcutset.recordingAction + "功能的快捷键"
                        }
                    }
                }

                // 快捷键列表
                QHead { text: "快捷键列表" }
                Rectangle {
                    width: settingStack.standWidth
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    height: shortCutColumn.height // 自适应高度

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
                                        color: Style.fontColor
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: Options.shortCuts[model.name] || model.defau
                                        width: 120
                                        font.pixelSize: Style.settings.textmain
                                        color: Style.fontColor
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }

                                    Label {
                                        text: "默认: " + model.defau
                                        width: 120
                                        font.pixelSize: Style.settings.textmain
                                        color: Qt.rgba(Style.fontColor.r, Style.fontColor.g, Style.fontColor.b, 0.6)
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                }
                                QSwitch {
                                    x: parent.width - 336
                                    y: 10
                                    width: 96
                                    height: 36
                                    switchTrue: Options.shortCuts[shortcutset.globalPropertyName(model.name)]
                                    onToggled: {
                                        const prop = shortcutset.globalPropertyName(model.name);
                                        Options.shortCuts[prop] = !Options.shortCuts[prop];
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
                                        Options.warn.tiped("已恢复默认快捷键", 1)
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
                    color: Style.textColor
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                }
            }
        }

        // 插件设置
        Item {
            id: modset
            width: settingStack.width
            height: settingStack.height - 60
            visible: false
            property int tabIndex: 0        // 子页签：0 歌词界面 / 1 功能 / 2 音乐源

            Text {
                x: 24 + settingStack.containX
                y: 24
                width: settingStack.standWidth
                height: 36
                color: Style.fontColor
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
                model: ["歌词界面","功能","音乐源"]
                tabWidth: 100
                width: 304
                rectXy: Qt.rect(0, 10, width, 40)
                blurSource: downloadChildPage
                onTabChange: (index) => {
                    modset.tabIndex = index
                    downloadChildPage.stack(index)
                }
            }

            // 「音乐源」尚未实现，仅在该子页签提示
            Rectangle {
                visible: modset.tabIndex === 2
                x: 24 + settingStack.containX
                y: 124
                width: settingStack.standWidth
                height: warnModText.implicitHeight + 48
                color: Style.containColor
                radius: Style.settings.cubeRadius
                border.color: Style.sideColor
                border.width: 1
                Text {
                    x: 24
                    y: 24
                    font.family: IconFont.name
                    height: warnModText.implicitHeight
                    text: "\uf11a"
                    color: Style.themeColor
                    font.pixelSize: Style.settings.texticon
                }
                Text {
                    id: warnModText
                    x: 48
                    y: 24
                    width: parent.width - 64
                    text: "音乐源插件还未开发完成，等待开发者更新喵"
                    wrapMode: Text.Wrap
                    color: Style.textColor
                    font.bold: false
                    font.pixelSize: Style.settings.textmain
                }
            }

            QPages {
                x: 24 + settingStack.containX
                y: 60
                width: settingStack.standWidth
                height: parent.height - 60
                id: downloadChildPage
                pageList: [uiMod,toolMod,musicMod]
                Item {
                    id: uiMod
                    visible: true
                    width: downloadChildPage.width
                    height: downloadChildPage.height

                    PluginManager {
                        anchors.fill: parent
                        store: LyricsPlugins
                        pluginKind: "歌词界面插件"
                    }
                }
                Item {
                    id: toolMod
                    visible: false
                    width: downloadChildPage.width
                    height: downloadChildPage.height

                    PluginManager {
                        anchors.fill: parent
                        store: FunctionPlugins
                        pluginKind: "功能插件"
                        multi: true
                    }
                }
                Item {
                    id: musicMod
                    visible: false
                    width: downloadChildPage.width
                    height: downloadChildPage.height
                    Text {
                        anchors.fill: parent
                        text: "音乐源"
                        verticalAlignment: Text.AlignVCenter
                        horizontalAlignment: Text.AlignHCenter
                        color: Style.textColor
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

            Column {
                id: aboutCol
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "关于应用"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                // 头标
                Column {
                    width: settingStack.standWidth
                    spacing: 32
                    topPadding: 64
                    bottomPadding: 64
                    //icon
                    Image {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 96
                        height: 96
                        source: "qrc:/QueMusic/resources/icon.ico"
                        sourceSize: Qt.size(96, 96)
                    }

                    // App Title
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: 56
                        width: implicitWidth// + 96
                        text: "QueMusic"
                        font.family: TextFont.name
                        font.pixelSize: 56
                        font.bold: false
                        verticalAlignment: Text.AlignVCenter
                        color: Style.fontColor
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "一款基于 C++/Qt Quick 框架开发的高性能音乐播放器"
                        wrapMode: Text.Wrap
                        color: Style.textColor
                        font.bold: false
                        font.pixelSize: Style.settings.textH2
                    }
                }

                Rectangle {
                    width: settingStack.standWidth
                    height: 60
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    SettingItem {
                        width: settingStack.standWidth - 32
                        label: "QueMusic Beta (测试版本)"
                        controlWidth: 120
                        height: 36
                        x: 16
                        y: 12
                        Row {
                            anchors.right: parent.right
                            height: 36
                            spacing: 12
                            Text {
                                height: 36
                                text: Options.version + " (" + Options.versionCode + ")"
                                color: Style.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                            QButton {
                                height: 36
                                width: 100
                                text: "检查更新"
                                buttonColor: Style.themeColor
                                textColor: Style.primaryColor
                                iconColor: Style.primaryColor
                                shadowEnabled: false
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
                    color: Style.containColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.sideColor
                    border.width: 1
                    Text {
                        x: 20
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: IconFont.name
                        height: warnText.implicitHeight
                        text: "\uf11a"
                        color: Style.themeColor
                        font.pixelSize: Style.settings.texticon
                    }
                    Text {
                        id: warnText
                        x: 48
                        y: 20
                        width: parent.width - 108
                        text: "该版本属于开发中Beta版本，是未正式发布的开发中测试版本，部分功能仍未有效，并且稳定性欠佳，非最终质量"
                        wrapMode: Text.Wrap
                        color: Style.fontColor
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
                    color: Style.containColor
                    radius: Style.settings.cubeRadius
                    border.color: Style.sideColor
                    border.width: 1
                    Text {
                        x: 20
                        anchors.verticalCenter: parent.verticalCenter
                        font.family: IconFont.name
                        height: warnMoneyText.implicitHeight
                        text: "\uf11a"
                        color: Style.themeColor
                        font.pixelSize: Style.settings.texticon
                    }
                    Text {
                        id: warnMoneyText
                        x: 48
                        y: 20
                        width: parent.width - 108
                        text: "QueMusic Beta（官方版）始终是完全免费且开源的软件，不存在付费，会员，捐献，充值，广告等入口，官方版本不存在Pro，Ultra，高级版等版本，如果你发现软件或软件内有需要付费的内容，请立即与开发者联系。"
                        wrapMode: Text.Wrap
                        color: Style.fontColor
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
                    color: settingStack.cardColor
                    radius: Style.settings.cubeRadius
                    Text {
                        id: description
                        anchors.centerIn: parent
                        width: settingStack.standWidth - 48
                        text: "QueMusic是一个基于Qt QML开发的全能音乐播放器，旨在让听歌变得更简单，在Qt RHI * QML * C++强大组合下，性能卓越，UI美观丝滑，基于C++的在线音源使其拥有强大的稳定在线体验，并且由此实现美观强大的自定义功能，QueMusic希望做一个全能的音乐播放器。"
                        wrapMode: Text.Wrap
                        color: Style.textColor
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
                        color: settingStack.cardColor
                    }
                }

                QHead { text: "技术践与版本信息" }

                Rectangle {
                    width: settingStack.standWidth
                    height: 238
                    color: settingStack.cardColor
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
                                text: Options.version + " (" + Options.versionCode + ") (Beta)"
                                color: Style.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "QuePlugin API版本"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: Options.pluginApi + " (Beta)"
                                color: Style.textColor
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
                                text: "Qt-" + qtRuntimeVersion + "(Community)"
                                color: Style.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "渲染技术"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "QRHI/Qt Scene Graph"
                                color: Style.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "主体技术"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "C++/JS/QtQuick"
                                color: Style.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                QHead { text: "主要使用的第三方项目" }

                Grid {
                    spacing: 24
                    columns: 2
                    rows: 2
                    width: settingStack.standWidth
                    AccountCard {
                        source: "qrc:/QueMusic/resources/app/icons/qwk.png"
                        title: "QWindowKit"
                        text: "实现全平台完美的无边框窗口"
                        openUrl: "https://github.com/stdware/qwindowkit"
                        color: settingStack.cardColor
                    }
                    AccountCard {
                        source: "qrc:/QueMusic/resources/app/icons/qticon.png"
                        title: "Qt Community"
                        text: "强大的开源跨平台软件包框架"
                        openUrl: "https://github.com/qt"
                        color: settingStack.cardColor
                    }
                    AccountCard {
                        source: ""
                        title: "QCloudMusicApi"
                        text: "网易云音乐第三方API服务框架"
                        openUrl: "https://github.com/s12mmm3/QCloudMusicApi"
                        color: settingStack.cardColor
                    }
                }

                QHead { text: "联系开发者" }

                Rectangle {
                    width: settingStack.standWidth
                    height: 200
                    color: settingStack.cardColor
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
                                color: Style.textColor
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
                                color: Style.textColor
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
                                color: Style.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        SettingItem {
                            width: settingStack.standWidth - 32
                            label: "QQ群组"
                            controlWidth: 120
                            Text {
                                anchors.right: parent.right
                                height: 36
                                text: "1105114511"
                                color: Style.textColor
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }

                Text { text: "期待您的贡献与反馈"; color: Style.fontColor; font.pixelSize: Style.settings.textmain }

                Row {
                    spacing: 20

                    QButton {
                        height: 40
                        radius: 20
                        text: "Github"
                        iconCharacter: "\uf0dd"
                        onClicked: {
                            Qt.openUrlExternally("https://github.com/bronekox/quemusic");
                        }
                    }

                    QButton {
                        height: 40
                        radius: 20
                        text: "QueMusic Web"
                        iconCharacter: "\uf0d7"
                        onClicked: {
                            Qt.openUrlExternally("https://quemusic.top");
                        }
                    }

                    QButton {
                        height: 40
                        radius: 20
                        text: "Bug反馈/issue"
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
                Item {
                    width: parent.width
                    height: 36
                    Text {
                        anchors.centerIn: parent
                        color: Style.fontColor
                        font.pixelSize: Style.settings.textmain
                        text: "Copyright (c) 2025-2026 QueMusic Contributors\n   本项目基于 Apache License 2.0 协议获得许可"
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

            Column {
                id: deBug
                spacing: 16
                padding: 24
                width: settingStack.containWidth
                x: settingStack.containX

                Text {
                    width: settingStack.standWidth
                    height: 40
                    color: Style.fontColor
                    verticalAlignment: Text.AlignVCenter
                    text: "DeBug"
                    font.pixelSize: Style.settings.pageTitle
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.3
                }

                Text { text: "本页设置仅供调试，可能会出现崩溃甚至软件失效，如要恢复请到软件配置目录删除"; color: Style.fontColor; font.pixelSize: Style.settings.textmain }

                QHead { text: "渲染" }

                Rectangle {
                    width: settingStack.standWidth
                    color: Style.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "使用系统标题栏"
                            tip: "改用系统原生标题栏，关闭无边框窗口效果"
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
                            label: "渲染引擎"
                            tip: "选择界面渲染使用的图形后端，重启后生效"
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
                            label: "Timer驱动界面"
                            tip: "用QTimer引擎驱动动画，代替Vsync垂直同步"
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
                            tip: "选择 QML 动画的驱动方式，重启后生效"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                text: switchTrue ? "Vsync" : "默认"
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
                    color: Style.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "显示渲染帧率"
                            tip: "在界面右上角显示实时帧率"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: Options.settings.displayFps
                                onToggled: Options.settings.displayFps = !Options.settings.displayFps
                            }
                        }

                        SettingItemCard {
                            label: "调试模式"
                            tip: "显示运行状态调试面板"
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
                    color: Style.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        width: parent.width
                        padding: 0
                        Component.onCompleted: parent.height = height

                        SettingItemCard {
                            label: "启用日志"
                            tip: "开启后把运行日志写入本地文件"
                            controlItem: QSwitch {
                                anchors.fill: parent
                                letRight: true
                                switchTrue: LogManager.enabled
                                onToggled: LogManager.enabled = !LogManager.enabled
                            }
                        }

                        SettingItemCard {
                            label: "记录等级"
                            tip: "设置写入日志的最低等级"
                            controlItem: QDrop {
                                anchors.fill: parent
                                choice: LogManager.minimumLevel
                                model: ["调试","信息","警告","错误","致命"]
                                onTransformed: (choiced) => { LogManager.minimumLevel = choiced }
                            }
                        }

                        SettingItemCard {
                            label: "打开日志目录"
                            tip: "在文件管理器中打开日志文件所在目录"
                            controlItem: QButton {
                                anchors.fill: parent
                                text: "打开"
                                shadowEnabled: false
                                buttonColor: "transparent"
                                radius: Style.settings.labelRadius
                                borderWidth: 2
                                onClicked: LogManager.openLogFolder()
                            }
                            bottomLine: false
                        }
                    }
                }

                Rectangle {
                    width: settingStack.standWidth
                    height: 320
                    color: Style.primaryColor
                    radius: Style.settings.cubeRadius
                    Column {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8
                        Text {
                            width: parent.width
                            text: "实时日志预览（完整内容见日志文件）"
                            color: Style.textColor
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
                                text: LogManager.logPreview
                                color: Style.textColor
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
        property int localVersion: Options.versionCode

        // 远程 version.txt 的 URL
        property string remoteVersionUrl: "https://raw.githubusercontent.com/BroNekoX/QueMusic/main/doc/updater.txt"
        property int newVersion: Options.versionCode
        property string versionDescription: ""

        function checkForUpdate(): void {
            console.log("正在检查更新...");
            Options.warn.tiped("正在检查更新", 0);

            const xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState === XMLHttpRequest.DONE) {
                    if (xhr.status === 200) {
                        const text = xhr.responseText.trim();
                        const remoteVersion = parseInt(text.substring(0,3));
                        console.log("远程版本号:", remoteVersion);

                        if (remoteVersion > localVersion) {
                            console.log("发现新版本!");
                            newVersion = remoteVersion;
                            updater.versionDescription = text.substring(4);
                            // 显示更新提示对话框
                            updateDialog.open();
                        } else {
                            console.log("当前已是最新版本");
                            // 可选：显示“已是最新”的提示
                            Options.warn.tiped("当前已是最新版本", 1);
                        }
                    } else {
                        console.error("检查更新失败，HTTP状态码:", xhr.status);
                        Options.warn.tiped("检查更新失败，请稍后重试", 2);
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
            message: "发现QueMusic的新版本：(v" + updater.newVersion + ")，是否前往更新？\n 版本更新日志：" + updater.versionDescription
            isInput: false
            blurSource: settingsView
            confirmText: "更新"

            onConfirm: {
                Qt.openUrlExternally("https://github.com/BroNekoX/QueMusic/releases");
            }
        }
    }
}
