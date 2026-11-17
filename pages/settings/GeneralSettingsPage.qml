// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//

pragma ComponentBehavior: Bound
import QtQuick

import QtQuick.Dialogs
import QtCore
import QueMusic 1.0

QScrollView {
    id: page
    property int standWidth: 0
    property int containWidth: 0
    property int containX: 0
    property color cardColor: Theme.primaryColor
    property Item blurSource: null

    y: 60
    width: parent ? parent.width : 0
    height: parent ? parent.height - 60 : 0
    visible: false

    signal neteaseLoginRequested()
    signal kugouLoginRequested()
    signal platformLoginRequested(string platform)

    Connections {
        target: AccountManager
        function onBilibiliLoginChanged(): void {
            if (AccountManager.bilibiliLoggedIn)
                bilibiliLoginDialog.close()
        }
    }

    QOptionDialog {
        id: bilibiliLoginDialog
        title: "哔哩哔哩 - 扫码登录"
        cancelText: "取消"
        confirmText: "关闭"
        blurSource: page.blurSource
        onCancel: { AccountManager.cancelBilibiliQrLogin(); close() }
        onClosed: AccountManager.cancelBilibiliQrLogin()

        options: Column {
            width: parent.width
            spacing: 12

            Rectangle {
                width: 232
                height: 232
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 12
                color: Options.darkis ? Theme.secondaryColor : "#ffffff"
                border.color: Theme.secondaryColor
                border.width: 1
                QRCodeView {
                    width: 212
                    height: 212
                    anchors.centerIn: parent
                    qrText: AccountManager.bilibiliQrText
                }
            }

            Text {
                width: parent.width
                text: AccountManager.bilibiliQrMessage
                color: Theme.fontColor
                font.pixelSize: Style.textmain
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            QButton {
                visible: AccountManager.bilibiliQrState === 3 || AccountManager.bilibiliQrState === 4
                anchors.horizontalCenter: parent.horizontalCenter
                text: "重新获取二维码"
                width: 150
                height: 34
                radius: 17
                shadowEnabled: false
                buttonColor: Theme.themeColor
                textColor: Theme.primaryColor
                onClicked: AccountManager.startBilibiliQrLogin()
            }

            Text {
                width: parent.width
                text: "扫码失败时可粘贴浏览器里的 Cookie（含 SESSDATA）"
                color: Theme.fontColor
                opacity: 0.7
                font.pixelSize: Style.textmain - 3
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            QTextField {
                id: bilibiliCookieInput
                width: parent.width
                height: 36
                leftPadding: 12
                placeholderText: "SESSDATA=...; bili_jct=...; DedeUserID=..."
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
                onClicked: AccountManager.loginBilibiliWithCookie(bilibiliCookieInput.text)
            }
        }
    }

    QOptionDialog {
        id: qqLoginDialog
        title: "QQ 音乐 - 登录"
        cancelText: "取消"
        confirmText: "关闭"
        blurSource: page.blurSource
        property string cookieText: ""
        onCancel: close()
        onConfirm: close()

        options: Column {
            width: parent.width
            spacing: 10

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                color: Theme.textColor
                font.pixelSize: Style.textmain - 1
                text: "手机号 + 短信验证码登录（推荐）。"
            }

            Row {
                width: parent.width
                spacing: 10
                QTextField {
                    id: qqPhoneInput
                    width: parent.width - 150
                    height: 38
                    leftPadding: 12
                    placeholderText: "手机号"
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
                    width: 140
                    height: 38
                    radius: 19
                    text: "发送验证码"
                    buttonColor: Theme.themeColor
                    textColor: Theme.primaryColor
                    onClicked: AccountManager.requestQQSmsCode(qqPhoneInput.text)
                }
            }

            QTextField {
                id: qqCodeInput
                width: parent.width
                height: 38
                leftPadding: 12
                placeholderText: "短信验证码"
                color: Theme.textColor
                font.pixelSize: Style.textmain
                background: Rectangle {
                    radius: 8
                    color: Theme.primaryColor
                    border.color: Theme.secondaryColor
                    border.width: 1
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                color: Theme.textColor
                font.pixelSize: Style.textmain - 1
                text: "或者：在浏览器打开 y.qq.com 登录后，复制含 uin 与 qqmusic_key 的 Cookie。"
            }

            QTextField {
                id: qqCookieInput
                width: parent.width
                height: 38
                leftPadding: 12
                text: qqLoginDialog.cookieText
                onTextChanged: qqLoginDialog.cookieText = text
                placeholderText: "uin=...; qqmusic_key=..."
                color: Theme.textColor
                font.pixelSize: Style.textmain
                background: Rectangle {
                    radius: 8
                    color: Theme.primaryColor
                    border.color: Theme.secondaryColor
                    border.width: 1
                }
            }

            Text {
                width: parent.width
                text: AccountManager.qqLoginMessage
                color: Theme.fontColor
                font.pixelSize: Style.textmain - 1
                wrapMode: Text.WordWrap
            }

            Row {
                spacing: 10
                QButton {
                    width: 150
                    height: 34
                    radius: 17
                    shadowEnabled: false
                    text: "验证码登录"
                    buttonColor: Theme.themeColor
                    textColor: Theme.primaryColor
                    onClicked: AccountManager.loginQQWithCode(qqPhoneInput.text, qqCodeInput.text)
                }
                QButton {
                    width: 150
                    height: 34
                    radius: 17
                    shadowEnabled: false
                    text: "Cookie 登录"
                    buttonColor: Theme.themeColor
                    textColor: Theme.primaryColor
                    onClicked: AccountManager.loginQQWithCookie(qqLoginDialog.cookieText)
                }
            }
        }
    }

    Column {
        spacing: 16
        padding: 24
        width: page.containWidth
        x: page.containX

        Text {
            height: 40
            color: Theme.fontColor
            verticalAlignment: Text.AlignVCenter
            text: "通用"
            font.pixelSize: Style.pageTitle
            font.weight: Font.DemiBold
        }

        QHead { text: "全局主题" }

        Rectangle {
            width: page.standWidth
            height: globalThemeCard.height
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                id: globalThemeCard
                width: parent.width
                padding: 0

                SettingItemCard {
                    label: "全局主题"
                    tip: "设置应用的全局主题"
                    isBigItem: true
                    QWideDrop {
                        model: ["浅色主题","深色主题","跟随系统"]
                        choice: Style.theme
                        onTransformed: (choiced) => {
                            Style.theme = choiced;
                            Options.changeTheme();
                        }
                    }
                }

                SettingItemCard {
                    label: "全局主题色"
                    tip: "设置应用的全局主要配色"
                    Item {
                        Row {
                            height: 36
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 12

                            Repeater {
                                model: Style.colorList
                                delegate: SButton {
                                    required property int index
                                    required property color modelData
                                    width: 36
                                    height: 36
                                    radius: Style.labelRadius
                                    borderWidth: Style.color === index ? 2 : 0
                                    borderColor: Theme.textColor
                                    iconCharacter: ""
                                    buttonColor: modelData
                                    onClicked: {
                                        Style.color = index;
                                        Options.changeTheme();
                                    }
                                }
                            }
                            SButton {
                                width: 36
                                height: 36
                                radius: Style.labelRadius
                                iconCharacter: "\uf08e"
                                visible: Style.colorList.length > 4
                                buttonColor: "#fa6666"
                                onClicked: {
                                    Style.colorList = Style.colorList.slice(0, Style.colorList.length - 1);
                                }
                            }
                            SButton {
                                width: 36
                                height: 36
                                radius: Style.labelRadius
                                iconCharacter: "\uf008"
                                visible: Style.colorList.length < 8
                                buttonColor: Theme.secondaryColor
                                onClicked: {
                                    themeColorChoose.openColor(Theme.themeColor);
                                }
                            }
                        }
                    }
                    ColorPickerDialog {
                        id: themeColorChoose
                        blurSource: page.blurSource
                        onAccepted: {
                            Style.colorList = Style.colorList.concat([Qt.hsva(selectedColor.hsvHue,0.9,0.8,1.0)]);
                            Options.warned("已添加主题颜色", 1);
                        }
                    }
                }

                SettingItemCard {
                    label: "配色方案"
                    tip: "设置通过主题色来改变整个应用色调的方法"
                    QDrop {
                        choice: Style.colorStyle
                        model: ["默认","Material","FluentUI","QuietUI","Material You"]
                        onTransformed: (choiced) => {
                            Style.colorStyle = choiced;
                            Options.changeTheme();
                        }
                    }
                }

                SettingItemCard {
                    label: "应用背景"
                    tip: "设置在背景显示的样式"
                    isBigItem: true
                    QWideDrop {
                        model: ["默认","浅主题色","云母材质","图片","模糊窗口"]
                        choice: Style.backmode
                        onTransformed: (choiced) => {
                            Style.backmode = choiced
                            Options.changeUi()
                            Options.changeTheme()
                        }
                    }
                }

                SettingsItemVisibleCard {
                    label: "选择背景图片"
                    tip: "设置应用背景壁纸显示的图片"
                    display: Style.backmode === 3
                    Behavior on height { NumberAnimation { duration: 320; easing.type: Easing.OutExpo } }
                    QDrop {
                        choice: Style.backpic
                        model: ["示例1","示例2","示例3","自定义图片"]
                        onTransformed: (choiced) => {
                            Style.backpic = choiced;
                            Options.changeUi();
                            Options.changeTheme();
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
                            Style.backgroundImage = imagefileDialog.selectedFile;
                            Options.warned("已设置背景图片", 1);
                            Options.changeUi();
                            Options.changeTheme();
                        }
                    }
                }
            }
        }

        QHead { text: "在线平台与账户" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
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
                    color: Theme.hoverColor
                    radius: Style.labelRadius
                    Text {
                        x: 12
                        anchors.verticalCenter: parent.verticalCenter
                        width: 30
                        text: "\uf0b6"
                        font.family: Fonts.icon
                        color: Theme.textColor
                        font.pixelSize: Style.texticon
                    }
                    Text {
                        x: 44
                        width: parent.width - 56
                        anchors.verticalCenter: parent.verticalCenter
                        text: "扫码登录：用你自己的账号在应用内登录（账号即能力），登录后自动读取登录态，仅保存在本机，随时可退出。"
                        color: Theme.textColor
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }
                }

                Grid {
                    x: 16
                    width: parent.width - 32
                    height: 288
                    columns: 3
                    rows: 2
                    columnSpacing: 14
                    rowSpacing: 14
                    PlatformCard {
                        text: "酷狗音乐"
                        chooseColor: "#4384F5"
                        chooseColor1: Options.darkis ? "#153A57" : "#CDE8FF"
                        width: page.standWidth / 3 - 20
                        height: 128
                        choose: MusicApi.songSource === 0
                        isLogin: AccountManager.kugouLoggedIn
                        header: AccountManager.kugouAvatar
                        name: AccountManager.kugouNickname
                        onClicked: MusicApi.songSource = 0;
                        onLogined: {
                            if (AccountManager.kugouLoggedIn)
                                Options.dialog.openSimpleDialog("警告", "是否退出账号？", function() { AccountManager.logoutKugou() })
                            else page.kugouLoginRequested()
                        }
                    }
                    PlatformCard {
                        text: "网易云音乐"
                        chooseColor: "#F54343"
                        chooseColor1: Options.darkis ? "#601515" : "#FFCDCD"
                        width: page.standWidth / 3 - 20
                        height: 128
                        choose: MusicApi.songSource === 1
                        isLogin: AccountManager.neteaseLoggedIn
                        header: AccountManager.neteaseAvatar
                        name: AccountManager.neteaseNickname
                        onClicked: MusicApi.songSource = 1;
                        onLogined: {
                            if (AccountManager.neteaseLoggedIn)
                                Options.dialog.openSimpleDialog("警告", "是否退出账号？", function() { AccountManager.logoutNetease() })
                            else page.neteaseLoginRequested()
                        }
                    }
                    PlatformCard {
                        text: "哔哩哔哩"
                        chooseColor: "#FB7299"
                        chooseColor1: Options.darkis ? "#5C2338" : "#FFD9E6"
                        width: page.standWidth / 3 - 20
                        height: 128
                        choose: MusicApi.songSource === 2
                        isLogin: AccountManager.bilibiliLoggedIn
                        header: AccountManager.bilibiliAvatar
                        name: AccountManager.bilibiliNickname
                        loginText: "扫码登录"
                        idleText: "未登录（高音质需登录）"
                        onClicked: MusicApi.songSource = 2;
                        onLogined: {
                            if (AccountManager.bilibiliLoggedIn)
                                Options.dialog.openSimpleDialog("警告", "是否退出账号？", function() { AccountManager.logoutBilibili() })
                            else {
                                AccountManager.startBilibiliQrLogin()
                                bilibiliLoginDialog.open()
                            }
                        }
                    }
                    PlatformCard {
                        text: "QQ音乐"
                        chooseColor: "#31C27C"
                        chooseColor1: Options.darkis ? "#123B2B" : "#D6F5E7"
                        width: page.standWidth / 3 - 20
                        height: 128
                        choose: MusicApi.songSource === 3
                        isLogin: AccountManager.qqLoggedIn
                        header: AccountManager.qqAvatar
                        name: AccountManager.qqNickname
                        loginText: "登录"
                        idleText: "未登录（VIP 歌曲需登录）"
                        onClicked: MusicApi.songSource = 3;
                        onLogined: {
                            if (AccountManager.qqLoggedIn)
                                Options.dialog.openSimpleDialog("警告", "是否退出账号？", function() { AccountManager.logoutQQ() })
                            else {
                                qqLoginDialog.cookieText = ""
                                qqLoginDialog.open()
                            }
                        }
                    }
                    PlatformCard {
                        text: "Spotify"
                        chooseColor: "#1DB954"
                        chooseColor1: Options.darkis ? "#103A24" : "#D7F5E3"
                        width: page.standWidth / 3 - 20
                        height: 128
                        choose: MusicApi.songSource === 4
                        enabled: MusicApi.sourceIds.indexOf(4) !== -1
                        locked: !AccountManager.spotifyLoggedIn
                        isLogin: AccountManager.spotifyLoggedIn
                        header: AccountManager.spotifyAvatar
                        name: AccountManager.spotifyNickname
                        loginText: "登录"
                        idleText: "未登录"
                        onClicked: {
                            if (AccountManager.spotifyLoggedIn) MusicApi.songSource = 4
                            else page.platformLoginRequested("Spotify")
                        }
                        onLogined: {
                            if (AccountManager.spotifyLoggedIn)
                                Options.dialog.openSimpleDialog("警告", "是否退出账号？", function() { AccountManager.logoutSpotify() })
                            else
                                page.platformLoginRequested("Spotify")
                        }
                    }
                    PlatformCard {
                        text: "Apple Music"
                        chooseColor: "#FA243C"
                        chooseColor1: Options.darkis ? "#4A1117" : "#FFD9DD"
                        width: page.standWidth / 3 - 20
                        height: 128
                        choose: MusicApi.songSource === 5
                        enabled: MusicApi.sourceIds.indexOf(5) !== -1
                        locked: !AccountManager.appleLoggedIn
                        isLogin: AccountManager.appleLoggedIn
                        header: AccountManager.appleAvatar
                        name: AccountManager.appleNickname
                        loginText: "登录"
                        idleText: "未登录"
                        onClicked: {
                            if (AccountManager.appleLoggedIn) MusicApi.songSource = 5
                            else page.platformLoginRequested("Apple Music")
                        }
                        onLogined: {
                            if (AccountManager.appleLoggedIn)
                                Options.dialog.openSimpleDialog("警告", "是否退出账号？", function() { AccountManager.logoutApple() })
                            else
                                page.platformLoginRequested("Apple Music")
                        }
                    }
                }
            }
        }

        QHead { text: "应用程序" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                width: parent.width
                padding: 0
                Component.onCompleted: parent.height = height

                SettingItemCard {
                    label: "打开欢迎界面"
                    tip: "重新打开用于第一次打开应用快速设置的指引界面"
                    QButton {
                        shadowEnabled: false
                        radius: Style.labelRadius
                        borderWidth: 2
                        text: "打开"
                        fontSize: Style.text
                        onClicked: {
                            Config.isFirst = true;
                            Options.warned("将在下一次启动软件显示欢迎界面", 1);
                        }
                    }
                }

                SettingItemCard {
                    label: "全局音量"
                    tip: "设置本应用音乐播放单的音量"
                    QSlider {
                        to: 100
                        stepSize: 1
                        leftText: true
                        valueText: value
                        value: Math.floor(Playback.volume * 100)
                        onMoved: {
                            Playback.setVolume(value / 100)
                        }
                    }
                }

                SettingItemCard {
                    label: "下载目录"
                    tip: "设置音乐文件默认下载的目录"
                    QButton {
                        shadowEnabled: false
                        radius: Style.labelRadius
                        borderWidth: 2
                        text: Config.downloadFolder ? "自定义目录" : "系统音乐文件夹"
                        fontSize: Style.text
                        onClicked: downloadFolderDialog.open()
                    }

                    FolderDialog {
                        id: downloadFolderDialog
                        title: "选择默认下载目录"
                        currentFolder: Config.downloadFolder || StandardPaths.writableLocation(StandardPaths.MusicLocation)
                        onAccepted: {
                            let p = downloadFolderDialog.selectedFolder.toString();
                            if (p.indexOf("file:///") === 0)
                                p = p.substring(8);
                            Config.downloadFolder = p;
                            Options.warned("已设置默认下载目录", 1);
                        }
                    }
                }

                SettingItemCard {
                    label: "最小化托盘"
                    tip: "开启后点击关闭按钮将隐藏窗口（任务栏不再显示），可从托盘图标恢复或退出"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.closeToManage
                        onToggled: Config.closeToManage = !Config.closeToManage
                    }
                }

                SettingItemCard {
                    label: "记住窗口"
                    tip: "下次启动将恢复原来的窗口位置和大小"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.rememberWindow
                        onToggled: Config.rememberWindow = !Config.rememberWindow
                    }
                }

                SettingItemCard {
                    label: "自动检查更新"
                    tip: "在每次启动应用程序时将检查是否有新版本"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.autoUpdate
                        onToggled: Config.autoUpdate = !Config.autoUpdate
                    }
                }

                SettingItemCard {
                    label: "默认缓存位置"
                    tip: "设置封面等图片缓存文件的存放目录"
                    QButton {
                        shadowEnabled: false
                        radius: Style.labelRadius
                        borderWidth: 2
                        text: Config.cacheUrl ? "自定义目录" : "系统默认"
                        fontSize: Style.text
                        onClicked: cacheFolderDialog.open()
                    }
                    FolderDialog {
                        id: cacheFolderDialog
                        title: "选择缓存目录"
                        onAccepted: {
                            let p = cacheFolderDialog.selectedFolder.toString();
                            if (p.indexOf("file:///") === 0)
                                p = p.substring(8);
                            Config.cacheUrl = p;
                            Options.coverHelper.setCacheDir(p);
                            Options.warned("已设置封面缓存目录", 1);
                        }
                    }
                }

                SettingItemCard {
                    label: "自动缓存图片"
                    tip: "加载的图片将自动缓存到内存中，可以提高性能，但会占用更多内存"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.picCache
                        onToggled: Config.picCache = !Config.picCache
                    }
                }

                SettingItemCard {
                    label: "清除图片缓存"
                    tip: "清除缓存到磁盘的图片文件"
                    QButton {
                        shadowEnabled: false
                        buttonColor: "transparent"
                        radius: Style.labelRadius
                        borderWidth: 2
                        text: "清理"
                        onClicked: {
                            Options.coverHelper.clearCache();
                            // 清缓存时 DB 引用已作废，重载列表以丢弃界面上的死链路径
                            Songs.reload();
                            Options.warned("已清除图片缓存", 1);
                        }
                    }
                    bottomLine: false
                }
            }
        }
    }
}
