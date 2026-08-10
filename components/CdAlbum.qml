import QtQuick
import QtQuick.Effects

Item {
    id: root
    property string source: "qrc:/QueMusic/resources/app/musicpic.png"
    property bool rotation: false
    NumberAnimation {
        target: cdImage
        property: "rotation"
        from: cdImage.rotation
        to: cdImage.rotation + 360
        duration: 16000
        running: true
        loops: Animation.Infinite
        paused: !root.rotation
    }

    RectangularShadow {
        anchors.fill: parent
        z: 0
        offset.x: 5
        offset.y: 15
        radius: root.width * 0.5
        blur: 24
        spread: 0
        color: "#66000000"
    }

    Image {
        source: "qrc:/QueMusic/resources/app/cd.png"
        z: 1
        width: root.width
        height: root.width
        QPicture {
            id: cdImage
            anchors.centerIn: parent
            rotation: 0
            width: root.width * 0.62
            height: root.width * 0.62
            radius: width * 0.5
            source: root.source
        }
    }

    Image {
        id: pointImage
        source: "qrc:/QueMusic/resources/app/cdpoint.png"
        z: 2
        x: root.width * 0.4
        y: root.width * -0.3
        width: root.width * 0.5
        height: root.width * 0.25
        transform: Rotation { origin.x: pointImage.height * 0.14; origin.y: pointImage.height * 0.14; angle: root.rotation ? 36 : 5; Behavior on angle { NumberAnimation { duration: 320; easing.type: Easing.InOutCubic } } }
    }
}