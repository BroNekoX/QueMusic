// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Effects
import QueMusic 1.0

Item {
    id: homePage
    property real toolsWindow: 0

    // 页面留白：左右合起来就是 homeView.standWidth 之外的 52
    readonly property int pagePadding: 24
    readonly property int pagePaddingRight: 28

    // 平台品牌色集中一处：tint 底色 / line 描边 / ink 文字
    readonly property var sourceStyle: [
        { ink: "#0F3975", tint: "#CDE8FF", line: "#4384F5" },
        { ink: "#750F0F", tint: "#FFCDCD", line: "#F54343" },
        { ink: "#7A1C3C", tint: "#FFD9E6", line: "#FB7299" },
        { ink: "#16750F", tint: "#CDFFCD", line: "#4DF543" },
        { ink: "#756F0F", tint: "#FFFFCD", line: "#F5F543" }
    ]
    readonly property var sourceSkin: sourceStyle[Math.max(0, Math.min(MusicApi.songSource, sourceStyle.length - 1))]

    property string greeting: homePage.greetingText()
    property var roamTitles: []   // 分类名，QBlurTapBar 只接受字符串数组
    property int roamIndex: 0     // 当前分类下标，与窗口内 tabbar 的选中项同步
    onVisibleChanged: if (visible) greeting = greetingText()

    function greetingText(): string {
        const hour = new Date().getHours()
        if (hour < 5 || hour >= 23) return "夜深了"
        if (hour < 9) return "早上好"
        if (hour < 12) return "上午好"
        if (hour < 14) return "中午好"
        if (hour < 18) return "下午好"
        return "晚上好"
    }

    function playLastSong(): void {
        if (Options.lastSongs.hash === "") return
        MusicApi.getMusicInfo(Options.lastSongs.hash, 0, Options.lastSongs.source)
    }

    function queueLastSong(): void {
        if (Options.lastSongs.hash === "" || Options.queue.indexOfPath(Options.lastSongs.hash) !== -1) return
        Options.queue.append({ name: Options.lastSongs.name, path: Options.lastSongs.hash, songer: Options.lastSongs.artist, source: Options.lastSongs.source })
        Options.warned("已加入播放列表", 1)
    }

    // 每日推荐：数据走私人雷达
    function openDaily(): void {
        MusicApi.personalRadar.clear()
        MusicApi.getPersonalRadar(1, 20, MusicApi.songSource)
        dailyWindow.opened("每日推荐", "qrc:/QueMusic/resources/app/musicpic.png")
        Options.exitIndex = 1
    }

    // 分类歌单：index 是 getHotlistMenu 下标，歌单漫游卡与窗口 tabbar 共用
    function openCategory(index: int): void {
        const menu = MusicApi.getHotlistMenu
        if (index < 0 || index >= menu.count) {
            Options.warned("歌单分类还在加载，稍后再试", 1)
            return
        }
        const titles = []
        for (let i = 0; i < menu.count; ++i) titles.push(menu.get(i).title)
        roamTitles = titles
        roamIndex = index

        const tag = menu.get(index)
        MusicApi.globaltagid = tag.tagid
        MusicApi.musicPlaylists.clear()
        MusicApi.getMusicPlaylists(tag.tagid, 1, 20)
        recommendWindow.opened(tag.title, (tag.cover || "").replace("{size}", "256") || "qrc:/QueMusic/resources/app/musicpic.png")
        Options.exitIndex = 1
    }

    function openHotList(row: var): void {
        hotlistsWindow.mainTarget = homeMain
        MusicApi.playlistSong.clear()
        MusicApi.globalid = row.hash
        MusicApi.getPlaylistSongs(row.hash, 1, 20)
        hotlistsWindow.opened(row.model)
        Options.exitIndex = 1
    }

    Component.onCompleted: {
        if (Options.homeLoaded) return
        MusicApi.getHotlistMenu.clear()
        MusicApi.getHotPlaylistMenu(3)
        MusicApi.getHotPlaylists(1)
        Options.homeLoaded = true
    }
    
    Item {
        id: homeMain
        anchors.fill: parent
        // 顶部标题 + 平台切换
        Item {
            id: homeTopBar
            x: homePage.pagePadding
            y: homePage.pagePadding
            width: homeView.standWidth
            height: 40
            z: 10

            Text {
                id: homeText
                anchors.verticalCenter: parent.verticalCenter
                text: homePage.greeting
                font.pixelSize: Style.pageTitle
                font.weight: Font.DemiBold
                color: Theme.fontColor
            }
            QDrop {
                width: 128
                height: 36
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                radius: 18
                cardRadius: Style.labelRadius
                choice: Math.max(0, MusicApi.sourceIds.indexOf(MusicApi.songSource))
                color: homePage.sourceSkin.tint
                textColor: homePage.sourceSkin.ink
                border.color: homePage.sourceSkin.line
                model: MusicApi.sourceNames
                onTransformed: (choiced) => {
                    MusicApi.songSource = MusicApi.sourceIdAt(choiced);
                    homeView.scrollTop();
                    MusicApi.hotPlayLists.clear();
                    MusicApi.getHotPlaylists(1);
                }
            }
        }

        QGridView {
            id: homeView
            x: 0
            y: 68
            width: homePage.width - 10
            height: homePage.height - 68
            readonly property int standWidth: homePage.width - 52
            // 列数与卡片宽度按页面留白均分：卡片首尾与 header 左右边界严格对齐，右侧不再留空
            readonly property int cardGutter: 20
            readonly property int columnCount: Math.max(2, Math.floor((standWidth + cardGutter) / (168 + cardGutter)))
            readonly property real cardWidth: (standWidth + cardGutter) / columnCount - cardGutter
            model: MusicApi.hotPlayLists
            cellWidth: (standWidth + cardGutter) / columnCount
            cellHeight: cardWidth + 122
            leftMargin: homePage.pagePadding
            // 末尾 0.5 是取整补偿：可用宽度略大于整列宽度，避免浮点误差少算一列
            rightMargin: homePage.pagePaddingRight - cardGutter - 10.5

            onAtYEndChanged: {
                if (atYEnd && !MusicApi.loadState && homeView.count !== 0) {
                    if (MusicApi.hotPlayLists.count % 20 === 0) {
                        MusicApi.getHotPlaylists(MusicApi.hotPlayLists.count / 20 + 1);
                    } else {
                        Options.warned("没有更多了",0);
                    }
                }
            }

            header: Column {
                id: homeContent
                spacing: 16
                height: implicitHeight + 16
                Rectangle {
                    width: homeView.standWidth
                    height: 48
                    radius: Style.cubeRadius
                    color: Theme.containColor
                    border.width: 1
                    border.color: Theme.themeColor
                    Text {
                        id: warnIcon
                        x: 24
                        anchors.verticalCenter: parent.verticalCenter
                        text: "\uf11a"
                        font.family: Fonts.icon
                        font.pixelSize: Style.textmain
                        color: Theme.themeColor
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        x: 56
                        text: "开发中 Beta 版本：部分功能尚未完善，稳定性欠佳"
                        color: Theme.fontColor
                        font.pixelSize: Style.text
                        elide: Text.ElideRight
                    }
                    SButton {
                        id: bannerClose
                        x: parent.width - 52
                        anchors.verticalCenter: parent.verticalCenter
                        width: 28
                        height: 28
                        radius: 14
                        iconCharacter: "\uf025"
                        iconSize: Style.textmain
                        buttonColor: "transparent"
                        hoverColor: Theme.hoverColor
                        shadowEnabled: false
                        tipText: "关闭提示"
                        onClicked: parent.visible = false
                    }
                }

                Item {
                    id: headerMain
                    width: homeView.standWidth
                    // 竖向布局或窗口偏窄时自动叠成三行
                    readonly property bool stacked: width < 700
                    readonly property real heroWidth: stacked ? width : width * 0.52 - 8
                    readonly property real sideWidth: stacked ? width : width * 0.48 - 8
                    readonly property int sideCardHeight: 76
                    height: stacked ? 200 + sideCardHeight * 2 : 168
                    onStackedChanged: {
                        if(stacked) {
                            homeView.scrollToY -= sideCardHeight * 2 + 32;
                            homeView.contentY -= sideCardHeight * 2 + 32;
                        }
                    }

                    QFloatCard {
                        id: dailyCard
                        x: 0
                        y: 0
                        width: headerMain.heroWidth
                        height: 168
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: Qt.rgba(Theme.containColor.r + 0.1, Theme.containColor.g + 0.1, Theme.containColor.b + 0.1, 1.0) }
                            GradientStop { position: 1.0; color: Theme.containColor }
                        }
                        border.width: 1
                        border.color: Theme.containColor
                        onClicked: homePage.openDaily()

                        Rectangle {
                            id: dailyBadge
                            x: 32
                            y: 32
                            width: 30
                            height: 30
                            radius: 9
                            color: Qt.rgba(Theme.themeColor.r, Theme.themeColor.g, Theme.themeColor.b, 0.22)
                            Text {
                                anchors.centerIn: parent
                                text: "\uf109"
                                font.family: Fonts.icon
                                font.pixelSize: Style.texticon
                                color: Theme.themeColor
                            }
                        }

                        Text {
                            x: 72
                            anchors.verticalCenter: dailyBadge.verticalCenter
                            text: "每日推荐"
                            color: Theme.fontColor
                            font.pixelSize: Style.textH1
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Text {
                            x: 32
                            y: 72
                            text: "每天推荐的新发行流行歌曲"
                            color: Theme.textColor
                            font.pixelSize: Style.text
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }

                        QButton {
                            x: 32
                            y: 108
                            height: 30
                            radius: 15
                            iconCharacter: "\uf0e7"
                            text: "去听听"
                            shadowEnabled: false
                            buttonColor: Theme.themeColor
                            textColor: Theme.fullColor
                            iconColor: Theme.fullColor
                            onClicked: homePage.openDaily()
                        }

                        Rectangle {
                            x: dailyCard.width - 142
                            y: 26
                            width: 116
                            height: 116
                            radius: Style.cubeRadius
                            color: Theme.fullColor
                            QPicture {
                                anchors.fill: parent
                                anchors.margins: 10
                                radius: Style.labelRadius
                                sourceSize: Qt.size(192, 192)
                                source: "qrc:/QueMusic/resources/app/rainbowMusicIcon.png"
                            }
                            RectangularShadow {
                                anchors.fill: parent
                                z: -1
                                offset.y: 8
                                radius: Style.cubeRadius
                                blur: 24
                                color: Theme.shadowColor
                            }
                        }
                    }

                    QFloatCard {
                        id: lastPlayedCard
                        x: headerMain.stacked ? 0 : headerMain.heroWidth + 16
                        y: headerMain.stacked ? 184 : 0
                        width: headerMain.sideWidth
                        height: headerMain.sideCardHeight
                        onClicked: homePage.playLastSong()

                        Text {
                            anchors.centerIn: parent
                            visible: Options.lastSongs.name === ""
                            text: "还没有播放记录"
                            color: Theme.textColor
                            font.pixelSize: Style.text
                        }

                        QPicture {
                            id: lastCover
                            x: 14
                            y: 14
                            width: 48
                            height: 48
                            radius: Style.labelRadius
                            sourceSize: Qt.size(64, 64)
                            source: Options.lastSongs.cover || "qrc:/QueMusic/resources/app/musicpic.png"
                        }

                        Text {
                            text: "继续播放"
                            x: 76
                            y: 14
                            color: Theme.themeColor
                            font.pixelSize: Style.textTip
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                        }
                        Text {
                            width: parent.width - 160
                            x: 76
                            y: 29
                            text: Options.lastSongs.name
                            color: Theme.fontColor
                            font.pixelSize: Style.textmain
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width - 160
                            x: 76
                            y: 48
                            text: Options.lastSongs.artist
                            color: Theme.textColor
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: Style.textTip
                            elide: Text.ElideRight
                        }

                        SButton {
                            id: lastPlayButton
                            x: lastPlayedCard.width - 88
                            anchors.verticalCenter: parent.verticalCenter
                            width: 32
                            height: 32
                            radius: 16
                            iconCharacter: "\uf00e"
                            iconSize: Style.textmain
                            iconColor: Theme.fullColor
                            buttonColor: Theme.themeColor
                            hoverColor: Qt.rgba(Theme.themeColor.r, Theme.themeColor.g, Theme.themeColor.b, 0.82)
                            shadowEnabled: false
                            tipText: "继续播放"
                            onClicked: homePage.playLastSong()
                        }
                        SButton {
                            id: lastQueueButton
                            x: lastPlayedCard.width - 46
                            anchors.verticalCenter: parent.verticalCenter
                            width: 32
                            height: 32
                            radius: 16
                            iconCharacter: "\uf095"
                            iconSize: Style.textmain
                            buttonColor: Theme.sideColor
                            hoverColor: Theme.hoverColor
                            shadowEnabled: false
                            tipText: "加入播放列表"
                            onClicked: homePage.queueLastSong()
                        }
                    }

                    // 歌单漫游
                    QFloatCard {
                        id: fmCard
                        x: headerMain.stacked ? 0 : headerMain.heroWidth + 16
                        y: headerMain.stacked ? 276 : 92
                        width: headerMain.sideWidth
                        height: headerMain.sideCardHeight
                        onClicked: homePage.openCategory(homePage.roamIndex)

                        Rectangle {
                            id: fmBadge
                            x: 14
                            y: 14
                            width: 48
                            height: 48
                            radius: Style.labelRadius
                            color: Qt.rgba(Theme.themeColor.r, Theme.themeColor.g, Theme.themeColor.b, 0.12)
                            Text {
                                anchors.centerIn: parent
                                text: "\uf098"
                                font.family: Fonts.icon
                                font.pixelSize: Style.texticonH
                                color: Theme.themeColor
                            }
                        }
                        Text {
                            x: 76
                            y: 15
                            height: 24
                            width: parent.width - 100
                            text: "歌单漫游"
                            color: Theme.fontColor
                            font.pixelSize: Style.textmain
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                        Text {
                            x: 76
                            y: 39
                            height: 21
                            width: parent.width - 100
                            text: "全网热门歌单分类快速预览"
                            color: Theme.textColor
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: Style.text
                            elide: Text.ElideRight
                        }
                        Text {
                            id: fmArrow
                            x: fmCard.width - 32
                            anchors.verticalCenter: parent.verticalCenter
                            text: "\uf0e7"
                            font.family: Fonts.icon
                            font.pixelSize: Style.textmain
                            color: Theme.textColor
                        }
                    }
                }


                QHead { text: "热门歌单" }

            }

            delegate: Rectangle {
                id: hotPlayRow
                required property string hash
                required property string cover
                required property string title
                required property string album
                required property real playcount
                required property real duration
                required property var model

                readonly property bool hovered: hotPlayListsArea.containsMouse
                readonly property string playcountText: hotPlayRow.playcount >= 10000
                    ? (hotPlayRow.playcount / 10000).toFixed(1) + "万"
                    : Math.round(hotPlayRow.playcount).toString()
                readonly property string durationText: hotPlayRow.duration > 0 ? Math.round(hotPlayRow.duration) + " 首" : ""

                // 封面为正方形，高度随列宽自适应；106 = 标题 10+38 + 歌手 + 间距 + 底栏 30
                width: homeView.cardWidth
                height: width + 106
                radius: Style.labelRadius
                color: Style.backmode === 0 ? Theme.primaryColor : Theme.primaryBlurColor
                Accessible.role: Accessible.ListItem
                Accessible.name: hotPlayRow.title
                Accessible.description: hotPlayRow.album

                // 悬停只做位移，不改 scale/blur，避免整块卡片重绘
                transform: Translate {
                    y: hotPlayRow.hovered ? -4 : 0
                    Behavior on y { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                }
                RectangularShadow {
                    anchors.fill: parent
                    z: -1
                    offset.x: 0
                    offset.y: 6
                    radius: Style.labelRadius
                    blur: 18
                    spread: 0
                    color: Theme.shadowColor
                }
                MouseArea {
                    id: hotPlayListsArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: homePage.openHotList(hotPlayRow)
                }
                QPicture {
                    id: hotPlayCover
                    width: parent.width
                    height: width
                    source: (hotPlayRow.cover || "").replace("{size}", "256")
                    sourceSize: Qt.size(256, 256)
                    radius: Style.labelRadius
                    radius3: 0
                    radius4: 0
                }
                // 悬停遮罩：淡入 + 播放图标轻微放大
                Rectangle {
                    anchors.fill: hotPlayCover
                    topLeftRadius: Style.labelRadius
                    topRightRadius: Style.labelRadius
                    color: "#30000000"
                    opacity: hotPlayRow.hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 160 } }
                }
                // 标题固定占两行，同一行卡片的信息区始终对齐
                Text {
                    id: hotPlayTitle
                    width: hotPlayRow.width - 24
                    x: 12
                    y: hotPlayCover.height + 10
                    height: 38
                    text: hotPlayRow.title
                    font.bold: true
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignTop
                }
                Text {
                    width: hotPlayRow.width - 24
                    x: 12
                    anchors.top: hotPlayTitle.bottom
                    text: hotPlayRow.album
                    color: Theme.textColor
                    font.pixelSize: Style.text
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
                // 底栏：播放量 + 曲目数
                Rectangle {
                    width: hotPlayRow.width
                    anchors.bottom: parent.bottom
                    height: 30
                    color: Theme.secondaryColor
                    bottomLeftRadius: Style.labelRadius
                    bottomRightRadius: Style.labelRadius
                    Text {
                        x: 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: "\uf0cb"
                        font.family: Fonts.icon
                        font.pixelSize: Style.textTip
                        color: Theme.textColor
                    }
                    Text {
                        x: 32
                        anchors.verticalCenter: parent.verticalCenter
                        text: hotPlayRow.playcountText
                        font.pixelSize: Style.textTip
                        color: Theme.textColor
                    }
                    Text {
                        x: parent.width - dtText.width - 32
                        anchors.verticalCenter: parent.verticalCenter
                        text: "\uf098"
                        font.family: Fonts.icon
                        font.pixelSize: Style.textTip
                        color: Theme.textColor
                    }
                    Text {
                        id: dtText
                        x: parent.width - width - 12
                        anchors.verticalCenter: parent.verticalCenter
                        text: hotPlayRow.durationText
                        font.pixelSize: Style.textTip
                        color: Theme.textColor
                    }
                }
                // 悬停描边画在最上层，不会被封面盖住
                Rectangle {
                    anchors.fill: parent
                    radius: Style.labelRadius
                    color: "transparent"
                    border.width: 2
                    border.color: Theme.themeColor
                    opacity: hotPlayRow.hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 160 } }
                }
            }
        }
    }

    // 每日推荐
    AnimatorWindow {
        id: dailyWindow
        mainTarget: homeMain
        haveControl: false
        content: QListView {
            id: dailyView
            x: 24
            y: 128
            width: dailyWindow.width - 32
            height: dailyWindow.height - 128
            model: MusicApi.personalRadar
            onCountChanged: if (count === 0) scrollTop()   // 重新打开时回到顶部
            clip: true
            topMargin: 8
            bottomMargin: 24

            onClicked: (index) => {
                const song = model.get(index)
                const hash = Config.soundQuality === 0 ? song.hash : Config.soundQuality === 1 ? song.hashhq : song.hashsq
                MusicApi.getMusicInfo(hash)
            }

            // 私人雷达只有一页（page > 1 直接回空表），不翻页
            onEnded: isEnd = true
        }
    }

    AnimatorWindow {
        id: recommendWindow
        mainTarget: homeMain
        haveControl: false
        content: QListView {
            id: recomView
            x: 24
            y: 128
            width: recommendWindow.width - 32
            height: recommendWindow.height - 128
            model: MusicApi.musicPlaylists
            onCountChanged: if (count === 0) scrollTop()   // 换分类清空后回到顶部
            clip: true
            topMargin: 56   // 给上方分类 tabbar 让位
            bottomMargin: 24
            isList: true

            QBlurTapBar {
                x: 0
                y: 10
                z: 12
                model: homePage.roamTitles
                tabWidth: 100
                width: model.length * 100 + 6
                onTabChange: (index) => homePage.openCategory(index)
            }

            onEnded: {
                if(MusicApi.musicPlaylists.count % 20 === 0 && MusicApi.musicPlaylists.count !== 0) {
                    MusicApi.getMusicPlaylists(MusicApi.globaltagid,MusicApi.musicPlaylists.count / 20 + 1,20);
                    isEnd = false;
                } else {
                    if(MusicApi.musicPlaylists.count !== 0) {
                        isEnd = true;
                    }
                }
            }

            onClicked: (index) => {
                hotlistsWindow.mainTarget = recommendWindow;
                MusicApi.playlistSong.clear();
                MusicApi.globalid = model.get(index).hash;
                MusicApi.getPlaylistSongs(model.get(index).hash,1,20);
                hotlistsWindow.opened(model.get(index));
                Options.exitIndex = 2;
            }
        }
    }
    PlayListWindow {
        id: hotlistsWindow
        mainTarget: homeMain
        winIndex: 2
        content: Item {

            QListView {
                id: hotlistsView
                x: 24
                y: 170
                width: hotlistsWindow.width - 32
                height: hotlistsWindow.height - 170
                model: MusicApi.playlistSong
                clip: true
                topMargin: 8
                bottomMargin: 24

                onClicked: (index) => {
                               if(Config.soundQuality === 0) {
                                   MusicApi.getMusicInfo(model.get(index).hash);
                               } else if(Config.soundQuality === 1) {
                                   MusicApi.getMusicInfo(model.get(index).hashhq);
                               } else {
                                   MusicApi.getMusicInfo(model.get(index).hashsq);
                               }
                           }

                onEnded: {
                    if(MusicApi.playlistSong.count % 20 === 0 && MusicApi.playlistSong.count !== 0) {
                        const tagid = hotlistsWindow.id;
                        MusicApi.getPlaylistSongs(tagid,MusicApi.playlistSong.count / 20 + 1,20);
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
}
