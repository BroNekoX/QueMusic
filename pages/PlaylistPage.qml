// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Effects
import QueMusic 1.0

Item {
    id: playlistPage
    property real toolsWindow: 0

    readonly property bool inSongWindow: playListSongsWindow.visible
    readonly property int tabIndex: playlistChildPage.lastIndex
    readonly property bool songListActive: inSongWindow || tabIndex === 0

    readonly property var filterItems: ["全部", "仅免费", "仅 VIP（付费）"]

    function reloadCurrent(): void {
        if (inSongWindow) {
            MusicApi.playlistSong.clear();
            playListSongsWindow.reload(1);
        } else {
            switch (tabIndex) {
            case 0:
                MusicApi.newSongs.clear();
                MusicApi.getNewSongs(musicsPage.musicMenuIndex + 1, 1, 20);
                break;
            case 1:
                MusicApi.musicPlaylists.clear();
                MusicApi.getCategoryPlaylists(playlistPage.tagIdAt(musicMenuPage.musicMenuIndex), 1, 20);
                break;
            case 2:
                MusicApi.toplistList.clear();
                MusicApi.getAllToplists();
                break;
            default:
                album.loadSingers(album.singerTypeIndex);
                break;
            }
        }
    }

    function refreshView(): void {
        reloadCurrent();
        mainWarn.tiped("已刷新", 1);
    }

    // 分类菜单是 OnlineListModel，只能按下标 get()，不能当数组用
    function tagIdAt(index: int): string {
        const menu = MusicApi.allPlaylistMenu;
        if (index < 0 || index >= menu.count)
            return menu.count > 0 ? menu.get(0).id : "";
        return menu.get(index).id;
    }

    // 筛选：改付费类型过滤
    function applyFilter(index: int): void {
        if (!songListActive)
            return;
        MusicApi.songFilter = index;
        reloadCurrent();
        mainWarn.tiped("已筛选： " + filterItems[index], 1);
    }

    QMenu {
        id: filterMenu
        model: playlistPage.filterItems
        current: MusicApi.songFilter
        onClicked: (i) => playlistPage.applyFilter(i)
    }

    Component.onCompleted: {
        if(!window.completedStart.playlistLoaded) {
            MusicApi.getPlaylistMenu(3);
            MusicApi.getNewSongs(1, 1, 20);
            MusicApi.getAllToplist();
            window.completedStart.playlistLoaded = true;
        }
    }

    QPages {
        id: playlistChildPage
        x: 24
        y: 68
        width: parent.width - 48
        height: parent.height - 68
        pageList: [musicsPage,musicMenuPage,cloud,album]

        // 顶部标题
        Item {
            x: 0
            y: -44
            height: 40
            width: parent.width
            z: 10
            Text {
                x: 0
                y: 0
                height: 40
                verticalAlignment: Text.AlignVCenter
                text: "分类"
                font.weight: Font.DemiBold
                font.pixelSize: Style.settings.pageTitle
                color: Style.fontColor
            }
            QDrop {
                x: parent.width - 120
                y: 0
                height: 36; width: 120
                anchors.right: parent.right
                choice: MusicApi.songSource
                textColor: MusicApi.songSource == 0 ? "#0F3975" : MusicApi.songSource == 1 ? "#750F0F" : MusicApi.songSource == 2 ? "#7A1C3C" : MusicApi.songSource == 3 ? "#16750F" : "#756F0F"
                color: MusicApi.songSource == 0 ? "#CDE8FF" : MusicApi.songSource == 1 ? "#FFCDCD" : MusicApi.songSource == 2 ? "#FFD9E6" : MusicApi.songSource == 3 ? "#CDFFCD" : "#FFFFCD"
                border.color: MusicApi.songSource == 0 ? "#4384F5" : MusicApi.songSource == 1 ? "#F54343" : MusicApi.songSource == 2 ? "#FB7299" : MusicApi.songSource == 3 ? "#4DF543" : "#F5F543"
                radius: 18
                cardRadius: Style.settings.labelRadius
                model: ["酷狗音乐","网易云音乐","哔哩哔哩","QQ音乐(x)","自定义源(x)"]
                onTransformed: (choiced) => {
                    MusicApi.songSource = choiced;
                    MusicApi.newSongs.clear();
                    MusicApi.getPlaylistMenu(3);
                    MusicApi.getNewSongs(1, 1, 20);
                    MusicApi.getAllToplist();
                    toplistFlick.scrollTop();
                }
            }
        }

        QBlurTapBar {
            x: 0
            y: 12
            z: 12
            model: ["歌曲","歌单","排行榜","歌手"]
            tabWidth: 80
            width: 326
            rectXy: Qt.rect(0, 12, width, 40)
            blurSource: playlistChildPage.pageList[playlistChildPage.lastIndex]
            onTabChange: (index) => {
                playlistChildPage.stack(index)
                switch(index) {
                case 0:
                    break;
                case 1:
                    MusicApi.getCategoryPlaylists(MusicApi.allPlaylistMenu[0].id, 1, 20)
                    break;
                case 2:
                    break;
                case 3:
                    break;
                }
            }
        }

        // 右侧操作区
        Row {
            x: parent.width - width
            y: 13
            z: 10
            spacing: 8
            // 刷新：重新拉取当前页签 / 当前歌曲列表
            QButton {
                id: refreshBtn
                height: 38
                text: ""
                iconCharacter: "\uf11e"
                buttonColor: Style.primaryColor
                tipText: "重新获取当前列表"
                onClicked: playlistPage.refreshView()
            }
            // 筛选：按付费类型（全部 / 仅免费 / 仅 VIP），只对歌曲列表有效
            QButton {
                id: filterBtn
                height: 38
                text: "筛选"
                iconCharacter: "\uf101"
                enabled: playlistPage.songListActive
                opacity: enabled ? 1 : 0.45
                buttonColor: Style.primaryColor
                tipText: "筛选免费 / VIP 歌曲"
                onClicked: filterMenu.popup(filterBtn, 0, filterBtn.height + 6)
            }
        }

        Item {
            id: musicsPage
            width: playlistChildPage.width
            height: playlistChildPage.height
            property int musicMenuIndex: 0
            Row {
                spacing: 6
                y: 72
                Repeater {
                    model: ["华语","欧美","日韩","韩语","日语"]
                    delegate: Rectangle {
                        width: 64
                        height: 32
                        radius: 16
                        color: musicsPage.musicMenuIndex === index ? Style.themeColor : Style.primaryColor
                        border.color: Style.sideColor
                        border.width: 1
                        Rectangle {
                            anchors.fill: parent
                            radius: 16
                            color: Style.hoverColor
                            opacity: musicsMenuArea.containsMouse ? 1 : 0
                            z: 1
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                        }

                        Text {
                            anchors.fill: parent
                            text: modelData
                            elide: Text.ElideRight
                            z: 2
                            font.pixelSize: Style.settings.text
                            color: musicsPage.musicMenuIndex === index ? Style.fullColor : Style.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        MouseArea {
                            id: musicsMenuArea
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                MusicApi.newSongs.clear();
                                MusicApi.globalid = index + 1;
                                musicsPage.musicMenuIndex = index;
                                MusicApi.getNewSongs(index + 1, 1, 20);
                            }
                        }
                    }
                }
            }
            QListView {
                id: searchSong
                width: parent.width + 16
                y: 104
                height: parent.height - 104
                model: MusicApi.newSongs
                clip: true

                onEnded: {
                    if(MusicApi.newSongs.count % 20 === 0 && MusicApi.newSongs.count !== 0) {
                        MusicApi.getNewSongs(MusicApi.globalid, MusicApi.newSongs.count / 20 + 1, 20);
                        isEnd = false;
                    } else {
                        if(MusicApi.newSongs.count !== 0) {
                            isEnd = true;
                        }
                    }
                }
                onClicked: (index) => {
                    if(Options.settings.soundQuality === 0) {
                        MusicApi.getMusicInfo(model.get(index).hash);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(model.get(index).hashhq);
                    } else {
                        MusicApi.getMusicInfo(model.get(index).hashsq);
                    }
                }
            }
        }
        Item {
            id: musicMenuPage
            visible: false
            width: playlistChildPage.width
            height: playlistChildPage.height
            property int musicMenuIndex: 0
            Flow {
                id: musicMenuFlow
                spacing: 6
                y: 72
                width: parent.width
                Repeater {
                    model: MusicApi.allPlaylistMenu
                    delegate: Rectangle {
                        id: playlistMenuRow
                        required property string title
                        required property int index
                        width: 64
                        height: 32
                        radius: 16
                        color: musicMenuPage.musicMenuIndex === playlistMenuRow.index ? Style.themeColor : Style.primaryColor
                        border.color: Style.sideColor
                        border.width: 1
                        Rectangle {
                            anchors.fill: parent
                            radius: 16
                            color: Style.hoverColor
                            opacity: musiclistMenuArea.containsMouse ? 1 : 0
                            z: 1
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                        }

                        Text {
                            anchors.fill: parent
                            text: playlistMenuRow.title
                            elide: Text.ElideRight
                            z: 2
                            font.pixelSize: Style.settings.text
                            color: musicMenuPage.musicMenuIndex === playlistMenuRow.index ? Style.fullColor : Style.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        MouseArea {
                            id: musiclistMenuArea
                            hoverEnabled: true
                            anchors.fill: parent
                            onClicked: {
                                musicMenuPage.musicMenuIndex = playlistMenuRow.index;
                                MusicApi.globaltagid = MusicApi.allPlaylistMenu[playlistMenuRow.index].id;
                                MusicApi.musicPlaylists.clear();
                                MusicApi.getCategoryPlaylists(MusicApi.globaltagid, 1, 20);
                            }
                        }
                    }
                }
            }

            QListView {
                id: musicMenuList
                height: parent.height - y
                clip: true
                y: musicMenuFlow.implicitHeight + 80
                width: parent.width + 16
                model: MusicApi.musicPlaylists
                property int artistX: width / 2 - 50
                bottomMargin: 24
                isList: true

                onEnded: {
                    if(MusicApi.musicPlaylists.count % 20 === 0 && MusicApi.musicPlaylists.count !== 0) {
                        MusicApi.getCategoryPlaylists(MusicApi.globaltagid, MusicApi.musicPlaylists.count / 20 + 1, 20);
                        isEnd = false;
                    } else {
                        if(MusicApi.musicPlaylists.count !== 0) {
                            isEnd = true;
                        }
                    }
                }

                onClicked: (index) => {
                    MusicApi.playlistSong.clear();
                    MusicApi.globalid = model.get(index).hash;
                    MusicApi.getPlaylistSongs(model.get(index).hash,1,20);
                    //var image = model.get(index).cover.replace("{size}", "256") || "qrc:/QueMusic/resources/app/musicpic.png";
                    //var title = model.get(index).title;
                    playListSongsWindow.opened(model.get(index));
                    window.exitIndex = 2;
                }
            }
        }
        Item {
            id: cloud
            visible: false
            width: playlistChildPage.width
            height: playlistChildPage.height
            Component.onCompleted: {
                if(MusicApi.toplistList.count === 0)
                    MusicApi.getAllToplist();
            }

            QGridView {
                id: toplistFlick
                width: parent.width + 16
                height: parent.height
                model: MusicApi.toplistList
                cellWidth: 180
                cellHeight: 240
                rightMargin: -8
                topMargin: 72
                delegate: Rectangle {
                    id: toplistCard
                    required property string cover
                    required property int source
                    required property string title
                    required property string artist
                    required property string hash
                    width: 156
                    height: 216
                    radius: Style.settings.labelRadius
                    color: Style.primaryColor
                    scale: toplistCardArea.containsMouse ? 1.04 : 1.0
                    Behavior on scale { NumberAnimation { duration: 200; easing.type: Easing.OutExpo } }
                    RectangularShadow {
                        anchors.fill: parent
                        z: -1
                        offset.x: 2
                        offset.y: 2
                        radius: Style.settings.labelRadius
                        blur: toplistCardArea.containsMouse ? 24 : 8
                        spread: 0
                        color: Style.shadowColor
                        Behavior on blur { NumberAnimation { duration: 200 } }
                    }
                    QPicture {
                        width: 156
                        height: 156
                        source: (toplistCard.cover || "").replace("{size}","128") || "qrc:/QueMusic/resources/app/musicpic.png"
                        radius: Style.settings.labelRadius
                        radius3: 0
                        radius4: 0
                        sourceSize: Qt.size(128,128)
                    }
                    // 平台徽标
                    Rectangle {
                        x: 8
                        y: 8
                        width: 50
                        height: 20
                        radius: 10
                        color: toplistCard.source === 0 ? "#CDE8FF" : toplistCard.source === 1 ? "#FFCDCD" : "#FFD9E6"
                        Text {
                            anchors.centerIn: parent
                            text: toplistCard.source === 0 ? "酷狗" : toplistCard.source === 1 ? "网易云" : "B站"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            color: toplistCard.source === 0 ? "#0F3975" : toplistCard.source === 1 ? "#750F0F" : "#7A1C3C"
                        }
                    }
                    Text {
                        x: 12
                        y: 166
                        width: 132
                        text: toplistCard.title
                        font.bold: true
                        color: Style.fontColor
                        font.pixelSize: Style.settings.textmain
                        elide: Text.ElideRight
                    }
                    Text {
                        x: 12
                        y: 190
                        width: 132
                        text: toplistCard.artist || ""
                        color: Style.textColor
                        font.pixelSize: Style.settings.text
                        elide: Text.ElideRight
                    }
                    MouseArea {
                        id: toplistCardArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            MusicApi.playlistSong.clear();
                            MusicApi.globalid = toplistCard.hash;
                            playListSongsWindow.listType = "toplist";
                            MusicApi.getMusicToplist(1, 20, Number(toplistCard.hash), toplistCard.source);
                            playListSongsWindow.opened(model);
                            window.exitIndex = 2;
                        }
                    }
                }
            }


        }
        Item {
            id: album
            visible: false
            width: playlistChildPage.width
            height: playlistChildPage.height
            property int singerTypeIndex: 0
            property int singerPage: 1
            // 歌手类型入口：area 按平台映射（酷狗 / 网易云）
            property var singerTypes: [
                { title: "华语", kg: 1, ne: 7 },
                { title: "欧美", kg: 2, ne: 96 },
                { title: "日本", kg: 5, ne: 8 },
                { title: "韩国", kg: 4, ne: 16 },
                { title: "热门歌手", kg: 3, ne: 0 }
            ]
            function loadSingers(typeIndex: int): void {
                singerTypeIndex = typeIndex;
                singerPage = 1;
                MusicApi.singerList.clear();
                const area = MusicApi.songSource === 0 ? singerTypes[typeIndex].kg : singerTypes[typeIndex].ne;
                if(area === 0) {
                    MusicApi.getHotSingers(1, 30, MusicApi.songSource);
                } else {
                    MusicApi.getSingerCategory(area, 1, 30, MusicApi.songSource);
                }
            }
            Component.onCompleted: {
                loadSingers(0);
            }
            // 歌手类型标签
            Flow {
                id: singerTypeFlow
                y: 72
                spacing: 8
                width: parent.width
                Repeater {
                    model: album.singerTypes
                    delegate: Rectangle {
                        id: singerTypeRow
                        required property string title
                        required property int index
                        width: 76
                        height: 32
                        radius: 16
                        color: album.singerTypeIndex === singerTypeRow.index ? Style.themeColor : Style.primaryColor
                        border.color: Style.sideColor
                        border.width: 1
                        Rectangle {
                            anchors.fill: parent
                            radius: 16
                            color: Style.hoverColor
                            opacity: singerTypeArea.containsMouse ? 1 : 0
                            z: 1
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                        }
                        Text {
                            anchors.fill: parent
                            text: singerTypeRow.title
                            elide: Text.ElideRight
                            z: 2
                            font.pixelSize: Style.settings.text
                            color: album.singerTypeIndex === singerTypeRow.index ? Style.fullColor : Style.textColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        MouseArea {
                            id: singerTypeArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: album.loadSingers(singerTypeRow.index)
                        }
                    }
                }
            }
            QGridView {
                id: singerFlick
                y: 104
                width: parent.width + 16
                height: parent.height - 104
                clip: true
                cellWidth: 120
                cellHeight: 156
                rightMargin: -12
                leftMargin: 12
                topMargin: 12

                onAtYEndChanged: {
                    if (atYEnd && !MusicApi.loadState && singerFlick.count !== 0) {
                        if (MusicApi.singerList.count % 30 === 0) {
                            album.singerPage += 1;
                            const area = MusicApi.songSource === 0
                                ? album.singerTypes[album.singerTypeIndex].kg
                                : album.singerTypes[album.singerTypeIndex].ne;
                            if(area === 0) {
                                MusicApi.getHotSingers(album.singerPage, 30, MusicApi.songSource);
                            } else {
                                MusicApi.getSingerCategory(area, album.singerPage, 30, MusicApi.songSource);
                            }
                        } else {
                            mainWarn.tiped("没有更多了",0);
                        }
                    }
                }
                model: MusicApi.singerList
                delegate: Item {
                    id: singerCard
                    required property string cover
                    required property string title
                    required property string hash
                    width: 96
                    height: 132
                    scale: singerArea.containsMouse ? 1.06 : 1.0
                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutExpo } }
                    QPicture {
                        width: 96
                        height: 96
                        radius: 48
                        source: (singerCard.cover || "").replace("{size}","128") || "qrc:/QueMusic/resources/app/musicpic.png"
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        y: 100
                        width: parent.width
                        text: singerCard.title
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        font.pixelSize: Style.settings.text
                        color: Style.textColor
                    }
                    MouseArea {
                        id: singerArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            MusicApi.playlistSong.clear();
                            MusicApi.globalid = singerCard.hash;
                            MusicApi.getSingerSongs(singerCard.hash, 1, 20, MusicApi.songSource);
                            singerSongsWindow.opened(singerCard.title, (singerCard.cover || "").replace("{size}","256"));
                            window.exitIndex = 2;
                        }
                    }
                }
            }
        }
    }

    // 歌单/榜单/歌手歌曲共用窗口
    PlayListWindow {
        id: playListSongsWindow
        mainTarget: playlistChildPage
        winIndex: 2
        property string listType: "playlist"   // playlist 歌单 / toplist 榜单
        function reload(page: int): void {
            const id = MusicApi.globalid;
            if (listType === "toplist")
                MusicApi.getMusicToplist(page, 20, Number(id), MusicApi.songSource);
            else
                MusicApi.getPlaylistSongs(id, page, 20, MusicApi.songSource);
        }
        content: Item {
            QListView {
                id: playListsView
                x: 24
                y: 184
                width: playListSongsWindow.width - 32
                height: playListSongsWindow.height - 184
                model: MusicApi.playlistSong
                clip: true
                topMargin: 8
                bottomMargin: 24
                onClicked: (index) => {
                    if(Options.settings.soundQuality === 0) {
                        MusicApi.getMusicInfo(model.get(index).hash);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(model.get(index).hashhq);
                    } else {
                        MusicApi.getMusicInfo(model.get(index).hashsq);
                    }
                }
                onEnded: {
                    if (MusicApi.loadState)
                        return;
                    if (MusicApi.playlistSong.count % 20 !== 0) {
                        mainWarn.tiped("没有更多了", 0);
                        return;
                    }
                    playListSongsWindow.reload(MusicApi.playlistSong.count / 20 + 1);
                }
            }
        }
    }
    AnimatorWindow {
        id: singerSongsWindow
        mainTarget: playlistChildPage
        winIndex: 2
        content: Item {
            anchors.fill: parent
            QListView {
                id: singerSongsView
                x: 24
                y: 128
                width: singerSongsWindow.width - 32
                height: singerSongsWindow.height - 128
                model: MusicApi.playlistSong
                clip: true
                topMargin: 8
                bottomMargin: 24
                onClicked: (index) => {
                    if(Options.settings.soundQuality === 0) {
                        MusicApi.getMusicInfo(model.get(index).hash);
                    } else if(Options.settings.soundQuality === 1) {
                        MusicApi.getMusicInfo(model.get(index).hashhq);
                    } else {
                        MusicApi.getMusicInfo(model.get(index).hashsq);
                    }
                }
                onEnded: {
                    if (MusicApi.loadState)
                        return;
                    if (MusicApi.playlistSong.count % 20 !== 0) {
                        mainWarn.tiped("没有更多了", 0);
                        return;
                    }
                    MusicApi.getSingerSongs(MusicApi.globalid, MusicApi.playlistSong.count / 20 + 1, 20, MusicApi.songSource);
                }
            }
        }
    }
}
