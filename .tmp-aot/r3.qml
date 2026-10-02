import QtQuick
ListView {
    id: view
    property string t: "x"
    width: 100
    height: 100
    function f(): string { return t }
}
