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
        readonly property QListView vw: ListView.view
        width: 100
        height: 20
        Text { text: row.vw.toolText0 }
    }
}
