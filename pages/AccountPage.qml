// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick

import QueMusic 1.0

Item {
    id: accountPage
    // 当前平台（0 酷狗 / 1 网易云 / 2 B站 / 3 QQ / 4 Spotify / 5 Apple）
    property int platformSource: 0
    property int kind: 0 // 0 收藏歌曲 / 1 收藏歌单 / 2 关注歌手
    onKindChanged: accountLists.stack(kind);

    readonly property bool loggedIn: platformSource === 0 ? AccountManager.kugouLoggedIn
        : platformSource === 1 ? AccountManager.neteaseLoggedIn
        : platformSource === 2 ? AccountManager.bilibiliLoggedIn
        : platformSource === 3 ? AccountManager.qqLoggedIn
        : platformSource === 4 ? AccountManager.spotifyLoggedIn
        : AccountManager.appleLoggedIn

    readonly property string nickname: platformSource === 0 ? AccountManager.kugouNickname
        : platformSource === 1 ? AccountManager.neteaseNickname
        : platformSource === 2 ? AccountManager.bilibiliNickname
        : platformSource === 3 ? AccountManager.qqNickname
        : platformSource === 4 ? AccountManager.spotifyNickname
        : AccountManager.appleNickname

    readonly property string avatar: platformSource === 0 ? AccountManager.kugouAvatar
        : platformSource === 1 ? AccountManager.neteaseAvatar
        : platformSource === 2 ? AccountManager.bilibiliAvatar
        : platformSource === 3 ? AccountManager.qqAvatar
        : platformSource === 4 ? AccountManager.spotifyAvatar
        : AccountManager.appleAvatar

    function currentModel() {
        return kind === 0 ? MusicApi.userFavoriteSongs
             : kind === 1 ? MusicApi.userPlaylists
             : MusicApi.followedArtists;
    }

    function reload(): void {
        const source = platformSource;
        MusicApi.userFavoriteSongs.clear();
        MusicApi.userPlaylists.clear();
        MusicApi.followedArtists.clear();
        if (!loggedIn) return;
        MusicApi.getUserFavoriteSongs(1, 20, source);
        MusicApi.getUserPlaylists(1, 20, source);
        MusicApi.getFollowedArtists(1, 20, source);
    }

    function more(): void {
        const model = currentModel();
        const view = kind === 0 ? favSongsView : kind === 1 ? playlistsView : artistsView;
        if (!loggedIn || model.count === 0) {
            view.isEnd = false;
            return;
        }
        if (model.count % 20 !== 0) {
            view.isEnd = true;
            return;
        }
        const page = model.count / 20 + 1;
        if (kind === 0) MusicApi.getUserFavoriteSongs(page, 20, platformSource);
        else if (kind === 1) MusicApi.getUserPlaylists(page, 20, platformSource);
        else MusicApi.getFollowedArtists(page, 20, platformSource);
    }

    function activate(index: int): void {
        const item = currentModel().get(index);
        if (kind === 0) {
            MusicApi.playSong(item);
            return;
        }
        MusicApi.playlistSong.clear();
        MusicApi.globalid = item.hash;
        if (kind === 1) {
            MusicApi.getPlaylistSongs(item.hash, 1, 20, platformSource);
            playlistWindow.opened(item);
        } else {
            MusicApi.getSingerSongs(item.hash, 1, 20, platformSource);
            singerWindow.opened(item.title, (item.cover || "").replace("{size}", "256"));
        }
        Options.exitIndex = 2;
    }

    readonly property int currentCount: kind === 0 ? MusicApi.userFavoriteSongs.count
        : kind === 1 ? MusicApi.userPlaylists.count : MusicApi.followedArtists.count

    function toggleSongFavorite(item: var): void {
        if (!item || !item.hash) return;
        const id = item.hash;
        if (FavoriteSongs.isFavorite(id, "song")) {
            FavoriteSongs.removeFavorite(id, "song");
            Options.warned("已取消收藏", 1);
        } else {
            FavoriteSongs.addFavorite(id, item.title, item.artist, item.cover,
                                      platformSource, item.duration, "song");
            Options.warned("已收藏", 1);
        }
    }

    function togglePlaylistFavorite(item: var): void {
        if (!item || !item.hash) return;
        const id = item.hash;
        if (FavoritePlaylists.isFavorite(id, "playlist")) {
            FavoritePlaylists.removeFavorite(id, "playlist");
            Options.warned("已取消收藏", 1);
        } else {
            FavoritePlaylists.addFavorite(id, item.title, item.artist, item.cover,
                                          platformSource, item.duration, "playlist");
            Options.warned("已收藏歌单", 1);
        }
    }

    function runSongMenu(index: int, choice: int): void {
        const item = MusicApi.userFavoriteSongs.get(index);
        if (!item) return;
        if (choice === 0) MusicApi.playSong(item);
        else if (choice === 1) toggleSongFavorite(item);
        else if (choice === 2) MusicApi.downloadSong(item, platformSource);
    }

    function runPlaylistMenu(index: int, choice: int): void {
        if (choice === 0) activate(index);
        else togglePlaylistFavorite(MusicApi.userPlaylists.get(index));
    }

    function runArtistMenu(index: int, choice: int): void {
        const item = MusicApi.followedArtists.get(index);
        if (!item) return;
        if (choice === 0) activate(index);
        else copyText(item.title);
    }

    function copyText(text: string): void {
        clipboardHelper.text = text;
        clipboardHelper.selectAll();
        clipboardHelper.copy();
        Options.warned("已复制", 1);
    }

    onPlatformSourceChanged: reload()
    Component.onCompleted: reload()

    Item {
        id: accountMain
        anchors.fill: parent

    // 顶部标题
    Item {
        x: 24
        y: 24
        height: 40
        width: parent.width - 48
        z: 10
        Text {
            x: 0
            y: 0
            height: 40
            anchors.verticalCenter: parent.verticalCenter
            text: "账户"
            font.weight: Font.DemiBold
            font.pixelSize: Style.pageTitle
            color: Theme.fontColor
        }
        Row {
            x: parent.width - width
            y: 2
            z: 2
            spacing: 8
            QDrop {
                width: 128
                height: 36
                radius: 18
                cardRadius: Style.labelRadius
                choice: Math.max(0, MusicApi.sourceIds.indexOf(accountPage.platformSource))
                model: MusicApi.sourceNames
                onTransformed: (choiced) => {
                    accountPage.platformSource = MusicApi.sourceIdAt(choiced);
                }
            }
        }
    }

    // 标签栏
    QBlurTapBar {
        x: 24
        y: 80
        z: 5
        model: ["收藏歌曲", "收藏歌单", "关注歌手"]
        tabWidth: 90
        width: 276
        rectXy: Qt.rect(0, 12, width, 40)
        blurSource: accountLists
        onTabChange: (index) => {
            accountPage.kind = index;
        }
    }

    // 账户信息：头像 + 昵称 + 数量
    Rectangle {
        id: accountInfo
        y: 80
        x: parent.width - width - 24
        height: 40
        width: accountPage.width < infoRow.width + 276 + 48 ? 160 : infoRow.width
        color: Theme.primaryColor
        radius: 20
        Row {
            id: infoRow
            height: 40
            width: implicitWidth
            spacing: 8
            padding: 2
            QPicture {
                anchors.verticalCenter: parent.verticalCenter
                image: accountPage.avatar
                visible: accountPage.loggedIn && image !== ""
                width: 36
                height: 36
                radius: 18
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: accountPage.loggedIn ? accountPage.nickname : "未登录"
                color: Theme.fontColor
                font.pixelSize: Style.textmain
                font.bold: true
                elide: Text.ElideRight
                width: Math.min(implicitWidth, 120)
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: accountPage.loggedIn
                      ? "收藏歌单 " + MusicApi.userPlaylists.count + " · 关注歌手 " + MusicApi.followedArtists.count
                      : "请到设置登录"
                color: Theme.textColor
                font.pixelSize: Style.text
                elide: Text.ElideRight
                opacity: accountPage.width >= infoRow.width + 276 + 48 ? 1 : 0
                width: implicitWidth + 6
            }
        }
    }

    // 未登录提示
    Text {
        anchors.centerIn: parent
        visible: !accountPage.loggedIn
        text: "未登录，请到设置登录"
        color: Theme.textColor
        font.pixelSize: Style.textmain
    }

    Text {
        anchors.centerIn: parent
        visible: accountPage.loggedIn && accountPage.currentCount === 0
        text: "暂无内容"
        color: Theme.textColor
        font.pixelSize: Style.textmain
    }

    // 列表：三套 QLocalView（各自滚动/动画/右键菜单），点击与右键动作统一走 activate
    QPages {
        id: accountLists
        x: 24
        y: 68
        width: accountPage.width - 32
        height: accountPage.height - 68
        visible: accountPage.loggedIn
        pageList: [favSongsView,playlistsView,artistsView]

        QLocalView {
            id: favSongsView
            width: accountLists.width
            height: accountLists.height
            visible: true
            clip: true
            topMargin: 72
            model: MusicApi.userFavoriteSongs
            headerModel: ["歌曲", "歌手", "", "时长"]
            menuModel: ["立即播放", "收藏 / 取消收藏", "下载"]
            onMenuClicked: (index, choice) => accountPage.runSongMenu(index, choice)
            onEnded: accountPage.more()

            delegate: Rectangle {
                id: songRow
                required property int index
                required property string title
                required property string artist
                required property string cover
                required property string hash
                required property var duration

                width: favSongsView.width - 16
                height: 60
                radius: Style.labelRadius
                color: "transparent"
                property real transY: 0
                transform: Translate { y: songRow.transY }

                ListView.onReused: { songRow.transY = 0; songRow.opacity = 1; }
                ListView.onPooled: { songRow.transY = 0; songRow.opacity = 1; }

                QPicture {
                    x: 8
                    y: 8
                    width: 44
                    height: 44
                    radius: 10
                    image: (songRow.cover || "").replace("{size}", "96")
                    visible: image !== ""
                }
                Text {
                    x: 72
                    width: favSongsView.artistX - 110
                    anchors.verticalCenter: parent.verticalCenter
                    text: songRow.title
                    color: Theme.fontColor
                    font.weight: Font.DemiBold
                    font.pixelSize: Style.textmain
                    elide: Text.ElideRight
                }
                Text {
                    x: favSongsView.artistX
                    width: favSongsView.artistX - 128
                    anchors.verticalCenter: parent.verticalCenter
                    text: songRow.artist
                    color: Theme.textColor
                    font.pixelSize: Style.text
                    elide: Text.ElideRight
                }
                Text {
                    x: favSongsView.width - 92
                    width: 60
                    anchors.verticalCenter: parent.verticalCenter
                    text: songRow.duration > 0 ? Playback.fmt(songRow.duration * 1000) : ""
                    color: Theme.textColor
                    font.pixelSize: Style.text
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: songArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            favSongsView.menu.index = songRow.index;
                            favSongsView.menu.popup();
                            return;
                        }
                        accountPage.activate(songRow.index);
                        forceActiveFocus();
                    }
                    Rectangle {
                        anchors.fill: parent
                        radius: Style.labelRadius
                        color: Theme.hoverColor
                        opacity: songArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 128; easing.type: Easing.OutCubic } }
                        SButton {
                            x: favSongsView.toolX
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf00e"
                            tipText: "立即播放"
                            onClicked: accountPage.activate(songRow.index)
                        }
                        SButton {
                            x: favSongsView.toolX + 38
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf0c8"
                            tipText: "收藏"
                            onClicked: accountPage.toggleSongFavorite(MusicApi.userFavoriteSongs.get(songRow.index))
                        }
                        SButton {
                            x: favSongsView.toolX + 76
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf050"
                            tipText: "更多"
                            onClicked: {
                                favSongsView.menu.index = songRow.index;
                                favSongsView.menu.popup();
                            }
                        }
                    }
                }
            }
        }

        QLocalView {
            id: playlistsView
            width: accountLists.width
            height: accountLists.height
            visible: false
            clip: true
            topMargin: 72
            model: MusicApi.userPlaylists
            headerModel: ["歌单", "创建者", "", "曲目"]
            menuModel: ["打开歌单", "收藏歌单"]
            onMenuClicked: (index, choice) => accountPage.runPlaylistMenu(index, choice)
            onEnded: accountPage.more()

            delegate: Rectangle {
                id: listRow
                required property int index
                required property string title
                required property string artist
                required property string cover
                required property string hash
                required property var duration

                width: playlistsView.width - 16
                height: 60
                radius: Style.labelRadius
                color: "transparent"
                property real transY: 0
                transform: Translate { y: listRow.transY }

                ListView.onReused: { listRow.transY = 0; listRow.opacity = 1; }
                ListView.onPooled: { listRow.transY = 0; listRow.opacity = 1; }

                QPicture {
                    x: 8
                    y: 8
                    width: 44
                    height: 44
                    radius: 10
                    image: (listRow.cover || "").replace("{size}", "96")
                    visible: image !== ""
                }
                Text {
                    x: 72
                    width: playlistsView.artistX - 110
                    anchors.verticalCenter: parent.verticalCenter
                    text: listRow.title
                    color: Theme.fontColor
                    font.weight: Font.DemiBold
                    font.pixelSize: Style.textmain
                    elide: Text.ElideRight
                }
                Text {
                    x: playlistsView.artistX
                    width: playlistsView.artistX - 128
                    anchors.verticalCenter: parent.verticalCenter
                    text: listRow.artist
                    color: Theme.textColor
                    font.pixelSize: Style.text
                    elide: Text.ElideRight
                }
                Text {
                    x: playlistsView.width - 92
                    width: 60
                    anchors.verticalCenter: parent.verticalCenter
                    text: listRow.duration > 0 ? listRow.duration + " 首" : ""
                    color: Theme.textColor
                    font.pixelSize: Style.text
                    horizontalAlignment: Text.AlignHCenter
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: listArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            playlistsView.menu.index = listRow.index;
                            playlistsView.menu.popup();
                            return;
                        }
                        accountPage.activate(listRow.index);
                        forceActiveFocus();
                    }
                    Rectangle {
                        anchors.fill: parent
                        radius: Style.labelRadius
                        color: Theme.hoverColor
                        opacity: listArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 128; easing.type: Easing.OutCubic } }
                        SButton {
                            x: playlistsView.toolX
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf00e"
                            tipText: "打开歌单"
                            onClicked: accountPage.activate(listRow.index)
                        }
                        SButton {
                            x: playlistsView.toolX + 38
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            visible: accountPage.platformSource !== 2
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf0c8"
                            tipText: "收藏歌单"
                            onClicked: accountPage.togglePlaylistFavorite(
                                           MusicApi.userPlaylists.get(listRow.index))
                        }
                        SButton {
                            x: playlistsView.toolX + 76
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf050"
                            tipText: "更多"
                            onClicked: {
                                playlistsView.menu.index = listRow.index;
                                playlistsView.menu.popup();
                            }
                        }
                    }
                }
            }
        }

        QLocalView {
            id: artistsView
            width: accountLists.width
            height: accountLists.height
            visible: false
            clip: true
            topMargin: 72
            model: MusicApi.followedArtists
            headerModel: ["歌手", "", "", ""]
            menuModel: ["查看歌曲", "复制名称"]
            onMenuClicked: (index, choice) => accountPage.runArtistMenu(index, choice)
            onEnded: accountPage.more()

            delegate: Rectangle {
                id: artistRow
                required property int index
                required property string title
                required property string cover
                required property string hash

                width: artistsView.width - 16
                height: 60
                radius: Style.labelRadius
                color: "transparent"
                property real transY: 0
                transform: Translate { y: artistRow.transY }

                ListView.onReused: { artistRow.transY = 0; artistRow.opacity = 1; }
                ListView.onPooled: { artistRow.transY = 0; artistRow.opacity = 1; }

                QPicture {
                    x: 8
                    y: 8
                    width: 44
                    height: 44
                    radius: 10
                    image: (artistRow.cover || "").replace("{size}", "96")
                    visible: image !== ""
                }
                Text {
                    x: 72
                    width: artistsView.width - 160
                    anchors.verticalCenter: parent.verticalCenter
                    text: artistRow.title
                    color: Theme.fontColor
                    font.weight: Font.DemiBold
                    font.pixelSize: Style.textmain
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: artistArea
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            artistsView.menu.index = artistRow.index;
                            artistsView.menu.popup();
                            return;
                        }
                        accountPage.activate(artistRow.index);
                        forceActiveFocus();
                    }
                    Rectangle {
                        anchors.fill: parent
                        radius: Style.labelRadius
                        color: Theme.hoverColor
                        opacity: artistArea.containsMouse ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 128; easing.type: Easing.OutCubic } }
                        SButton {
                            x: artistsView.toolX
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf00e"
                            tipText: "查看歌曲"
                            onClicked: accountPage.activate(artistRow.index)
                        }
                        SButton {
                            x: artistsView.toolX + 38
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf04b"
                            tipText: "复制名称"
                            onClicked: accountPage.copyText(artistRow.title)
                        }
                        SButton {
                            x: artistsView.toolX + 76
                            y: 12
                            width: 36
                            height: 36
                            radius: 18
                            buttonColor: "transparent"
                            hoverColor: Theme.hoverColor
                            iconCharacter: "\uf050"
                            tipText: "更多"
                            onClicked: {
                                artistsView.menu.index = artistRow.index;
                                artistsView.menu.popup();
                            }
                        }
                    }
                }
            }
        }
    }
    }

    TextEdit {
        id: clipboardHelper
        visible: false
        width: 0
        height: 0
    }

    // 歌单内歌曲
    PlayListWindow {
        id: playlistWindow
        mainTarget: accountMain
        winIndex: 2
        content: QListView {
            x: 24
            y: 170
            width: playlistWindow.width - 32
            height: playlistWindow.height - 170
            model: MusicApi.playlistSong
            clip: true
            topMargin: 8
            bottomMargin: 24
            onClicked: (index) => MusicApi.playSong(model.get(index))
            onEnded: {
                if (MusicApi.loadState)
                    return;
                if (MusicApi.playlistSong.count === 0 || MusicApi.playlistSong.count % 20 !== 0) {
                    Options.warned("没有更多了", 0);
                    return;
                }
                MusicApi.getPlaylistSongs(playlistWindow.id, MusicApi.playlistSong.count / 20 + 1,
                                          20, accountPage.platformSource);
            }
        }
    }

    // 歌手歌曲
    AnimatorWindow {
        id: singerWindow
        mainTarget: accountMain
        winIndex: 2
        content: Item {
            anchors.fill: parent
            QListView {
                x: 24
                y: 128
                width: singerWindow.width - 32
                height: singerWindow.height - 128
                model: MusicApi.playlistSong
                clip: true
                topMargin: 8
                bottomMargin: 24
                onClicked: (index) => MusicApi.playSong(model.get(index))
                onEnded: {
                    if (MusicApi.loadState)
                        return;
                    if (MusicApi.playlistSong.count === 0 || MusicApi.playlistSong.count % 20 !== 0) {
                        Options.warned("没有更多了", 0);
                        return;
                    }
                    MusicApi.getSingerSongs(MusicApi.globalid, MusicApi.playlistSong.count / 20 + 1,
                                            20, accountPage.platformSource);
                }
            }
        }
    }
}