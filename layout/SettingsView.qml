// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick

import QueMusic 1.0

Item {
    id: settingsView

    // 账号登录面板展开状态
    property bool neteaseShowLogin: false
    property bool kugouShowLogin: false
    property bool neteaseAltVisible: false
    property string neteasePhone: ""
    property string neteaseCaptcha: ""
    readonly property bool neteasePhoneReady: neteasePhone !== "" && neteaseCaptcha !== ""
    property string neteaseLoginStatus: "等待登录…"
    property string kugouLoginStatus: "等待登录…"
    property url backSource
    property color sidebarColor
    property color mainContentColor

    Image {
        id: settingsBackGround
        z: 0
        x: 0
        y: 0
        width: parent.width
        height: parent.height
        source: settingsView.backSource
        visible: Style.backmode === 2 || Style.backmode === 3
        asynchronous: true
        sourceSize: Qt.size(width, height)
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
                color: Options.darkis ? Theme.secondaryColor : "#ffffff"
                border.color: Theme.secondaryColor
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
                color: Theme.fontColor
                font.pixelSize: Style.textmain
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
                buttonColor: Theme.themeColor
                textColor: Theme.primaryColor
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
                buttonColor: Theme.themeColor
                textColor: Theme.primaryColor
                onClicked: AccountManager.startNeteaseQrLogin()
            }

            // 备用登录方式（扫码提示"环境异常"时用）
            Text {
                width: parent.width
                text: "手机号登录被网易云风控拦截时，可改用：粘贴浏览器里的 Cookie（最稳）"
                color: Theme.fontColor
                opacity: 0.7
                font.pixelSize: Style.textmain - 3
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
                buttonColor: Theme.secondaryColor
                textColor: Theme.fontColor
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

                QTextField {
                    id: neteasePhoneInput
                    width: parent.width
                    height: 36
                    leftPadding: 12
                    placeholderText: "手机号"
                    color: Theme.textColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                    background: Rectangle {
                        radius: 8
                        color: Theme.primaryColor
                        border.color: Theme.secondaryColor
                        border.width: 1
                    }
                }

                Row {
                    width: parent.width
                    spacing: 10

                    QTextField {
                        id: neteaseCaptchaInput
                        width: parent.width - 130
                        height: 36
                        leftPadding: 12
                        placeholderText: "短信验证码"
                        color: Theme.textColor
                        font.pixelSize: Style.textmain
                        verticalAlignment: Text.AlignVCenter
                        background: Rectangle {
                            radius: 8
                            color: Theme.primaryColor
                            border.color: Theme.secondaryColor
                            border.width: 1
                        }
                    }

                    QButton {
                        width: 120
                        height: 36
                        radius: Style.labelRadius
                        shadowEnabled: false
                        text: "发送验证码"
                        buttonColor: Theme.secondaryColor
                        textColor: Theme.fontColor
                        onClicked: AccountManager.sendNeteaseCaptcha(neteasePhoneInput.text)
                    }
                }

                QButton {
                    width: 150
                    height: 36
                    radius: Style.labelRadius
                    shadowEnabled: false
                    text: "手机号登录"
                    buttonColor: Theme.themeColor
                    textColor: Theme.primaryColor
                    onClicked: AccountManager.loginNeteaseWithCellphone(neteasePhoneInput.text,
                                                                      neteaseCaptchaInput.text)
                }

                // 发送验证码/登录的进度与失败原因：弹窗上方那条提示滚上去就看不见了，这里就近再显示一份
                Text {
                    width: parent.width
                    text: AccountManager.neteaseQrMessage
                    color: Theme.fontColor
                    opacity: 0.7
                    font.pixelSize: Style.textmain - 3
                    wrapMode: Text.WordWrap
                    horizontalAlignment: Text.AlignHCenter
                }

                QButton {
                    visible: AccountManager.neteaseVerifyUrl !== ""
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "去完成安全验证"
                    width: 150
                    height: 36
                    radius: Style.labelRadius
                    shadowEnabled: false
                    buttonColor: Theme.themeColor
                    textColor: Theme.primaryColor
                    onClicked: AccountManager.openNeteaseVerifyUrl()
                }
            }

            // 粘贴 Cookie 登录
            Column {
                width: parent.width
                spacing: 10
                visible: settingsView.neteaseAltVisible

                QTextField {
                    id: neteaseCookieInput
                    width: parent.width
                    height: 36
                    leftPadding: 12
                    placeholderText: "粘贴含 MUSIC_U 的 Cookie"
                    color: Theme.textColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                    background: Rectangle {
                        radius: 8
                        color: Theme.primaryColor
                        border.color: Theme.secondaryColor
                        border.width: 1
                    }
                }

                QButton {
                    width: 150
                    height: 36
                    radius: Style.labelRadius
                    shadowEnabled: false
                    text: "用 Cookie 登录"
                    buttonColor: Theme.themeColor
                    textColor: Theme.primaryColor
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
                color: Options.darkis ? Theme.secondaryColor : "#ffffff"
                border.color: Theme.secondaryColor
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
                color: Theme.fontColor
                font.pixelSize: Style.textmain
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
                buttonColor: Theme.themeColor
                textColor: Theme.primaryColor
                onClicked: AccountManager.startKugouQrLogin()
            }
        }
    }

    QOptionDialog {
        id: platformLoginDialog
        property string platform: "Spotify"
        property string cookieText: ""
        readonly property string displayName: platform === "Spotify" ? "Spotify" : "Apple Music"
        readonly property string loginMessage: platform === "Spotify" ? AccountManager.spotifyLoginMessage : AccountManager.appleLoginMessage
        title: displayName + " - 浏览器登录"
        cancelText: "取消"
        confirmText: "关闭"
        blurSource: settingsView
        onCancel: close()
        onConfirm: close()

        function openFor(name) {
            platform = name
            cookieText = ""
            open()
            if (name === "Spotify") AccountManager.startSpotifyBrowserLogin()
            else AccountManager.startAppleBrowserLogin()
        }

        options: Column {
            width: parent.width
            spacing: 10
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                color: Theme.textColor
                font.pixelSize: Style.textmain - 1
                text: platformLoginDialog.platform === "Spotify"
                    ? "在浏览器登录 Spotify，复制含 sp_dc 的 Cookie 后粘贴到下方。"
                    : "在浏览器登录 Apple Music，复制含 media-user-token 的 Cookie 后粘贴到下方。"
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                color: Theme.fontColor
                font.pixelSize: Style.textmain
                text: platformLoginDialog.loginMessage
            }
            QButton {
                width: 150
                height: 34
                radius: 17
                shadowEnabled: false
                text: "打开浏览器登录"
                buttonColor: Theme.themeColor
                textColor: Theme.primaryColor
                onClicked: platformLoginDialog.platform === "Spotify"
                    ? AccountManager.startSpotifyBrowserLogin()
                    : AccountManager.startAppleBrowserLogin()
            }
            QTextField {
                id: cookieInput
                width: parent.width
                height: 38
                leftPadding: 12
                text: platformLoginDialog.cookieText
                onTextChanged: platformLoginDialog.cookieText = text
                placeholderText: platformLoginDialog.platform === "Spotify" ? "粘贴含 sp_dc 的 Cookie" : "粘贴含 media-user-token 的 Cookie"
                color: Theme.textColor
                font.pixelSize: Style.textmain
                background: Rectangle {
                    radius: 8
                    color: Theme.primaryColor
                    border.color: Theme.secondaryColor
                    border.width: 1
                }
            }
            QButton {
                width: 150
                height: 34
                radius: 17
                shadowEnabled: false
                text: "完成登录"
                buttonColor: Theme.themeColor
                textColor: Theme.primaryColor
                onClicked: platformLoginDialog.platform === "Spotify"
                    ? AccountManager.loginSpotifyWithCookie(platformLoginDialog.cookieText)
                    : AccountManager.loginAppleWithCookie(platformLoginDialog.cookieText)
            }
        }

        Connections {
            target: AccountManager
            function onSpotifyLoginChanged() {
                if (platformLoginDialog.platform === "Spotify" && AccountManager.spotifyLoginMessage === "Spotify 登录成功") {
                    platformLoginDialog.close()
                    MusicApi.songSource = 4
                }
            }
            function onAppleLoginChanged() {
                if (platformLoginDialog.platform === "Apple Music" && AccountManager.appleLoginMessage === "Apple Music 登录成功") {
                    platformLoginDialog.close()
                    MusicApi.songSource = 5
                }
            }
        }
    }

    QOptionDialog {
        id: serverDialog
        title: "插件市场设置"
        cancelText: "取消"
        confirmText: "确定"
        blurSource: settingsView
        property string serverText: Config.pluginServer

        onConfirm: {
            Config.pluginServer = serverDialog.serverText.trim();
            PluginMarket.reloadConfig();
            PluginMarket.refresh();
        }

        options: Column {
            width: parent.width
            spacing: 16

            SettingItem {
                label: "服务器地址"
                width: parent.width
                isBigItem: true

                QInput {
                    id: serverField
                    y: 40
                    width: parent.width
                    height: 36
                    inputText: Config.pluginServer
                    onInputTextChanged: serverDialog.serverText = inputText;
                }
            }

            QButton {
                height: 36
                text: "恢复默认"
                iconCharacter: "\uf0e2"
                shadowEnabled: false
                buttonColor: Theme.secondaryColor
                borderColor: Theme.sideColor
                borderWidth: 1
                onClicked: serverField.inputText = PluginMarket.defaultSourceUrl
            }
        }
    }

    Text {
        x: leftSidebarSettings.width + settingStack.containX + 26
        z: 3
        height: 24
        text: navModelSettings.get(leftSidebarSettings.setChoiceIndex).display
        color: Theme.textColor
        font.pixelSize: 14
        verticalAlignment: Text.AlignVCenter
        visible: settingStack.setPages[leftSidebarSettings.setChoiceIndex].contentY > 64
        opacity: visible ? 1 : 0
        y: visible ? 18 : 36
        Behavior on opacity { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        Behavior on y { NumberAnimation { duration: 320; easing.type: Easing.OutExpo } }
    }

    Rectangle {
        id: leftSidebarSettings
        x: 0
        y: 0
        z: 5
        height: settingsView.height
        width: Options.barWidth
        opacity: 1
        property color choiceColor: Theme.hoverColor
        property color choiceTextColor: Theme.fontColor
        property int setChoiceIndex: 0
        color: settingsView.sidebarColor
        Connections {
            target: Options
            function onChangeTheme(): void {
                if(Style.sidebarStyle === 0) {
                    leftSidebarSettings.choiceColor = Theme.hoverColor;
                    leftSidebarSettings.choiceTextColor = Theme.fontColor;
                    choicebar1.x = 18;
                    choicebar1.radius = 2;
                } else if(Style.sidebarStyle === 1) {
                    leftSidebarSettings.choiceColor = Theme.themeColor;
                    leftSidebarSettings.choiceTextColor = Theme.primaryColor;
                    choicebar1.x = 0;
                    choicebar1.radius = 0;
                }
            }
        }
        Component.onCompleted: {
            index1ed(0);
            if(Style.sidebarStyle === 0) {
                leftSidebarSettings.choiceColor = Theme.hoverColor;
                leftSidebarSettings.choiceTextColor = Theme.fontColor;
                choicebar1.x = 18;
                choicebar1.radius = 2;
            } else if(Style.sidebarStyle === 1) {
                leftSidebarSettings.choiceColor = Theme.themeColor;
                leftSidebarSettings.choiceTextColor = Theme.primaryColor;
                choicebar1.x = 0;
                choicebar1.radius = 0;
            }
        }

        function index1ed(choice: int): void {
            if(Options.isMacOS) {
                choicebar1.willBarY = 44 * choice + 120
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
                settingStack.setPages[leftSidebarSettings.setChoiceIndex].contentY = 0;
                settingStack.setPages[leftSidebarSettings.setChoiceIndex].scrollToY = 0;
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
            y: Options.isMacOS ? 100 : 80
            topRightRadius: 2
            bottomRightRadius: 2
            radius: 2
            color: Theme.themeColor
            opacity: 1
            property int barBottom: Options.isMacOS ? 122 : 102
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
            x: 16;y: Options.isMacOS ? 62 : 12
            height: 36
            width: Options.isSmall ? 74 : 180
            QWKButton {
                id: returnButton
                width: 36
                height: 36
                source: Options.darkis ? "qrc:/QueMusic/resources/window-bar/returnd.svg" : "qrc:/QueMusic/resources/window-bar/return.svg"
                background: Rectangle {
                    color: returnButton.hovered ? Theme.hoverColor : "transparent"
                    radius: Style.noControlRadius ? Style.labelRadius : 8
                    Behavior on color { ColorAnimation { duration: 50 } }
                }
                onClicked: Options.settingOutAnime.running = true;
            }

            Text {
                id: setText
                x: 50
                y: 0
                width: 120
                height: 36
                text: "应用设置"
                font.pixelSize: 14
                font.bold: false
                verticalAlignment: Text.AlignVCenter
                color: Theme.fontColor
                opacity: Options.isSmall ? 0 : 1
                visible: opacity !== 0
                Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
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

        Repeater {
            model: navModelSettings

            delegate: Rectangle {
                id: navDelegateSettings
                width: Options.barWidth - 30
                height: 42
                x: 15
                radius: Style.labelRadius
                color: isSelected ? leftSidebarSettings.choiceColor : "transparent"
                Component.onCompleted: {
                    if(Options.isMacOS) {
                        y = 44 * index + 110
                    } else {
                        y = 44 * index + 70
                    }
                }

                property bool isSelected: leftSidebarSettings.setChoiceIndex === index
                required property int index
                required property var model

                scale: 1.0
                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on scale { NumberAnimation { duration: 80; easing.type: Easing.OutCubic } }

                Rectangle {
                    anchors.fill: parent
                    radius: Style.labelRadius
                    color: Theme.hoverColor
                    opacity: barMouse.containsMouse ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 80 } }
                }

                Text {
                    x: 1 + setText.opacity * 5
                    y: 0
                    z: 1
                    width: 42
                    height: 42
                    text: navDelegateSettings.model.iconChar
                    font.family: Fonts.icon
                    font.pixelSize: Style.texticon
                    color: navDelegateSettings.isSelected ? leftSidebarSettings.choiceTextColor : Theme.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                Text {
                    x: 52
                    y: 0
                    z: 2
                    width: 140
                    height: 42
                    opacity: setText.opacity
                    visible: setText.visible
                    text: navDelegateSettings.model.display
                    color: navDelegateSettings.isSelected ? leftSidebarSettings.choiceTextColor : Theme.textColor
                    font.bold: navDelegateSettings.isSelected
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                }

                MouseArea {
                    id: barMouse
                    anchors.fill: parent
                    hoverEnabled: true

                    onPressed: navDelegateSettings.scale = 0.96
                    onReleased: navDelegateSettings.scale = 1.0
                    onCanceled: navDelegateSettings.scale = 1.0
                    onClicked: {
                        leftSidebarSettings.index1ed(navDelegateSettings.index)
                        leftSidebarSettings.setChoiceIndex = navDelegateSettings.index
                    }
                }
            }
        }
    }

    Rectangle {
        id: settingStack
        x: leftSidebarSettings.width
        width: parent.width - x
        height: parent.height
        color: settingsView.mainContentColor
        z: 2
        property color cardColor: Style.backmode === 0 ? Theme.primaryColor : Theme.primaryBlurColor
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

        property int standWidth: parent.width > 1410 ? 1148 : parent.width - Options.barWidth - 52
        property int containWidth: parent.width > 1410 ? 1200 : parent.width - Options.barWidth
        property int containX: parent.width > 1410 ? parent.width / 2 - 705 : 0

        GeneralSettingsPage {
            id: themeset
            visible: true
            standWidth: settingStack.standWidth
            containWidth: settingStack.containWidth
            containX: settingStack.containX
            cardColor: settingStack.cardColor
            blurSource: settingsView
            onNeteaseLoginRequested: {
                settingsView.neteaseShowLogin = true
                neteaseQrDialog.open()
                AccountManager.startNeteaseQrLogin()
            }
            onKugouLoginRequested: {
                settingsView.kugouShowLogin = true
                kugouQrDialog.open()
                AccountManager.startKugouQrLogin()
            }
            onPlatformLoginRequested: (platform) => platformLoginDialog.openFor(platform)
        }

        InterfaceSettingsPage {
            id: uiset
            standWidth: settingStack.standWidth
            containWidth: settingStack.containWidth
            containX: settingStack.containX
            cardColor: settingStack.cardColor
            blurSource: settingsView
        }

        PlayerSettingsPage {
            id: toolset
            standWidth: settingStack.standWidth
            containWidth: settingStack.containWidth
            containX: settingStack.containX
            cardColor: settingStack.cardColor
        }

        AudioSettingsPage {
            id: playerset
            standWidth: settingStack.standWidth
            containWidth: settingStack.containWidth
            containX: settingStack.containX
            cardColor: settingStack.cardColor
        }

        ShortcutSettingsPage {
            id: shortcutset
            standWidth: settingStack.standWidth
            containWidth: settingStack.containWidth
            containX: settingStack.containX
            cardColor: settingStack.cardColor
        }

        PluginSettingsPage {
            id: modset
            standWidth: settingStack.standWidth
            containWidth: settingStack.containWidth
            containX: settingStack.containX
            cardColor: settingStack.cardColor
        }

        AboutSettingsPage {
            id: aboutset
            standWidth: settingStack.standWidth
            containWidth: settingStack.containWidth
            containX: settingStack.containX
            cardColor: settingStack.cardColor
            blurSource: settingsView
        }

        DebugSettingsPage {
            id: debugset
            standWidth: settingStack.standWidth
            containWidth: settingStack.containWidth
            containX: settingStack.containX
            cardColor: settingStack.cardColor
        }
    }
}
