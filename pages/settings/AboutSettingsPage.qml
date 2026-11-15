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
    property color cardColor: Theme.primaryColor
    property Item blurSource: null

    y: 60
    width: parent ? parent.width : 0
    height: parent ? parent.height - 60 : 0
    visible: false

    QtObject {
        id: updater

        // 远程 version.txt 的 URL
        property string remoteVersionUrl: "https://raw.githubusercontent.com/BroNekoX/QueMusic/main/doc/updater.txt"
        property int newVersion: Options.versionCode
        property string versionDescription: ""

        function checkForUpdate(): void {
            console.log("正在检查更新...");
            Options.warned("正在检查更新", 0);

            const xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState === XMLHttpRequest.DONE) {
                    if (xhr.status === 200) {
                        const text = xhr.responseText.trim();
                        const remoteVersion = parseInt(text.substring(0,3));
                        console.log("远程版本号:", remoteVersion);

                        if (remoteVersion > Options.versionCode) {
                            console.log("发现新版本!");
                            newVersion = remoteVersion;
                            updater.versionDescription = text.substring(4);
                            // 显示更新提示对话框
                            updater.updateDialog.open();
                        } else {
                            console.log("当前已是最新版本");
                            // 可选：显示“已是最新”的提示
                            Options.warned("当前已是最新版本", 1);
                        }
                    } else {
                        console.error("检查更新失败，HTTP状态码:", xhr.status);
                        Options.warned("检查更新失败，请稍后重试", 2);
                    }
                }
            }
            xhr.open("GET", remoteVersionUrl)
            xhr.send();
        }

        // 更新提示对话框
        property QAlertDialog updateDialog: QAlertDialog {
            title: "有新版本！"
            message: "发现QueMusic的新版本：(v" + updater.newVersion + ")，是否前往更新？\n 版本更新日志：" + updater.versionDescription
            isInput: false
            blurSource: page.blurSource
            confirmText: "更新"

            onConfirm: {
                Qt.openUrlExternally("https://github.com/BroNekoX/QueMusic/releases");
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
            text: "关于应用"
            font.pixelSize: Style.pageTitle
            font.weight: Font.DemiBold
        }

        // 头标
        Column {
            width: page.standWidth
            spacing: 32
            topPadding: 64
            bottomPadding: 64
            //icon
            Image {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 96
                height: 96
                source: "qrc:/QueMusic/resources/icon.png"
                sourceSize: Qt.size(96, 96)
            }

            // App Title
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                height: 56
                width: implicitWidth// + 96
                text: "QueMusic"
                font.pixelSize: 56
                font.bold: false
                font.family: Fonts.text
                verticalAlignment: Text.AlignVCenter
                color: Theme.fontColor
            }

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "一款基于 C++/Qt Quick 框架开发的高性能音乐播放器"
                wrapMode: Text.Wrap
                color: Theme.textColor
                font.bold: false
                font.pixelSize: Style.textH2
            }
        }

        Rectangle {
            width: page.standWidth
            height: 60
            color: page.cardColor
            radius: Style.cubeRadius
            SettingItem {
                width: page.standWidth - 32
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
                        text: Application.version + " (" + Options.versionCode + ")"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                    QButton {
                        height: 36
                        width: 100
                        text: "检查更新"
                        buttonColor: Theme.themeColor
                        textColor: Theme.primaryColor
                        iconColor: Theme.primaryColor
                        shadowEnabled: false
                        onClicked: {
                            updater.checkForUpdate();
                        }
                    }
                }
            }
        }

        Rectangle {
            width: page.standWidth
            height: warnText.implicitHeight + 40
            color: Theme.containColor
            radius: Style.cubeRadius
            border.color: Theme.sideColor
            border.width: 1
            Text {
                x: 20
                anchors.verticalCenter: parent.verticalCenter
                font.family: Fonts.icon
                height: warnText.implicitHeight
                text: "\uf11a"
                color: Theme.themeColor
                font.pixelSize: Style.texticon
            }
            Text {
                id: warnText
                x: 48
                y: 20
                width: parent.width - 108
                text: "该版本属于开发中Beta版本，是未正式发布的开发中测试版本，部分功能仍未有效，并且稳定性欠佳，非最终质量"
                wrapMode: Text.Wrap
                color: Theme.fontColor
                font.bold: false
                font.pixelSize: Style.textmain
            }
            SButton {
                iconCharacter: "\uf025"
                x: parent.width - 52
                anchors.verticalCenter: parent.verticalCenter
                width: 36
                height: 36
                radius: 18
                iconSize: Style.texticon
                buttonColor: "transparent"
                shadowEnabled: false
                onClicked: {
                    parent.visible = false;
                }
            }
        }

        Rectangle {
            width: page.standWidth
            height: warnMoneyText.implicitHeight + 40
            color: Theme.containColor
            radius: Style.cubeRadius
            border.color: Theme.sideColor
            border.width: 1
            Text {
                x: 20
                anchors.verticalCenter: parent.verticalCenter
                font.family: Fonts.icon
                height: warnMoneyText.implicitHeight
                text: "\uf11a"
                color: Theme.themeColor
                font.pixelSize: Style.texticon
            }
            Text {
                id: warnMoneyText
                x: 48
                y: 20
                width: parent.width - 108
                text: "QueMusic Beta（官方版）始终是完全免费且开源的软件，不存在付费，会员，捐献，充值，广告等入口，官方版本不存在Pro，Ultra，高级版等版本，如果你发现软件或软件内有需要付费的内容，请立即与开发者联系。"
                wrapMode: Text.Wrap
                color: Theme.fontColor
                font.bold: false
                font.pixelSize: Style.textmain
            }
            SButton {
                iconCharacter: "\uf025"
                x: parent.width - 52
                anchors.verticalCenter: parent.verticalCenter
                width: 36
                height: 36
                radius: 18
                iconSize: Style.texticon
                buttonColor: "transparent"
                shadowEnabled: false
                onClicked: {
                    parent.visible = false;
                }
            }
        }

        QHead { text: "软件信息" }

        Rectangle {
            width: page.standWidth
            height: description.implicitHeight + 48
            color: page.cardColor
            radius: Style.cubeRadius
            Text {
                id: description
                anchors.centerIn: parent
                width: page.standWidth - 48
                text: "QueMusic是一个基于Qt QML开发的全能音乐播放器，旨在让听歌变得更简单，在Qt RHI * QML * C++强大组合下，性能卓越，UI美观丝滑，基于C++的在线音源使其拥有强大的稳定在线体验，并且由此实现美观强大的自定义功能，QueMusic希望做一个全能的音乐播放器。"
                wrapMode: Text.Wrap
                color: Theme.textColor
                font.pixelSize: 13
            }
        }

        QHead { text: "技术践与版本信息" }

        Rectangle {
            width: page.standWidth
            height: 238
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                spacing: 8
                padding: 16
                SettingItem {
                    width: page.standWidth - 32
                    label: "软件版本"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: Application.version + " (" + Options.versionCode + ") (Beta)"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    width: page.standWidth - 32
                    label: "QuePlugin API版本"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: Options.pluginApi + " (Beta)"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    width: page.standWidth - 32
                    label: "Qt框架版本"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: "Qt-" + "6.10.3" + "(Community)"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    width: page.standWidth - 32
                    label: "渲染技术"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: "QRHI/Qt Scene Graph"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    width: page.standWidth - 32
                    label: "主体技术"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: "C++/JS/QtQuick"
                        color: Theme.textColor
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
            width: page.standWidth
            AccountCard {
                width: page.standWidth / 2 - 12
                source: "qrc:/QueMusic/resources/app/icons/qwk.png"
                title: "QWindowKit"
                text: "实现全平台完美的无边框窗口"
                openUrl: "https://github.com/stdware/qwindowkit"
                baseColor: page.cardColor
            }
            AccountCard {
                width: page.standWidth / 2 - 12
                source: "qrc:/QueMusic/resources/app/icons/qticon.png"
                title: "Qt Community"
                text: "强大的开源跨平台软件包框架"
                openUrl: "https://github.com/qt"
                baseColor: page.cardColor
            }
            AccountCard {
                width: page.standWidth / 2 - 12
                source: ""
                title: "QCloudMusicApi"
                text: "网易云音乐第三方API服务框架"
                openUrl: "https://github.com/s12mmm3/QCloudMusicApi"
                baseColor: page.cardColor
            }
        }
        Rectangle {
            width: page.standWidth
            height: fontDescript.implicitHeight + 48
            color: page.cardColor
            radius: Style.cubeRadius
            Text {
                id: fontDescript
                anchors.centerIn: parent
                width: page.standWidth - 48
                text: "QueMusic使用Feather字体库用于显示图标以及Figtree字体库用于显示部分英文。还有更多被QueMusic使用的第三方库，开发者表示感谢，详细查看项目Readme"
                wrapMode: Text.Wrap
                color: Theme.textColor
                font.pixelSize: 13
            }
        }

        QHead { text: "联系开发者 —— BroNekoX" }

        Rectangle {
            width: page.standWidth
            height: 200
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                spacing: 8
                padding: 16
                SettingItem {
                    width: page.standWidth - 32
                    label: "QQ"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: "241422517"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    width: page.standWidth - 32
                    label: "邮箱/Email"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: "uihugd@outlook.com"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    width: page.standWidth - 32
                    label: "Bilibili"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: "695207057"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
                SettingItem {
                    width: page.standWidth - 32
                    label: "QQ群组"
                    controlWidth: 120
                    Text {
                        anchors.right: parent.right
                        height: 36
                        text: "1105114511"
                        color: Theme.textColor
                        font.pixelSize: 14
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        Text { text: "期待您的贡献与反馈"; color: Theme.fontColor; font.pixelSize: Style.textmain }

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
                    Options.textWatch.active = true;
                }
            }
        }
        Item {
            width: parent.width
            height: 36
            Text {
                anchors.centerIn: parent
                color: Theme.fontColor
                font.pixelSize: Style.textmain
                text: "Copyright (c) 2025-2026 QueMusic Contributors\n   本项目基于 Apache License 2.0 协议获得许可"
            }
        }
    }
}
