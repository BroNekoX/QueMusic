import QtQuick
ListView {
    id: view
    property string t: "x"
    width: 100
    height: 100
    model: 2
    delegate: Item { width: 10; height: 10; Text { text: view.t } }
}
