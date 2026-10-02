import QtQuick
import QueMusic 1.0

QListView {
    id: root
    width: 300
    height: 200
    model: 2
    delegate: Item {
        id: row
        required property int index
        width: 100
        height: 20
        Text { text: ListView.view.toolText0 }
    }
}
