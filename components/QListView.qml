// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 在线音乐列表：歌曲与歌单共用（isList 区分）。
// 收藏 / 加入播放列表 / 下一首 / 下载 / 信息弹窗在此内部完成，页面只接 onClicked 与 onEnded。
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QueMusic 1.0
import 'qrc:/QueMusic/components'

ListView {
    id: view
    topMargin: 6
    bottomMargin: 24
    rightMargin: 16
    acceptedButtons: Qt.NoButton
    property int scrollToY: view.contentY
    property list<string> headerModel: isList ? ["标题","创建者","曲目","操作"] : ["标题","歌手","时长","操作"]
    property list<string> menuModel: isList ? ["收藏","歌单信息"] : ["下载到本地","下一首播放","添加到列表","收藏","歌曲信息"]
    property list<int> selectedIndices: []
    readonly property var selectedSet: new Set(selectedIndices)
    property bool isList: false
    property int artistX: width / 2 - 32
    property int toolX: width - 210
    property string toolText0: "\uf095"  // 行尾按钮：加入播放列表 / 歌单信息
    property string toolText1: "\uf0c8"  // 行尾按钮：收藏（未收藏时为空心）
    property alias menu: menu
    property bool isEnd: false
    property var toolHandler: null   // 返回 true 表示页面自行处理，跳过内置动作
    property var menuHandler: null
    readonly property string favType: isList ? "playlist" : "song"
    readonly property var favModel: isList ? FavoritePlaylists : FavoriteSongs
    readonly property var localDriveRe: /^[a-zA-Z]:[\\/]/
    readonly property var sourceNames: ["酷狗音乐", "网易云音乐", "哔哩哔哩", "QQ音乐", "自定义源"]
    readonly property var qualityNames: ["标准", "无损", "Hi-Res"]
    contentWidth: view.width - 16
    synchronousDrag: true
    reuseItems: true
    onDraggingChanged: view.scrollToY = view.contentY

    // 回到顶部并同步 scrollToY，防止滚轮动画把 contentY 拉回过期位置
    function scrollTop(): void {
        listViewAnime.stop();
        scrollToY = view.originY - view.topMargin;
        contentY = view.originY - view.topMargin;
    }
    signal clicked(int index)
    signal ended()

    onAtYEndChanged: {
        if (atYEnd && !MusicApi.loadState && count !== 0) ended();
    }

    // 行取值：在线模型用 hash/title/artist，收藏模型用 id/favId/…
    function itemAt(i: int): var { return view.model && i >= 0 ? view.model.get(i) : null }
    function keyOf(it: var): string {
        return it ? (it.hash || it.favId || it.path || it.fileUrl || it.id || "") : ""
    }
    function titleOf(it: var): string { return it ? (it.title || it.name || "") : "" }
    function artistOf(it: var): string { return it ? (it.artist || it.singer || "") : "" }
    function coverOf(it: var): string {
        const s = it && it.cover ? String(it.cover).replace("{size}", "256") : ""
        return s !== "" ? s : "qrc:/QueMusic/resources/app/musicpic.png"
    }
    // 按当前音质取 hash，高档缺档时降级
    function hashOf(it: var): string {
        if (!it) return ""
        const q = Options.settings.soundQuality
        const h = q === 2 ? it.hashsq : q === 1 ? it.hashhq : it.hash
        return h || it.hash || it.favId || it.id || ""
    }
    // 源：模型未给出时按 key 推断，本地路径记为 -1
    function sourceOf(it: var): var {
        if (it && it.source !== undefined) return it.source
        const k = keyOf(it)
        return (k.indexOf("file:") === 0 || localDriveRe.test(k)) ? -1 : MusicApi.songSource
    }
    function favoriteIcon(i: int): string {
        const it = itemAt(i)
        return it && favModel.isFavorite(keyOf(it), favType) ? "\uf09f" : view.toolText1
    }

    function toggleFavorite(i: int): void {
        const it = itemAt(i)
        const key = keyOf(it)
        if (!key) return
        if (favModel.isFavorite(key, favType)) {
            favModel.removeFavorite(key, favType)
            Style.warned("已取消收藏", 0)
        } else {
            favModel.addFavorite(key, titleOf(it), artistOf(it), coverOf(it), sourceOf(it),
                                 it.duration || 0, favType)
            Style.warned("已收藏", 1)
        }
    }
    function enqueue(i: int, next: bool): void {
        const it = itemAt(i)
        const key = keyOf(it)
        if (!key) return
        const row = { name: titleOf(it), path: key, songer: artistOf(it), source: sourceOf(it) }
        if (next) {
            Playback.playNext(row)
            Style.warned("已设为下一首播放", 1)
        } else {
            Style.warned(Playback.enqueue(row) ? "已加入播放列表" : "已在播放列表中", 1)
        }
    }
    function download(i: int): void {
        const it = itemAt(i)
        const h = hashOf(it)
        if (!h) return
        MusicApi.getMusicInfo(h, 1, sourceOf(it))
        Style.warned("已开始下载", 1)
    }
    function showInfo(i: int): void {
        const it = itemAt(i)
        if (!it) return
        infoDialog.info = it
        infoDialog.open()
    }

    // 菜单索引：歌单 0 收藏 / 1 信息；歌曲 0 下载 / 1 下一首 / 2 加入列表 / 3 收藏 / 4 信息
    function runMenuAction(choice: int): void {
        const i = menu.index
        if (menuHandler && menuHandler(choice, i) === true) return
        if (view.isList) {
            if (choice === 0) toggleFavorite(i)
            else showInfo(i)
        } else if (choice === 0) download(i)
        else if (choice === 1) enqueue(i, true)
        else if (choice === 2) enqueue(i, false)
        else if (choice === 3) toggleFavorite(i)
        else showInfo(i)
    }
    function runToolAction(i: int, tool: int): void {
        if (toolHandler && toolHandler(tool, i) === true) return
        if (tool === 1) toggleFavorite(i)
        else if (view.isList) showInfo(i)
        else enqueue(i, false)
    }

    footer: Item {
        width: view.width
        height: 32
        visible: view.isEnd
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            text: "没有更多了~"
            color: Style.themes.textColor
            font.pixelSize: Style.settings.text
        }
    }
    Menu {
        id: menu
        title: "Menu"
        parent: Overlay.overlay
        property int index

        background: Rectangle {
            implicitWidth: 160
            implicitHeight: 40
            color: Style.themes.primaryColor
            radius: Style.settings.labelRadius
            RectangularShadow {
                anchors.fill: parent
                z: -1
                offset.x: 0
                offset.y: 5
                radius: parent.radius
                blur: 20
                spread: 0
                color: Style.themes.shadowColor
            }
        }

        Instantiator {
            model: view.menuModel
            onObjectAdded: (i, obj) => menu.insertItem(i, obj)
            onObjectRemoved: (i, obj) => menu.removeItem(obj)
            delegate: MenuItem {
                id: menuItem
                background: Rectangle {
                    implicitWidth: 146
                    implicitHeight: 36
                    x: 2
                    y: 2
                    radius: Style.settings.labelRadius - 2
                    width: menuItem.width - 4
                    height: menuItem.height - 4
                    color: menuItem.down || menuItem.highlighted ? Style.themes.hoverColor : "transparent"
                }
                text: modelData
                contentItem: Text {
                    text: menuItem.text
                    color: Style.themes.fontColor
                    font.pixelSize: Style.settings.textmain
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 12
                    elide: Text.ElideRight
                }
                onTriggered: view.runMenuAction(index)
            }
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; duration: 160; from: 0; to: 1 }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; duration: 120; to: 0 }
        }
    }

    // 歌曲 / 歌单信息，打开时才构建内容
    QOptionDialog {
        id: infoDialog
        title: view.isList ? "歌单信息" : "歌曲信息"
        confirmText: "关闭"
        property var info: null

        function rows(): var {
            const it = info
            if (!it) return []
            const src = view.sourceNames[it.source] || "未知来源"
            if (view.isList)
                return [["名称", view.titleOf(it)], ["创建者", view.artistOf(it)],
                        ["曲目", (it.duration || 0) + " 首"], ["来源", src]]
            const sec = it.duration || 0
            return [["歌手", view.artistOf(it)], ["专辑", it.album || "—"],
                    ["时长", Math.floor(sec / 60) + ":" + ("0" + Math.floor(sec % 60)).slice(-2)],
                    ["音质", view.qualityNames[Options.settings.soundQuality] || "标准"], ["来源", src]]
        }

        Column {
            width: parent.width
            spacing: 14
            Row {
                width: parent.width
                spacing: 14
                QPicture {
                    width: 72
                    height: 72
                    radius: 14
                    source: view.coverOf(infoDialog.info)
                    sourceSize: Qt.size(160, 160)
                }
                Column {
                    width: parent.width - 86
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6
                    Text {
                        width: parent.width
                        text: view.titleOf(infoDialog.info)
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: Style.themes.fontColor
                        elide: Text.ElideRight
                    }
                    Text {
                        width: parent.width
                        text: view.artistOf(infoDialog.info)
                        font.pixelSize: 12
                        color: Style.themes.textColor
                        elide: Text.ElideRight
                    }
                }
            }
            Repeater {
                model: infoDialog.rows()
                delegate: Row {
                    required property var modelData
                    width: infoDialog.width - 40
                    height: 22
                    Text {
                        width: 72
                        height: 22
                        text: modelData[0]
                        font.pixelSize: 12
                        color: Style.themes.textColor
                        verticalAlignment: Text.AlignVCenter
                    }
                    Text {
                        width: parent.width - 72
                        height: 22
                        text: modelData[1]
                        font.pixelSize: 12
                        color: Style.themes.fontColor
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }
    }

    ScrollBar.vertical: ScrollBar {
        id: viewBar
        parent: view
        anchors.top: view.top
        anchors.right: view.right
        anchors.bottom: view.bottom
        onPressedChanged: {
            view.scrollToY = view.contentY
        }
    }
    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        readonly property real wheelHeightCount: Qt.application.styleHints.wheelScrollLines * 0.25
        readonly property int scrollBottom: view.contentHeight - view.height + view.bottomMargin + view.originY
        onWheel: (event) => {
            listViewAnime.running = false;
            view.scrollToY = Math.max(view.originY - view.topMargin, Math.min( view.scrollToY - (event.angleDelta.y * wheelHeightCount), scrollBottom));
            viewBar.active = true;
            event.accepted = true;
            listViewAnime.running = true;
        }
    }

    NumberAnimation {
        id: listViewAnime
        target: view
        property: "contentY"
        duration: 240
        to: view.scrollToY
        easing.type: Easing.OutCubic
        onFinished: viewBar.active = false
    }

    header: Item {
        width: view.width
        height: 36
        Text {
            x: 76
            height: 36
            text: view.headerModel[0]
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.artistX
            height: 36
            text: view.headerModel[1]
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.width - 76
            height: 36
            text: view.headerModel[2]
            color: Style.themes.textColor
            font.pixelSize: Style.settings.textTip
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            width: parent.width - 16
            height: 1
            color: Style.themes.sideColor
            opacity: 0.5
            y: 35
        }
    }

    displaced: Transition {
        id: listDisplacedAnime
        SequentialAnimation {
            PauseAnimation {
                duration: {
                    const vt = listDisplacedAnime.ViewTransition;
                    const ti = vt.targetIndexes;
                    const base = (ti && ti.length > 0) ? ti[0] : vt.index;
                    return Math.min(Math.abs(vt.index - base), 8) * 30;
                }
            }
            NumberAnimation {
                properties: "y"
                duration: 240
                easing.type: Easing.Bezier; easing.bezierCurve: [ 0.23, 0.06, 0.00, 1.00, 1, 1 ]
            }
        }
    }
    add: Transition {
        ParallelAnimation {
            NumberAnimation {
                properties: "transY"
                from: 60
                to: 0
                duration: 320
                easing.type: Easing.OutQuint
            }
            NumberAnimation {
                properties: "opacity"
                from: 0
                to: 1
                duration: 280
                easing.type: Easing.OutCubic
            }
        }
    }

    delegate: Rectangle {
        id: listDel
        height: 60
        width: view.width - 16
        color: view.selectedSet.has(index) ? Style.themes.containColor : "#00000000"
        radius: Style.settings.labelRadius
        property int transY: 0
        transform: Translate { y: listDel.transY }

        // reuseItems 下非 model 提供的属性不会随复用自动恢复，按官方建议手动复位
        ListView.onReused: { listDel.transY = 0; listDel.opacity = 1; }
        ListView.onPooled: { listDel.transY = 0; listDel.opacity = 1; }

        Rectangle {
            anchors.fill: parent
            radius: Style.settings.labelRadius
            color: Style.themes.hoverColor
            opacity: listArea.containsMouse ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        QPicture {
            y: 8
            x: 8
            width: 44
            height: 44
            radius: 10
            source: (model.cover || "").replace("{size}","64") || "qrc:/QueMusic/resources/app/musicpic.png"
        }

        Text {
            id: title
            x: 76
            y: 16
            width: view.artistX - 110
            height: 28
            text: model.title || "Unknown"
            color: Style.themes.fontColor
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            font.pixelSize: Style.settings.textmain
            verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            color: Style.themes.containColor
            x: title.implicitWidth > title.width ? title.width + 75 : title.implicitWidth + 85
            y: 20
            width: 32
            height: 18
            radius: 9
            visible: model.paytype === 3
            Text {
                text: "VIP"
                anchors.centerIn: parent
                color: Style.themes.themeColor
                font.pixelSize: 9
                font.weight: Font.DemiBold
            }
        }
        Text {
            x: view.artistX
            y: 16
            width: view.artistX - 128
            height: 28
            text: model.artist || "Unknown"
            color: Style.themes.textColor
            font.weight: Font.Normal
            elide: Text.ElideRight
            font.pixelSize: Style.settings.text
            verticalAlignment: Text.AlignVCenter
        }
        Text {
            x: view.width - 92
            y: 16
            width: 60
            height: 28
            text: view.isList ? model.duration + "首" : Math.floor(model.duration / 60) + ":" + (model.duration % 60)
            color: Style.themes.textColor
            font.bold: false
            elide: Text.ElideRight
            font.pixelSize: Style.settings.text
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }

        MouseArea {
            id: listArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: (mouse) => {
                if (mouse.button === Qt.LeftButton) {
                    view.clicked(index);
                } else {
                    menu.index = index;
                    view.menu.popup();
                }
                forceActiveFocus();
            }

            Row {
                x: view.toolX
                spacing: 2
                y: 12
                height: 36
                opacity: listArea.containsMouse ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 160 } }
                SButton {
                    iconCharacter: "\uf050"
                    width: 36
                    height: 36
                    radius: 36
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    shadowEnabled: false
                    tipText: "更多"
                    onClicked: {
                        menu.index = index
                        view.menu.popup()
                    }
                }
                SButton {
                    // 只在悬停时查收藏状态，避免滚动时逐行访问数据库
                    iconCharacter: listArea.containsMouse ? view.favoriteIcon(index) : view.toolText1
                    width: 36
                    height: 36
                    radius: 36
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    shadowEnabled: false
                    tipText: "收藏"
                    onClicked: view.toggleFavorite(index)
                }
                SButton {
                    iconCharacter: view.toolText0
                    width: 36
                    height: 36
                    radius: 36
                    buttonColor: "transparent"
                    hoverColor: Style.themes.hoverColor
                    shadowEnabled: false
                    tipText: view.isList ? "歌单信息" : "加入播放列表"
                    onClicked: view.runToolAction(index, 0)
                }
            }
        }
    }
}
