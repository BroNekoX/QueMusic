// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0
import QtQuick.Controls.Basic

Item {
    id: favouritePage
    property int setMode: 0                 // 歌单选择：0 无 | 2 选择歌单
    property list<int> chooseIndex: []
    property var favSortOptions: [
        { label: "默认顺序", mode: 0, desc: false },
        { label: "标题 A→Z", mode: 1, desc: false },
        { label: "标题 Z→A", mode: 1, desc: true },
        { label: "歌手 A→Z", mode: 2, desc: false },
        { label: "歌手 Z→A", mode: 2, desc: true },
        { label: "时长 小→大", mode: 3, desc: false },
        { label: "时长 大→小", mode: 3, desc: true }
    ]
    signal scrollTop

    QSortModel {
        id: listSort
        model: FavoritePlaylists
        options: favouritePage.favSortOptions
        nameRole: "title"
    }
    QMenu {
        id: sortMenu
        model: favouritePage.favSortOptions.map(o => o.label)
        current: listSort.menuIndex
        onClicked: (i) => {
            listSort.selectMenu(i);
            favouritePage.setMode = 0;
            favouritePage.chooseIndex = [];
            favouritePage.scrollTop;
        }
    }

    // 打开收藏夹：切到该夹歌曲并滑出歌曲窗口
    function openFavoriteFolder(folderId: int, name: string): void {
        FavoriteSongs.folderId = folderId
        FavoriteSongs.searchText = ""
        songFavListMusic.setMode = 0
        songFavListMusic.chooseIndex = []
        songFavListMusic.opened(name, "")
    }

    function chooseTotal(): int {
        return FavoritePlaylists.count;
    }

    function isAllChosen(): bool {
        const total = favouritePage.chooseTotal();
        return total > 0 && favouritePage.chooseIndex.length >= total;
    }

    function toggleAllChoose(): void {
        const total = favouritePage.chooseTotal();
        if (total === 0) return;
        if (favouritePage.isAllChosen()) {
            favouritePage.chooseIndex = [];
            return;
        }
        const all = [];
        for (let i = 0; i < total; i++) all.push(i);
        favouritePage.chooseIndex = all;
    }

    QPages {
        id: favouriteChildPage
        x: 24
        y: 68
        width: parent.width - 48
        height: parent.height - 68
        pageList: [songsLoader,listsLoader,singer,historyLoader]

        // 顶部常驻显示
        Item {
            x: 0
            y: -44
            height: 40
            width: favouriteChildPage.width
            z: 10
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "收藏内容"
                font.weight: Font.DemiBold
                font.pixelSize: Style.pageTitle
                color: Theme.fontColor
            }
        }

        QBlurTapBar {
            x: 0
            y: 12
            z: 5
            model: ["歌曲","歌单","歌手","历史"]
            tabWidth: 75
            width: 306
            rectXy: Qt.rect(0, 12, width, 40)
            blurSource: favouriteChildPage.pageList[favouriteChildPage.lastIndex]
            onTabChange: (index) => {
                favouriteChildPage.stack(index);
                favouritePage.setMode = 0;
                favouritePage.chooseIndex = [];
            }
        }

        // 右侧操作区
        Row {
            x: parent.width - width
            y: 13
            z: 2
            spacing: 8
            QButton {
                visible: favouriteChildPage.lastIndex === 0
                height: 38
                text: "新建收藏夹"
                iconCharacter: "\uf0f8"
                buttonColor: Theme.primaryColor
                QAlertDialog {
                    id: newFavDialog
                    title: "新建收藏夹"
                    message: "为收藏夹设定一个名称："
                    isInput: true
                    onConfirm: {
                        if (input !== "") {
                            FavoriteFolders.addFolder(input)
                            Options.warned("已新建收藏夹", 1)
                        } else {
                            Options.warned("请输入名称", 0)
                        }
                    }
                }
                onClicked: newFavDialog.open()
            }
            QButton {
                id: sortBtn
                visible: favouriteChildPage.lastIndex === 1
                height: 38
                text: "排序"
                iconCharacter: "\uf10b"
                buttonColor: Theme.primaryColor
                onClicked: sortMenu.popup(sortBtn, 0, sortBtn.height + 6)
            }
            QButton {
                visible: favouriteChildPage.lastIndex === 3
                height: 38
                text: "清空历史"
                iconCharacter: "\uf08e"
                onClicked: Options.dialog.openSimpleDialog("清空", "将清空全部播放历史，是否继续？",
                    function() {
                        Playback.clearHistory()
                        Options.warned("已清空播放历史", 1)
                    })
            }
            QButton {
                visible: favouriteChildPage.lastIndex === 1
                height: 38
                text: favouritePage.setMode === 2 ? "取消选择" : "选择"
                iconCharacter: "\uf09f"
                buttonColor: favouritePage.setMode === 2 ? Theme.containColor : Theme.primaryColor
                onClicked: {
                    if(favouritePage.setMode === 2) {
                        favouritePage.setMode = 0;
                        favouritePage.chooseIndex = [];
                    } else {
                        favouritePage.setMode = 2;
                    }
                }
            }
        }

        // 歌曲收藏夹列表（与本地音乐「我的文件夹」同款双栏样式）
        Loader {
            id: songsLoader
            asynchronous: true
            active: visible
            visible: true
            sourceComponent: QLocalView {
                id: favFolderView
                width: favouriteChildPage.width + 16
                height: favouriteChildPage.height
                model: FavoriteFolders
                clip: true
                topMargin: 72
                headerModel: ["收藏夹","","","操作"]

                QAlertDialog {
                    id: editFavDialog
                    title: "重命名"
                    message: "为收藏夹重新命名："
                    isInput: true
                    property int folderId
                    onConfirm: {
                        if (input !== "") {
                            FavoriteFolders.renameFolder(editFavDialog.folderId, input)
                        } else {
                            Options.warned("请输入名称", 0)
                        }
                    }
                }

                delegate: Rectangle {
                    id: favFolderRow
                    required property int folderId
                    required property string name
                    required property bool isDefault
                    height: 64
                    width: favFolderView.width - 16
                    radius: Style.labelRadius
                    color: "#00000000"

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.labelRadius
                        color: Theme.hoverColor
                        opacity: favFolderArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }
                    Rectangle {
                        y: 8; x: 8; z: 2; width: 48; height: 48
                        color: Theme.containColor; radius: 10
                        Text {
                            anchors.fill: parent
                            text: "\uf0f5"
                            font.family: Fonts.icon
                            font.pixelSize: Style.texticon
                            color: Theme.fontColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                    Text {
                        x: 72; y: 0; z: 2
                        width: favFolderView.width - 150
                        height: 64
                        text: favFolderRow.name
                        color: Theme.fontColor
                        font.bold: true
                        font.pixelSize: Style.textmain
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    MouseArea {
                        id: favFolderArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: favouritePage.openFavoriteFolder(favFolderRow.folderId, favFolderRow.name)
                        Row {
                            anchors.right: parent.right
                            anchors.rightMargin: 20
                            y: 12
                            height: 36
                            spacing: 2
                            z: 3
                            SButton {
                                iconCharacter: "\uf005"
                                width: 36; height: 36; radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                tipText: "重命名收藏夹"
                                onClicked: {
                                    if (favFolderRow.isDefault) { Options.warned("无法修改默认收藏夹名称", 0); return }
                                    editFavDialog.input = favFolderRow.name
                                    editFavDialog.folderId = favFolderRow.folderId
                                    editFavDialog.open()
                                }
                            }
                            SButton {
                                iconCharacter: "\uf08e"
                                width: 36; height: 36; radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                shadowEnabled: false
                                tipText: "删除收藏夹"
                                onClicked: {
                                    if (favFolderRow.isDefault) { Options.warned("无法删除默认收藏夹", 0); return }
                                    Options.dialog.openSimpleDialog("删除",
                                        "将删除收藏夹「" + favFolderRow.name + "」及其中的歌曲收藏，是否继续？",
                                        function() {
                                            FavoriteFolders.deleteFolder(favFolderRow.folderId)
                                            Options.warned("已删除收藏夹", 1)
                                        })
                                }
                            }
                        }
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: FavoriteFolders.count === 0
                    text: "还没有收藏夹"
                    color: Theme.textColor
                    font.pixelSize: 14
                }
            }
        }

        Loader {
            id: listsLoader
            asynchronous: true
            active: visible
            visible: false
            sourceComponent: QListView {
                id: lists
                width: favouriteChildPage.width + 16
                height: favouriteChildPage.height
                model: listSort
                clip: true
                isList: true
                topMargin: 72
                selectedIndices: favouritePage.chooseIndex
                Connections {
                    target: favouritePage
                    function onScrollTop(): void { lists.scrollTop() }
                }

                onClicked: (index) => {
                    if (favouritePage.setMode === 2) {
                        const idx = favouritePage.chooseIndex.indexOf(index);
                        if (idx === -1) {
                            favouritePage.chooseIndex = favouritePage.chooseIndex.concat([index]);
                        } else {
                            favouritePage.chooseIndex = favouritePage.chooseIndex.filter(v => v !== index);
                        }
                    } else {
                        const r = listSort.at(index);
                        MusicApi.playlistSong.clear();
                        MusicApi.globalid = r.id;
                        MusicApi.getPlaylistSongs(r.id,1,20,r.source);
                        playListSongsWindow.songSource = r.source;
                        playListSongsWindow.opened({ id: r.id, title: r.title, artist: r.artist, cover: r.cover, duration: r.duration });
                        Options.exitIndex = 1;
                    }
                }
                AnimatedImage {
                    anchors.centerIn: parent
                    visible: FavoritePlaylists.loading
                    playing: visible
                    source: "qrc:/QueMusic/resources/loader.gif"
                }
                Text {
                    anchors.centerIn: parent
                    visible: !FavoritePlaylists.loading && FavoritePlaylists.count === 0
                    text: "没有收藏的内容？快去收藏一些歌单吧"
                    color: Theme.textColor
                    font.pixelSize: 14
                }
            }
        }
        Item {
            id: singer
            visible: false
            width: favouriteChildPage.width
            height: favouriteChildPage.height
            Text {
                anchors.centerIn: parent
                text: "喜欢的歌手"
                color: Theme.textColor
                font.pixelSize: 14
            }
        }

        Loader {
            id: historyLoader
            asynchronous: true
            active: visible
            visible: false
            sourceComponent: QListView {
                id: history
                width: favouriteChildPage.width + 16
                height: favouriteChildPage.height
                model: Playback.history
                clip: true
                topMargin: 72
                menuModel: ["加入播放列表", "收藏", "移除记录"]
                toolText0: "\uf095"
                toolText1: "\uf0c8"

                function addToQueue(e: var): void {
                    if (Options.queue.indexOfPath(e.path) !== -1) return
                    Options.queue.append({ name: e.title, path: e.path, songer: e.artist, source: e.source })
                    Options.warned("已加入播放列表", 1)
                }
                function toggleFav(e: var): void {
                    if (e.source === -1) { Options.warned("本地歌曲请使用本地收藏", 0); return }
                    Options.favPicker.show({ id: e.path, title: e.title, artist: e.artist,
                                             cover: e.cover, source: e.source, duration: e.duration })
                }

                onClicked: (index) => {
                    const e = Playback.history.get(index)
                    Playback.playItem({ name: e.title, path: e.path, songer: e.artist, source: e.source })
                }
                // 记录列表有专属菜单项（移除记录），其余动作仍走内置实现
                menuHandler: function(choice: int, index: int): bool {
                    const e = Playback.history.get(index)
                    if (choice === 0) { history.addToQueue(e); return true }
                    if (choice === 1) { history.toggleFav(e); return true }
                    Playback.history.remove(index, 1)
                    return true
                }
                Text {
                    anchors.centerIn: parent
                    visible: Playback.history.count === 0
                    text: "还没有播放记录，去听点什么吧"
                    color: Theme.textColor
                    font.pixelSize: 14
                }
            }
        }

        // 歌单选择操作条
        Rectangle {
            id: chooseArea
            x: -24
            y: favouriteChildPage.height - 60
            opacity: visible ? 1 : 0
            width: favouritePage.width
            height: 60
            visible: favouritePage.setMode === 2
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Theme.sideColor }
            }
            Behavior on opacity { NumberAnimation { duration: 320; easing.type: Easing.OutCubic } }
            QButton {
                shadowEnabled: false
                x: 16
                y: 12
                width: 92
                height: 36
                radius: 20
                borderWidth: 1
                buttonColor: favouritePage.isAllChosen() ? Theme.themeColor : Theme.fullColor
                textColor: favouritePage.isAllChosen() ? Theme.primaryColor : Theme.fontColor
                text: "全选"
                onClicked: favouritePage.toggleAllChoose()
            }
            Rectangle {
                x: 118
                y: 12
                width: 92
                height: 36
                radius: 20
                color: "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "已选择:" + favouritePage.chooseIndex.length + "项"
                    color: Theme.textColor
                    font.pixelSize: Style.textmain
                }
            }
            Row {
                y: 12
                x: chooseArea.width - width - 16
                spacing: 8
                QButton {
                    shadowEnabled: false
                    height: 36
                    radius: 20
                    borderWidth: 1
                    buttonColor: "#fa4642"
                    text: "取消收藏"
                    onClicked: Options.dialog.openSimpleDialog("取消收藏",
                        "将取消收藏选中的 " + favouritePage.chooseIndex.length + " 个歌单",
                        function() {
                            const total = favouritePage.chooseIndex.length;
                            for(let i = 0; i < total; i++) {
                                const r = listSort.at(favouritePage.chooseIndex[i]);
                                if (r) FavoritePlaylists.removeFavorite(r.id, "playlist");
                            }
                            favouritePage.chooseIndex = [];
                            Options.warned("已取消收藏 " + total + " 个歌单", 1);
                        })
                }
                QButton {
                    shadowEnabled: false
                    width: 92
                    height: 36
                    radius: 20
                    borderWidth: 1
                    buttonColor: Theme.themeColor
                    textColor: Theme.primaryColor
                    text: "完成"
                    onClicked: {
                        favouritePage.setMode = 0;
                        favouritePage.chooseIndex = [];
                    }
                }
            }
        }
    }

    PlayListWindow {
        id: playListSongsWindow
        mainTarget: favouriteChildPage
        winIndex: 1
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
                    if(Config.soundQuality === 0) {
                        MusicApi.getMusicInfo(model.get(index).hash,0,playListSongsWindow.songSource);
                    } else if(Config.soundQuality === 1) {
                        MusicApi.getMusicInfo(model.get(index).hashhq,0,playListSongsWindow.songSource);
                    } else {
                        MusicApi.getMusicInfo(model.get(index).hashsq,0,playListSongsWindow.songSource);
                    }
                }

                onEnded: {
                    if(MusicApi.playlistSong.count % 20 === 0 && MusicApi.playlistSong.count !== 0) {
                        const tagid = playListSongsWindow.id;
                        MusicApi.getPlaylistSongs(tagid,MusicApi.playlistSong.count / 20 + 1,20,playListSongsWindow.songSource);
                        isEnd = false;
                    } else {
                        if(MusicApi.playlistSong.count !== 0) {
                            isEnd = true;
                        }
                    }
                }
            }
        }
    }

    // 收藏夹内的歌曲：搜索 / 排序 / 刷新 / 播放全部 / 批量取消收藏
    AnimatorWindow {
        id: songFavListMusic
        mainTarget: favouriteChildPage
        winIndex: 1
        property int setMode: 0
        property list<int> chooseIndex: []

        content: Item {
            width: songFavListMusic.width
            height: songFavListMusic.height

            QSortModel {
                id: favSongSort
                model: FavoriteSongs
                options: favouritePage.favSortOptions
                nameRole: "title"
            }

            QMenu {
                id: favSongSortMenu
                model: favSongSort.options.map(o => o.label)
                current: favSongSort.menuIndex
                onClicked: (i) => {
                    favSongSort.selectMenu(i)
                    favSongView.scrollTop()
                }
            }

            // 全部加入播放列表，play=true 时立即播放
            function playAll(play: bool): void {
                const total = FavoriteSongs.count
                if (total === 0) { Options.warned("当前收藏夹没有歌曲", 0); return }
                const rows = []
                for (let i = 0; i < total; i++) {
                    const item = favSongSort.at(i)
                    if (item && item.id)
                        rows.push({ name: item.title, path: item.id, songer: item.artist, source: item.source })
                }
                const added = rows.length > 0 ? Options.queue.appendBatch(rows) : 0
                if (added === 0) {
                    Options.warned("列表中的歌曲都已在播放列表中", 0)
                } else {
                    Options.warned("已加入播放列表 " + added + " 首", 1)
                    if (play) Playback.goTo(Options.queue.count - added)
                }
            }

            function removeChosen(): void {
                const total = songFavListMusic.chooseIndex.length
                if (total === 0) return
                for (let i = 0; i < total; i++) {
                    const item = favSongSort.at(songFavListMusic.chooseIndex[i])
                    if (item) FavoriteSongs.removeFavorite(item.id, "song")
                }
                songFavListMusic.chooseIndex = []
                songFavListMusic.setMode = 0
                Options.warned("已取消收藏 " + total + " 首歌曲", 1)
            }

            // 顶栏
            Row {
                x: 136
                y: 76
                height: 36
                spacing: 6
                QButton {
                    height: 36; width: 96
                    radius: Style.labelRadius
                    iconCharacter: "\uf00e"
                    text: "播放"
                    shadowEnabled: false
                    buttonColor: Theme.sideColor
                    tipText: "播放当前收藏夹全部歌曲"
                    onClicked: playAll(true)
                }
                SButton {
                    width: 36; height: 36; radius: Style.labelRadius
                    iconCharacter: "\uf095"
                    shadowEnabled: false
                    buttonColor: Theme.sideColor
                    tipText: "全部加入播放列表"
                    onClicked: playAll(false)
                }
                SButton {
                    width: 36; height: 36; radius: Style.labelRadius
                    iconCharacter: "\uf10c"
                    shadowEnabled: false
                    buttonColor: Theme.sideColor
                    tipText: "刷新收藏夹"
                    onClicked: {
                        favSearchInput.text = ""
                        FavoriteSongs.refresh()
                        Options.warned("已刷新当前收藏夹", 1)
                    }
                }
                SButton {
                    id: favSortBtn
                    width: 48; height: 36; radius: Style.labelRadius
                    iconCharacter: "\uf10b"
                    shadowEnabled: false
                    buttonColor: Theme.sideColor
                    tipText: "排序方式"
                    onClicked: favSongSortMenu.popup(favSortBtn, 0, favSortBtn.height + 6)
                }
            }

            Row {
                x: songFavListMusic.width - width - 24
                y: 26
                height: 40
                spacing: 8
                z: 2
                QButton {
                    height: 40
                    radius: 20
                    text: songFavListMusic.setMode === 2 ? "取消选择" : "选择"
                    iconCharacter: "\uf09f"
                    buttonColor: songFavListMusic.setMode === 2 ? Theme.containColor : Theme.primaryColor
                    onClicked: {
                        if (songFavListMusic.setMode === 2) {
                            songFavListMusic.setMode = 0
                            songFavListMusic.chooseIndex = []
                        } else {
                            songFavListMusic.setMode = 2
                        }
                    }
                }
            }

            TextField {
                id: favSearchInput
                x: songFavListMusic.width - width - 24
                y: 76
                z: 3
                width: 200
                height: 36
                leftPadding: 12
                rightPadding: 38
                placeholderText: "搜索与过滤"
                placeholderTextColor: Theme.textColor
                color: Theme.textColor
                font.pixelSize: Style.text
                verticalAlignment: Text.AlignVCenter
                selectionColor: Theme.containColor
                onTextChanged: FavoriteSongs.searchText = text
                background: Rectangle {
                    radius: Style.labelRadius
                    color: Theme.primaryColor
                    border.width: 2
                    border.color: favSearchInput.focus ? Theme.themeColor : Theme.sideColor
                }
                SButton {
                    visible: favSearchInput.text !== ""
                    y: 2
                    x: parent.width - 34
                    width: 32; height: 32
                    radius: Style.labelRadius
                    iconCharacter: "\uf025"
                    iconSize: 15
                    buttonColor: "transparent"
                    shadowEnabled: false
                    onClicked: favSearchInput.text = ""
                }
            }

            QListView {
                id: favSongView
                x: 24
                y: 128
                width: songFavListMusic.width - 32
                height: songFavListMusic.height - 128
                model: favSongSort
                clip: true
                topMargin: 8
                bottomMargin: 24
                selectedIndices: songFavListMusic.chooseIndex
                onClicked: (index) => {
                    if (songFavListMusic.setMode === 2) {
                        const idx = songFavListMusic.chooseIndex.indexOf(index)
                        songFavListMusic.chooseIndex = idx === -1
                            ? songFavListMusic.chooseIndex.concat([index])
                            : songFavListMusic.chooseIndex.filter(v => v !== index)
                    } else {
                        const item = favSongSort.at(index)
                        if (item) MusicApi.getMusicInfo(item.id, 0, item.source)
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: !FavoriteSongs.loading && FavoriteSongs.count === 0
                    text: "这个收藏夹还没有歌曲"
                    color: Theme.textColor
                    font.pixelSize: 14
                }
            }

            // 选择操作条
            Rectangle {
                x: 0
                y: songFavListMusic.height - 60
                width: songFavListMusic.width
                height: 60
                visible: songFavListMusic.setMode === 2
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: Theme.sideColor }
                }
                QButton {
                    shadowEnabled: false
                    x: 16; y: 12
                    width: 92; height: 36; radius: 20; borderWidth: 1
                    buttonColor: (FavoriteSongs.count > 0
                                  && songFavListMusic.chooseIndex.length >= FavoriteSongs.count)
                                 ? Theme.themeColor : Theme.fullColor
                    textColor: (FavoriteSongs.count > 0
                                && songFavListMusic.chooseIndex.length >= FavoriteSongs.count)
                               ? Theme.primaryColor : Theme.fontColor
                    text: "全选"
                    onClicked: {
                        const total = FavoriteSongs.count
                        if (total === 0) return
                        if (songFavListMusic.chooseIndex.length >= total) {
                            songFavListMusic.chooseIndex = []
                        } else {
                            const all = []
                            for (let i = 0; i < total; i++) all.push(i)
                            songFavListMusic.chooseIndex = all
                        }
                    }
                }
                Text {
                    x: 118; y: 12
                    width: 130; height: 36
                    text: "已选择:" + songFavListMusic.chooseIndex.length + "项"
                    color: Theme.textColor
                    font.pixelSize: Style.textmain
                    verticalAlignment: Text.AlignVCenter
                }
                Row {
                    y: 12
                    x: parent.width - width - 16
                    spacing: 8
                    QButton {
                        shadowEnabled: false
                        height: 36; radius: 20; borderWidth: 1
                        buttonColor: "#fa4642"
                        text: "取消收藏"
                        onClicked: removeChosen()
                    }
                    QButton {
                        shadowEnabled: false
                        height: 36; radius: 20; borderWidth: 1
                        text: "加入播放列表"
                        onClicked: playAll(false)
                    }
                    QButton {
                        shadowEnabled: false
                        width: 92; height: 36; radius: 20; borderWidth: 1
                        buttonColor: Theme.themeColor
                        textColor: Theme.primaryColor
                        text: "完成"
                        onClicked: {
                            songFavListMusic.setMode = 0
                            songFavListMusic.chooseIndex = []
                        }
                    }
                }
            }
        }
    }
}
