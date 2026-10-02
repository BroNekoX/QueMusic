import QtQuick
import QueMusic 1.0

Rectangle {
    id: root
    width: 256
    height: 128
    radius: Style.settings.labelRadius
    border.width: 2
    property color chooseColor: Style.themeColor
    property color chooseColor1: Style.containColor
    property bool choose: false
    property bool isLogin: false
    property bool showLogin: true
    property string name: ""
    property string header: ""
    property string idleText: "未绑定账户"
    property string text: "Music"
    color: choose ? chooseColor1 : Style.secondaryColor
    border.color: choose ? chooseColor : Style.sideColor
    signal clicked()
    signal logined()
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: root.radius
        color: Style.hoverColor
        opacity: area.containsMouse ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 120 } }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
        QButton {
            x: root.width - width - 16
            y: 80
            visible: root.showLogin
            text: root.isLogin ? "退出登录" : "扫码登录"
            height: 32
            radius: 16
            shadowEnabled: false
            buttonColor: root.chooseColor
            textColor: Style.secondaryColor
            onClicked: root.logined();
        }
    }

    Rectangle {
        x: 16
        y: 24
        width: 12
        height: 12
        radius: 6
        color: root.chooseColor
    }
    Text {
        x: 36
        y: 20
        height: 20
        text: root.text
        color: Style.fontColor
        font.pixelSize: Style.settings.textmain
        font.bold: true
        verticalAlignment: Text.AlignVCenter
    }
    Row {
        x: 16
        y: 80
        height: 32
        spacing: 8
        QPicture {
            height: 32
            width: 32
            visible: root.isLogin && root.header !== ""
            source: root.header
        }
        Text {
            height: 32
            text: root.isLogin ? root.name : root.idleText
            color: root.isLogin ? Style.fontColor : Style.textColor
            font.pixelSize: Style.settings.textmain
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }
    Rectangle {
        x: root.width - width - 16
        y: 18
        height: 24
        width: 72
        radius: 12
        color: root.choose ? root.chooseColor : Style.secondaryColor
        border.color: root.chooseColor1
        Text {
            anchors.centerIn: parent
            text: root.choose ? "当前主平台" : "选择此平台"
            font.bold: true
            font.pixelSize: Style.settings.textTip
            color: root.choose ? Style.secondaryColor : root.chooseColor
        }
    }
}