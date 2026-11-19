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

    readonly property int pagePadding: 24
    readonly property int pagePaddingRight: 28

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
        if (Config.lastSongHash === "") return
        Playback.playItem(Config.lastSongName, Config.lastSongHash, Config.lastSongArtist, Config.lastSongSource)
    }

    function queueLastSong(): void {
        if (Config.lastSongHash === "") return
        if (Playback.enqueue(Config.lastSongName, Config.lastSongHash, Config.lastSongArtist, Config.lastSongSource))
            Options.warned("已加入播放列表", 1)
    }

    // 每日推荐
    function openDaily(): void {
        MusicApi.personalRadar.clear()
        MusicApi.getPersonalRadar(1, 20, MusicApi.songSource)
        dailyWindow.opened("每日推荐", "")
        Options.exitIndex = 1
    }

    // 分类歌单
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
        recommendWindow.opened("歌单漫游", "") //tag.title, (tag.cover || "").replace("{size}", "256")
        Options.exitIndex = 1
    }

    function reloadSource(): void {
        homeView.scrollTop()
        MusicApi.hotPlayLists.clear()
        MusicApi.recommendPlaylists.clear()
        MusicApi.getHotlistMenu.clear()
        MusicApi.getHotPlaylistMenu(3)
        MusicApi.getHotPlaylists(1)
        MusicApi.getRecommendPlaylists(1, 20)
        roamTitles = []
        roamIndex = 0
    }

    Connections {
        target: MusicApi
        function onSongSourceChanged() {
            if (Options.homeLoaded)
                homePage.reloadSource()
        }
    }

    Component.onCompleted: {
        if (Options.homeLoaded) return
        MusicApi.getHotlistMenu.clear()
        MusicApi.getHotPlaylistMenu(3)
        MusicApi.getHotPlaylists(1)
        MusicApi.getRecommendPlaylists(1, 20)
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
                radius: 18
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                cardRadius: Style.labelRadius
                choice: Math.max(0, MusicApi.sourceIds.indexOf(MusicApi.songSource))
                model: MusicApi.sourceNames
                onTransformed: (choiced) => {
                    MusicApi.songSource = MusicApi.sourceIdAt(choiced);
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
            model: MusicApi.recommendPlaylists
            cellWidth: (standWidth + cardGutter) / columnCount
            cellHeight: cardWidth + 122
            leftMargin: homePage.pagePadding
            // 末尾 0.5 是取整补偿：可用宽度略大于整列宽度，避免浮点误差少算一列
            rightMargin: homePage.pagePaddingRight - cardGutter - 10.5

            onAtYEndChanged: {
                if (atYEnd && !MusicApi.loadState && homeView.count !== 0) {
                    if (MusicApi.recommendPlaylists.count % 20 === 0) {
                        MusicApi.getRecommendPlaylists(MusicApi.recommendPlaylists.count / 20 + 1);
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
                        tipText: "关闭提示"
                        onClicked: parent.visible = false
                    }
                }

                Item {
                    id: headerMain
                    width: homeView.standWidth
                    // 竖向布局或窗口偏窄时自动叠成三行
                    readonly property real heroWidth: width * 0.52 - 8
                    readonly property real sideWidth: width * 0.48 - 8
                    readonly property int sideCardHeight: 76
                    height: 168

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
                                image: "qrc:/QueMusic/resources/app/rainbowMusicIcon.jpg"
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

                    // 热门歌单
                    QFloatCard {
                        id: hotlistCard
                        x: headerMain.heroWidth + 16
                        y: 0
                        width: headerMain.sideWidth
                        height: headerMain.sideCardHeight
                        onClicked: {
                            MusicApi.hotPlayLists.clear();
                            MusicApi.getHotPlaylists(1);
                            hotPlayWindow.opened("热门歌单", "");
                        }

                        Rectangle {
                            id: hotlistBadge
                            x: 14
                            y: 14
                            width: 48
                            height: 48
                            radius: Style.labelRadius
                            color: Qt.rgba(Theme.themeColor.r, Theme.themeColor.g, Theme.themeColor.b, 0.12)
                            Text {
                                anchors.centerIn: parent
                                text: "\uf0d5"
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
                            text: "热门歌单"
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
                            text: "全网热门歌单集合"
                            color: Theme.textColor
                            verticalAlignment: Text.AlignVCenter
                            font.pixelSize: Style.text
                            elide: Text.ElideRight
                        }
                        Text {
                            id: hotlistArrow
                            x: hotlistCard.width - 32
                            anchors.verticalCenter: parent.verticalCenter
                            text: "\uf0e7"
                            font.family: Fonts.icon
                            font.pixelSize: Style.textmain
                            color: Theme.textColor
                        }
                    }

                    // 歌单漫游
                    QFloatCard {
                        id: fmCard
                        x: headerMain.heroWidth + 16
                        y: 92
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
                            text: "各大热门分类歌单快速查看"
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


                QHead { text: "推荐歌单" }

            }

            Rectangle {
                id: recomplayMask
                width: homeView.cardWidth
                height: homeView.cardWidth
                radius: Style.labelRadius
                bottomLeftRadius: 0
                bottomRightRadius: 0
                color: "#000000"
                layer.enabled: true
                visible: false
            }

            delegate: Rectangle {
                id: recomPlayRow
                required property string hash
                required property string cover
                required property string title
                required property string album
                required property real playcount
                required property real duration
                required property var model

                readonly property string playcountText: recomPlayRow.playcount >= 10000 ? (recomPlayRow.playcount / 10000).toFixed(1) + "万" : Math.round(recomPlayRow.playcount).toString()
                readonly property string durationText: recomPlayRow.duration > 0 ? Math.round(recomPlayRow.duration) + " 首" : ""

                // 封面为正方形，高度随列宽自适应；106 = 标题 10+38 + 歌手 + 间距 + 底栏 30
                width: homeView.cardWidth
                height: width + 106
                radius: Style.labelRadius
                color: Options.cardColor
                Accessible.role: Accessible.ListItem
                Accessible.name: recomPlayRow.title
                Accessible.description: recomPlayRow.album

                // 悬停只做位移，不改 scale/blur，避免整块卡片重绘
                transform: Translate {
                    y: recomPlayListsArea.hovered ? -4 : 0
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
                TapHandler {
                    onTapped: {
                        hotlistsWindow.mainTarget = homeMain
                        MusicApi.playlistSong.clear()
                        MusicApi.globalid = recomPlayRow.hash
                        MusicApi.getPlaylistSongs(recomPlayRow.hash, 1, 20)
                        hotlistsWindow.opened(recomPlayRow.model)
                        Options.exitIndex = 1
                    }
                }
                HoverHandler {
                    id: recomPlayListsArea
                    cursorShape: Qt.PointingHandCursor
                }
                MultiEffect {
                    id: recomPlayCover
                    source: recomPlayImage.status === Image.Ready ? recomPlayImage : Options.musicpic
                    Image {
                        id: recomPlayImage
                        source: (recomPlayRow.cover || "").replace("{size}", "256")
                        width: recomPlayRow.width
                        height: width
                        sourceSize: Qt.size(256,256)
                        cache: Config.picCache
                        fillMode: Image.PreserveAspectCrop
                        visible: false
                    }
                    width: recomPlayRow.width
                    height: width
                    maskEnabled: true
                    maskSource: recomplayMask
                    // 边缘抗锯齿
                    maskThresholdMin: 0.5
                    maskSpreadAtMin: 1.0
                }
                // 悬停遮罩：淡入 + 播放图标轻微放大
                Rectangle {
                    anchors.fill: recomPlayCover
                    topLeftRadius: Style.labelRadius
                    topRightRadius: Style.labelRadius
                    color: "#30000000"
                    opacity: recomPlayListsArea.hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 160 } }
                }
                // 标题固定占两行，同一行卡片的信息区始终对齐
                Text {
                    id: recomPlayTitle
                    width: recomPlayRow.width - 24
                    x: 12
                    y: recomPlayCover.height + 10
                    height: 38
                    text: recomPlayRow.title
                    font.bold: true
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignTop
                }
                Text {
                    width: recomPlayRow.width - 24
                    x: 12
                    anchors.top: recomPlayTitle.bottom
                    text: recomPlayRow.album
                    color: Theme.textColor
                    font.pixelSize: Style.text
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
                // 底栏：播放量 + 曲目数
                Rectangle {
                    width: recomPlayRow.width
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
                        text: recomPlayRow.playcountText
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
                        text: recomPlayRow.durationText
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
                    opacity: recomPlayListsArea.hovered ? 1 : 0
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

            onClicked: (index) => MusicApi.playSong(model.get(index))

            // 私人雷达只有一页（page > 1 直接回空表），不翻页
            onEnded: isEnd = true
        }
    }

    AnimatorWindow {
        id: recommendWindow
        mainTarget: homeMain
        haveControl: false
        mainHeadMode: true
        QBlurTapBar {
            x: 24
            y: 80
            z: 2
            model: homePage.roamTitles
            rectXy: Qt.rect(0, 12, width, 40)
            blurSource: recommendWindow.loadItem
            tabWidth: 100
            width: model.length * 100 + 6
            onTabChange: (index) => homePage.openCategory(index)
        }

        content: QListView {
            id: recomView
            x: 24
            y: 68
            width: recommendWindow.width - 32
            height: recommendWindow.height - 68
            model: MusicApi.musicPlaylists
            onCountChanged: if (count === 0) scrollTop()   // 换分类清空后回到顶部
            clip: true
            topMargin: 72   // 给上方分类 tabbar 让位
            bottomMargin: 24
            isList: true

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
    AnimatorWindow {
        id: hotPlayWindow
        mainTarget: homeMain
        haveControl: false
        mainHeadMode: true

        content: QGridView {
            id: hotPlayView
            x: 0
            y: 68
            width: hotPlayWindow.width - 10
            height: hotPlayWindow.height - 68
            readonly property int standWidth: hotPlayWindow.width - 52
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
                if (atYEnd && !MusicApi.loadState && hotPlayView.count !== 0) {
                    if (MusicApi.hotPlayLists.count % 20 === 0) {
                        MusicApi.getHotPlaylists(MusicApi.hotPlayLists.count / 20 + 1);
                    } else {
                        Options.warned("没有更多了",0);
                    }
                }
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

                readonly property string playcountText: hotPlayRow.playcount >= 10000 ? (hotPlayRow.playcount / 10000).toFixed(1) + "万" : Math.round(hotPlayRow.playcount).toString()
                readonly property string durationText: hotPlayRow.duration > 0 ? Math.round(hotPlayRow.duration) + " 首" : ""

                // 封面为正方形，高度随列宽自适应；106 = 标题 10+38 + 歌手 + 间距 + 底栏 30
                width: homeView.cardWidth
                height: width + 106
                radius: Style.labelRadius
                color: Options.cardColor
                Accessible.role: Accessible.ListItem
                Accessible.name: hotPlayRow.title
                Accessible.description: hotPlayRow.album

                // 悬停只做位移，不改 scale/blur，避免整块卡片重绘
                transform: Translate {
                    y: hotPlayListsArea.hovered ? -4 : 0
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
                TapHandler {
                    onTapped: {
                        hotlistsWindow.mainTarget = hotPlayWindow
                        MusicApi.playlistSong.clear()
                        MusicApi.globalid = hotPlayRow.hash
                        MusicApi.getPlaylistSongs(hotPlayRow.hash, 1, 20)
                        hotlistsWindow.opened(hotPlayRow.model)
                        Options.exitIndex = 2
                    }
                }
                HoverHandler {
                    id: hotPlayListsArea
                    cursorShape: Qt.PointingHandCursor
                }
                MultiEffect {
                    id: hotPlayCover
                    source: hotPlayImage.status === Image.Ready ? hotPlayImage : Options.musicpic
                    Image {
                        id: hotPlayImage
                        source: (hotPlayRow.cover || "").replace("{size}", "256")
                        width: hotPlayRow.width
                        height: width
                        sourceSize: Qt.size(256,256)
                        cache: Config.picCache
                        fillMode: Image.PreserveAspectCrop
                        visible: false
                    }
                    width: hotPlayRow.width
                    height: width
                    maskEnabled: true
                    maskSource: recomplayMask
                    // 边缘抗锯齿
                    maskThresholdMin: 0.5
                    maskSpreadAtMin: 1.0
                }
                // 悬停遮罩：淡入 + 播放图标轻微放大
                Rectangle {
                    anchors.fill: hotPlayCover
                    topLeftRadius: Style.labelRadius
                    topRightRadius: Style.labelRadius
                    color: "#30000000"
                    opacity: hotPlayListsArea.hovered ? 1 : 0
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
                        x: parent.width - dt2Text.width - 32
                        anchors.verticalCenter: parent.verticalCenter
                        text: "\uf098"
                        font.family: Fonts.icon
                        font.pixelSize: Style.textTip
                        color: Theme.textColor
                    }
                    Text {
                        id: dt2Text
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
                    opacity: hotPlayListsArea.hovered ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: 160 } }
                }
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

                onClicked: (index) => MusicApi.playSong(model.get(index))

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
