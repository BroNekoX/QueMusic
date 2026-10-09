import QtQuick
import QueMusic 1.0

Rectangle {
    id: root
    width: 256
    height: 128
    radius: Style.labelRadius
    border.width: 2
    property color chooseColor: Theme.themeColor
    property color chooseColor1: Theme.containColor
    property bool choose: false
    property bool isLogin: false
    property bool showLogin: true
    property string name: ""
    property string header: ""
    property string idleText: "未绑定账户"
    property string text: "Music"
    color: choose ? chooseColor1 : Theme.secondaryColor
    border.color: choose ? chooseColor : Theme.sideColor
    signal clicked()
    signal logined()
    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: root.radius
        color: Theme.hoverColor
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
            textColor: Theme.secondaryColor
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
        color: Theme.fontColor
        font.pixelSize: Style.textmain
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
            color: root.isLogin ? Theme.fontColor : Theme.textColor
            font.pixelSize: Style.textmain
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
        color: root.choose ? root.chooseColor : Theme.secondaryColor
        border.color: root.chooseColor1
        Text {
            anchors.centerIn: parent
            text: root.choose ? "当前主平台" : "选择此平台"
            font.bold: true
            font.pixelSize: Style.textTip
            color: root.choose ? Theme.secondaryColor : root.chooseColor
        }
    }
}