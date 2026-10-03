// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import QueMusic 1.0
Item {
    id: filePage

    // 宿主注入：播放引擎不再靠上下文继承访问宿主的局部 id
    readonly property AudioEngine player: Playback.player

    property int folderNumber: 0
    property int setMode: 0
    property var chooseIndex: []
    property int localSortField: FolderListModel.Unsorted
    property bool localSortReversed: false
    property var localSortOptions: [
        { label: "默认顺序", field: FolderListModel.Unsorted, desc: false },
        { label: "文件名 A→Z", field: FolderListModel.Name, desc: false },
        { label: "文件名 Z→A", field: FolderListModel.Name, desc: true },
        { label: "修改时间 旧→新", field: FolderListModel.Time, desc: false },
        { label: "修改时间 新→旧", field: FolderListModel.Time, desc: true },
        { label: "大小 小→大", field: FolderListModel.Size, desc: false },
        { label: "大小 大→小", field: FolderListModel.Size, desc: true }
    ]
    readonly property int localSortMenuIndex: {
        for (let i = 0; i < localSortOptions.length; i++)
            if (localSortOptions[i].field === localSortField && localSortOptions[i].desc === localSortReversed) return i
        return 0
    }
    // 右键当前行：由 delegate 在弹菜单前写入，避免按 index 反查排序代理
    property var menuRow: ({})

    // 移入回收站；结果由 onDeleteFinished 提示
    function deleteLocalFile(path: string, title: string): void {
        Options.dialog.openSimpleDialog("删除本地文件", "这将把「" + title + "」移入回收站，是否继续？",
            function() { localFileModel.deleteFiles([path]) })
    }

    function toggleChoose(key: var): void {
        filePage.chooseIndex = filePage.chooseIndex.indexOf(key) === -1
            ? filePage.chooseIndex.concat([key])
            : filePage.chooseIndex.filter(value => value !== key);
    }

    function clearChoose(): void {
        filePage.chooseIndex = [];
        filePage.setMode = 0;
    }

    // 导入在 DB 线程执行，数量由信号回报
    Connections {
        target: Songs
        function onSongsAdded(count: int): void {
            if (count > 0)
                Style.warned("已导入 " + count + " 首音乐", 1);
        }
    }

    function chooseTotal(): int {
        if (filePage.setMode === 3) return folderMusic.searching ? Songs.searchResults.count : Songs.rowCount();
        if (filePage.setMode === 4) return localFolderMusic.searching ? localFileModel.searchResults.count : localFileModel.count;
        return 0;
    }

    function isAllChosen(): bool {
        const total = filePage.chooseTotal();
        return total > 0 && filePage.chooseIndex.length >= total;
    }

    function toggleAllChoose(): void {
        if (filePage.isAllChosen()) {
            filePage.chooseIndex = [];
            return;
        }
        const all = [];
        let i = 0;
        if (filePage.setMode === 3) {
            if (folderMusic.searching) {
                for (i = 0; i < Songs.searchResults.count; i++) all.push(Songs.searchResults.getRow(i).songId);
            } else {
                for (i = 0; i < Songs.rowCount(); i++) all.push(Songs.get(i).songId);
            }
        } else if (filePage.setMode === 4) {
            if (localFolderMusic.searching) {
                for (i = 0; i < localFileModel.searchResults.count; i++) all.push(localFileModel.searchResults.getRow(i).fileUrl.toString());
            } else {
                for (i = 0; i < localFileModel.count; i++) all.push(localFileModel.get(i, "fileUrl").toString());
            }
        }
        filePage.chooseIndex = all;
    }

    function addChosenToList(): void {
        const count = filePage.chooseIndex.length;
        if (count === 0) {
            Style.warned("请先选择歌曲", 0);
            return;
        }
        const rows = [];
        if (filePage.setMode === 3) {
            for (let i = 0; i < Songs.rowCount(); i++) {
                const song = Songs.get(i);
                if (!song || !song.path || filePage.chooseIndex.indexOf(song.songId) === -1) continue;
                rows.push({ name: song.name, path: song.path, songer: song.singer || "", source: -1 });
            }
        } else if (filePage.setMode === 4) {
            for (let j = 0; j < localFileModel.count; j++) {
                const path = localFileModel.get(j, "fileUrl").toString();
                if (filePage.chooseIndex.indexOf(path) === -1) continue;
                // 标题/歌手由 LocalMusicScanner 后台解析，不再同步开 TagLib
                const title = localFileModel.get(j, "title") || "";
                const artist = localFileModel.get(j, "artist") || "";
                rows.push({ name: title || localFileModel.get(j, "fileName"), path: path, songer: artist, source: -1 });
            }
        }
        // 一次批量追加，逐条 append 会触发 N 次插入与信号
        const added = rows.length > 0 ? Options.queue.appendBatch(rows) : 0;
        Style.warned(added === 0 ? "所选歌曲都已在播放列表中" : "已加入播放列表 " + added + " 首", added === 0 ? 0 : 1);
    }

    function deleteChosen(): void {
        const count = filePage.chooseIndex.length;
        if (count === 0) {
            Style.warned(filePage.setMode < 3 ? "请先选择文件夹" : "请先选择歌曲", 0);
            return;
        }

        // 一次性批量处理：逐个调用会每删一条整表 reset + 全量重跑 TAG
        const keys = [];
        for (let i = 0; i < count; i++)
            keys.push(filePage.chooseIndex[i]);

        switch (filePage.setMode) {
        case 1:
            MyFolders.deleteFolders(keys);
            break;
        case 2:
            LocalFolders.deleteFolders(keys);
            break;
        case 3:
            Songs.deleteSongs(keys);
            break;
        case 4:
            // 移入回收站较慢，交给工作线程执行并显示进度，结果由 onDeleteFinished 提示
            deleteProgressDialog.processed = 0;
            deleteProgressDialog.total = keys.length;
            deleteProgressDialog.open();
            localFileModel.deleteFiles(keys);
            filePage.chooseIndex = [];
            return;
        default:
            return;
        }

        filePage.chooseIndex = [];
        Style.warned("已删除 " + count + (filePage.setMode < 3 ? " 个文件夹" : " 首音乐"), 1);
    }

    // 把「我的文件夹」歌曲模型里的歌全部加入播放列表，play=true 时立即播放
    function addAllSongModelToList(play: bool): void {
        const searching = folderMusic.searching;
        const total = searching ? Songs.searchResults.count : Songs.rowCount();
        if (total === 0) {
            Style.warned("当前文件夹没有歌曲", 0);
            return;
        }
        const rows = [];
        for (let i = 0; i < total; i++) {
            const item = searching ? Songs.searchResults.getRow(i) : Songs.get(i);
            if (!item || !item.name || !item.path) continue;
            rows.push({ name: item.tagTitle || item.name, path: item.path, songer: item.tagArtist || item.singer || "", source: -1 });
        }
        // 批量接口内部去重，并只发一次插入信号
        const added = rows.length > 0 ? Options.queue.appendBatch(rows) : 0;
        if (added === 0) {
            Style.warned("列表中的歌曲都已在播放列表中", 0);
        } else {
            Style.warned("已加入播放列表 " + added + " 首", 1);
        }
        if (play && added > 0) {
            Playback.goTo(Options.queue.count - added);
        }
    }

    // 把「本地文件夹」里扫描到的音频文件全部加入播放列表，play=true 时立即播放
    function addAllLocalFilesToList(play: bool): void {
        const searching = localFolderMusic.searching;
        const total = searching ? localFileModel.searchResults.count : localFileModel.count;
        if (total === 0) {
            Style.warned("当前文件夹没有音频文件", 0);
            return;
        }
        const rows = [];
        for (let i = 0; i < total; i++) {
            const row = searching ? localFileModel.searchResults.getRow(i) : null;
            const name = row ? (row.title || row.name) : localFileModel.get(i, "fileName");
            const path = row ? row.fileUrl.toString() : localFileModel.get(i, "fileUrl").toString();
            if (!name || !path) continue;
            rows.push({ name: name, path: path, songer: row ? (row.artist || "") : "", source: -1 });
        }
        const added = rows.length > 0 ? Options.queue.appendBatch(rows) : 0;
        if (added === 0) {
            Style.warned("列表中的歌曲都已在播放列表中", 0);
        } else {
            Style.warned("已加入播放列表 " + added + " 首", 1);
        }
        if (play && added > 0) {
            Playback.goTo(Options.queue.count - added);
        }
    }

    // 打开某个歌曲文件的所在文件夹（本地浏览器）
    function openSongFolder(songPath: string): void {
        let p = songPath || "";
        if (p.startsWith("file:///"))
            p = p.substring(8);
        const idx = Math.max(p.lastIndexOf('/'), p.lastIndexOf('\\'));
        const dir = idx > 0 ? p.substring(0, idx) : p;
        if (dir) {
            Qt.openUrlExternally(dir);
        } else {
            Style.warned("无法定位所在文件夹", 0);
        }
    }

    // 首页面
    Item {
        id: fileMain
        x: 0
        y: 0
        width: filePage.width
        height: filePage.height
        visible: true

        // 顶部标题
        Item {
            x: 24
            y: 24
            height: 40
            width: fileMain.width - 48
            z: 10
            Text {
                x: 0
                y: 0
                height: 40
                verticalAlignment: Text.AlignVCenter
                text: "本地音乐"
                font.weight: Font.DemiBold
                font.pixelSize: Style.settings.pageTitle
                color: Style.fontColor
            }
        }

        QBlurTapBar {
            x: 24
            y: 80
            z: 5
            model: ["我的文件夹","本地文件夹","WebDAV"]
            tabWidth: 110
            width: 336
            rectXy: Qt.rect(0, 12, 358, 40)
            blurSource: fileChildPage
            onTabChange: (index) => {
                fileChildPage.stack(index);
                filePage.clearChoose();
            }
        }

        QPages {
            x: 24
            y: 68
            width: fileMain.width - 32
            height: fileMain.height - 68
            id: fileChildPage
            pageList: [myFile,localFile,webDav]
            // WebDAV（云端网盘目录）
            WebDavPage {
                id: webDav
                width: fileChildPage.width
                height: fileChildPage.height
                visible: false
            }
            // 我的文件夹
            Item {
                id: myFile
                width: fileChildPage.width
                height: fileChildPage.height
                visible: true

                // 右侧操作区
                Row {
                    x: parent.width - width - 16
                    y: 11
                    z: 2
                    spacing: 8
                    QButton {
                        height: 38
                        text: filePage.setMode === 1 ? "取消选择" : "选择"
                        iconCharacter: "\uf09f"
                        buttonColor: filePage.setMode === 1 ? Style.containColor : Style.primaryColor
                        onClicked: {
                            if(filePage.setMode === 1) {
                                filePage.clearChoose();
                            } else {
                                filePage.setMode = 1;
                            }
                        }
                    }
                    // 添加
                    QButton {
                        height: 38
                        text: "新建文件夹"
                        iconCharacter: "\uf0f8"
                        QAlertDialog {
                            id: dialog
                            title: "新建文件夹"
                            message: "为文件夹设定一个名称："
                            isInput: true
                            onConfirm: {
                                if(input!=="") {
                                    MyFolders.addFolder(input, "my", "");
                                    Style.warned("已添加文件夹", 1);
                                } else {
                                    Style.warned("请输入文件名",0);
                                }
                            }
                        }
                        onClicked: dialog.open()
                    }
                }

                QLocalView {
                    id: folderView
                    anchors.fill: parent
                    model: MyFolders
                    clip: true
                    topMargin: 60
                    headerModel: ["标题","","","菜单"]
                    function openFilePage(title: string, image: string): void {
                        folderMusic.opened(title,image)
                    }
                    rebound: Transition {
                        NumberAnimation {
                            properties: "y"
                            duration: 480
                            easing.type: Easing.Bezier
                            easing.bezierCurve: [ 0.32, 0.12, 0.00, 1.00, 1, 1 ]
                        }
                    }
                    QAlertDialog {
                        id: editDialog
                        title: "重命名"
                        message: "为文件夹重新命名新名称："
                        isInput: true
                        property int folderId
                        onConfirm: {
                            if(input!=="") {
                                MyFolders.renameFolder(editDialog.folderId, input);
                                Options.warn.tiped("文件夹已重命名", 1);
                            } else {
                                Options.warn.tiped("请输入文件名",0);
                            }
                        }
                    }
                    delegate: Rectangle {
                        required property int folderId
                        required property string name
                        required property string path
                        required property int index
                        id: listfolder
                        height: 64
                        width: folderView.width - 16
                        radius: Style.settings.labelRadius
                        property bool chosen: filePage.setMode === 1 && filePage.chooseIndex.indexOf(listfolder.folderId) !== -1
                        color: listfolder.chosen ? Style.containColor : "#00000000"

                        Rectangle {
                            anchors.fill: parent
                            radius: Style.settings.labelRadius
                            color: Style.hoverColor
                            opacity: foldArea.containsMouse ? 1 : 0
                            z: 1
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                        }

                        Rectangle {
                            y: 8
                            x: 8
                            z: 4
                            width: 48
                            height: 48
                            color: Style.containColor
                            radius: 10
                            Text {
                                anchors.fill: parent
                                text: "\uf0f5"
                                font.family: IconFont.name
                                font.pixelSize: Style.settings.texticon
                                color: Style.fontColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }


                        Label {
                            x: 80
                            y: 0
                            z: 3
                            width: 140
                            height: 64
                            text: listfolder.name
                            color: Style.fontColor
                            font.bold: true
                            font.pixelSize: Style.settings.textmain
                            verticalAlignment: Text.AlignVCenter
                            visible: true
                            Behavior on color { ColorAnimation { duration: 120 } }
                        }

                        MouseArea {
                            id: foldArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if(filePage.setMode === 1) {
                                    filePage.toggleChoose(listfolder.folderId);
                                } else {
                                    filePage.folderNumber = listfolder.index;
                                    Options.exitIndex = 1;
                                    Songs.folderId = listfolder.folderId;
                                    Songs.clearSearch();
                                    folderMusic.searching = false;
                                    folderView.openFilePage(listfolder.name,"");
                                }
                            }
                            Row {
                                anchors.right: parent.right
                                anchors.rightMargin: 20
                                spacing: 2
                                z: 2
                                y: 12
                                height: 36
                                SButton {
                                    iconCharacter: "\uf050"
                                    width: 36
                                    height: 36
                                    radius: 18
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                    shadowEnabled: false
                                    tipText: listfolder.path ? "打开文件夹位置" : "应用逻辑文件夹（无磁盘路径）"
                                    onClicked: {
                                        if (listfolder.path) {
                                            Qt.openUrlExternally(listfolder.path);
                                        } else {
                                            Style.warned("「我的文件夹」没有关联的磁盘路径", 0);
                                        }
                                    }
                                }
                                SButton {
                                    iconCharacter: "\uf005"
                                    width: 36
                                    height: 36
                                    radius: 18
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                    shadowEnabled: false
                                    tipText: "重命名文件夹"

                                    onClicked: {
                                        if(listfolder.folderId !== 1) {
                                            editDialog.input = listfolder.name;
                                            editDialog.folderId = listfolder.folderId;
                                            editDialog.open();
                                        } else {
                                            Style.warned("无法修改默认文件夹名称",0);
                                        }
                                    }
                                }
                                SButton {
                                    iconCharacter: "\uf08e"
                                    width: 36
                                    height: 36
                                    radius: 18
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                    shadowEnabled: false
                                    tipText: "删除文件夹"
                                    onClicked: {
                                        if(listfolder.folderId !== 1) {
                                            Options.dialog.openSimpleDialog("删除", "这将删除本文件夹，无法恢复，是否删除？",
                                                function() {
                                                    MyFolders.deleteFolder(listfolder.folderId);
                                                    Style.warned("已删除「我的文件夹」", 1);
                                                }
                                            );
                                        } else {
                                            Style.warned("无法删除默认文件夹",0);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 本地文件夹
            Item {
                id: localFile
                width: fileChildPage.width
                height: fileChildPage.height
                visible: false
                //用于存储本地文件夹目录
                //用于存放文件夹内显示音频文件
                LocalMusicScanner {
                    id: localFileModel
                    nameFilters: ["*.mp3","*.wav","*.aac","*.flac","*.ogg","*.eac3","*.wma","*.ac3","*.alac","*.mkv","*.wmv","*.avi","*.mpeg4","*.m4a"]
                    showDirs: false
                    sortField: filePage.localSortField
                    sortReversed: filePage.localSortReversed
                    onScanFinished: function(count) {
                        if (count > 0)
                            Options.warn.tiped("已加载 " + count + " 个音频文件", 1);
                    }
                }

                FolderDialog {
                    id: folderDialog
                    title: "选择音乐的文件夹"
                    onAccepted: {
                        // 获取选中的文件夹URL（file:// 格式）
                        const folderUrl = folderDialog.selectedFolder;
                        const folderPath = folderUrl.toString();
                        const folderName = folderPath.split('/').pop(); // 使用 '/' 分割，取最后一部分
                        LocalFolders.addFolder(folderName, "local", folderPath);
                        Options.warn.tiped("已定位本地文件夹", 1);

                    }
                }

                // 右侧操作区
                Row {
                    x: parent.width - width - 16
                    y: 11
                    z: 2
                    spacing: 8
                    QButton {
                        height: 38
                        text: filePage.setMode === 2 ? "取消选择" : "选择"
                        iconCharacter: "\uf09f"
                        buttonColor: filePage.setMode === 2 ? Style.containColor : Style.primaryColor
                        onClicked: {
                            if(filePage.setMode === 2) {
                                filePage.clearChoose();
                            } else {
                                filePage.setMode = 2;
                            }
                        }
                    }
                    // 添加
                    QButton {
                        height: 38
                        text: "导入目录"
                        iconCharacter: "\uf0f1"
                        onClicked: {
                            folderDialog.open();
                        }
                    }
                }

                QLocalView {
                    id: localFolderView
                    anchors.fill: parent
                    model: LocalFolders
                    clip: true
                    topMargin: 60
                    headerModel: ["标题","目录","","菜单"]

                    rebound: Transition {
                        NumberAnimation {
                            properties: "y"
                            duration: 480
                            easing.type: Easing.Bezier
                            easing.bezierCurve: [ 0.32, 0.12, 0.00, 1.00, 1, 1 ]
                        }
                    }
                    QAlertDialog {
                        id: editLocalDialog
                        title: "重命名"
                        message: "为文件夹重新命名新名称："
                        isInput: true
                        property int index
                        onConfirm: {
                            if(input!=="") {
                                LocalFolders.renameFolder(editLocalDialog.index, input);
                            } else {
                                Options.warn.opened("请输入文件名",0);
                            }
                        }
                    }
                    delegate: Rectangle {
                        required property int folderId
                        required property string name
                        required property string path
                        id: listLocalfolder
                        height: 64
                        width: localFolderView.width - 16
                        radius: Style.settings.labelRadius
                        property bool chosen: filePage.setMode === 2 && filePage.chooseIndex.indexOf(listLocalfolder.folderId) !== -1
                        color: listLocalfolder.chosen ? Style.containColor : "#00000000"

                        Rectangle {
                            anchors.fill: parent
                            radius: Style.settings.labelRadius
                            color: Style.hoverColor
                            opacity: foldersArea.containsMouse ? 1 : 0
                            z: 1
                            Behavior on opacity { NumberAnimation { duration: 80 } }
                        }

                        Rectangle {
                            y: 8
                            x: 8
                            z: 4
                            width: 48
                            height: 48
                            color: Style.containColor
                            radius: 10
                            Text {
                                anchors.fill: parent
                                text: "\uf0f5"
                                font.family: IconFont.name
                                font.pixelSize: Style.settings.texticon
                                color: Style.fontColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }


                        Label {
                            x: 80
                            z: 3
                            width: parent.width / 2 - 108
                            height: 64
                            text: listLocalfolder.name
                            color: Style.fontColor
                            font.bold: true
                            elide: Text.ElideRight
                            font.pixelSize: Style.settings.textmain
                            verticalAlignment: Text.AlignVCenter
                        }
                        Label {
                            height: 60
                            z: 2
                            x: parent.width / 2 - 24
                            width: parent.width / 2 - 120
                            text: listLocalfolder.path.substring(8)
                            color: Style.textColor
                            font.pixelSize: Style.settings.textTip
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                        }

                        MouseArea {
                            id: foldersArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if(filePage.setMode === 2) {
                                    filePage.toggleChoose(listLocalfolder.folderId);
                                } else {
                                    localFileModel.folder = listLocalfolder.path;
                                    Options.exitIndex = 1
                                    localFileModel.clearSearch();
                                    localFolderMusic.searching = false;
                                    localFolderMusic.opened(listLocalfolder.name,"");
                                }
                            }

                            Row {
                                anchors.right: parent.right
                                anchors.rightMargin: 20
                                spacing: 2
                                z: 2
                                y: 12
                                height: 36
                                SButton {
                                    iconCharacter: "\uf050"
                                    width: 36
                                    height: 36
                                    radius: 18
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                    shadowEnabled: false
                                    tipText: "打开文件夹位置"
                                    onClicked: {
                                        if (listLocalfolder.path) {
                                            Qt.openUrlExternally(listLocalfolder.path);
                                        } else {
                                            Style.warned("无法定位文件夹", 0);
                                        }
                                    }
                                }
                                SButton {
                                    iconCharacter: "\uf005"
                                    width: 36
                                    height: 36
                                    radius: 18
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                    shadowEnabled: false
                                    tipText: "重命名文件夹"

                                    onClicked: {
                                        editLocalDialog.input = listLocalfolder.name
                                        editLocalDialog.index = listLocalfolder.folderId
                                        editLocalDialog.open()
                                    }
                                }
                                SButton {
                                    iconCharacter: "\uf08e"
                                    width: 36
                                    height: 36
                                    radius: 18
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                    shadowEnabled: false
                                    tipText: "删除文件夹"
                                    onClicked: {
                                        Options.dialog.openSimpleDialog("删除", "这将移除本文件夹，是否删除？",
                                            function() {
                                                LocalFolders.deleteFolder(listLocalfolder.folderId);
                                                Style.warned("已移除本地文件夹", 1);
                                            }
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    AnimatorWindow {
        id: folderMusic
        mainTarget: fileMain
        winIndex: 1
        property bool searching: false

        content: Item {
            anchors.fill: parent

            FileDialog {
                id: musicfileDialog
                title: "选择音乐文件"
                fileMode: FileDialog.OpenFiles
                nameFilters: ["音频文件 (*.mp3 *.wav *.aac *.flac *.ogg *.eac3 *.wma *.ac3 *.alac *.mkv *.wmv *.avi *.mpeg4 *.m4a)"]
                onAccepted: {
                    // 获取选中的文件URL（file:// 格式）
                    const fileUrls = musicfileDialog.selectedFiles;

                    // 先批量转换为本地路径，再一次交给 C++ 侧事务写入，
                    // 避免上千首歌曲重复打开DB/刷新列表导致界面假死。
                    const importList = [];
                    for (let i = 0; i < fileUrls.length; i++) {
                        let filePath = fileUrls[i].toString();
                        if (filePath.startsWith("file:///")) {
                            filePath = filePath.substring(8);// 去前8字符：file:///
                        }
                        const fileName = filePath.split('/').pop(); // 使用 '/' 分割，取最后一部分
                        if (fileName && filePath) {
                            importList.push({ name: fileName, path: filePath, singer: "" });
                        }
                    }

                    if (importList.length > 0)
                        Songs.addSongs(Songs.folderId, importList);
                }
                onRejected: {
                    console.log("操作取消");
                }
            }

            QSortModel {
                id: songSort
                model: Songs
                options: [
                    { label: "默认顺序", mode: 0, desc: false },
                    { label: "文件名 A→Z", mode: 1, desc: false },
                    { label: "文件名 Z→A", mode: 1, desc: true }
                ]
                nameRole: "name"
            }

            QSortModel {
                id: songSearchSort
                model: Songs.searchResults
                options: songSort.options
                nameRole: "name"
            }

            QMenu {
                id: songSortMenu
                model: songSort.options.map(o => o.label)
                current: songSort.menuIndex
                onClicked: (i) => {
                    songSort.selectMenu(i);
                    songSearchSort.selectMenu(i);
                    fileView.scrollTop();
                }
            }

            // 顶栏
            Row {
                x: 136
                y: 76
                height: 36
                spacing: 6
                QButton {
                    height: 36; width: 96
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf00e"
                    text: "播放"
                    shadowEnabled: false
                    buttonColor: Style.sideColor
                    tipText: "播放当前文件夹全部歌曲"
                    onClicked: {
                        filePage.addAllSongModelToList(true);
                    }
                }
                SButton {
                    width: 36
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf095"
                    shadowEnabled: false
                    buttonColor: Style.sideColor
                    tipText: "全部加入播放列表"
                    onClicked: {
                        filePage.addAllSongModelToList(false);
                    }
                }
                SButton {
                    width: 36
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf10c"
                    shadowEnabled: false
                    buttonColor: Style.sideColor
                    tipText: "重新解析标签并刷新"
                    onClicked: {
                        Songs.clearSearch();
                        folderMusic.searching = false;
                        filterInput1.text = "";
                        // 丢弃 TAG 缓存重读（普通刷新只读库）
                        Songs.rescanTags();
                        fileView.scrollTop();
                        Style.warned("已刷新当前列表", 1);
                    }
                }
                SButton {
                    id: songSortBtn
                    width: 48
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf10b"
                    shadowEnabled: false
                    buttonColor: Style.sideColor
                    tipText: "排序方式（再次点击反向）"
                    onClicked: songSortMenu.popup(songSortBtn, 0, songSortBtn.height + 6)
                }
            }

            Row {
                x: folderMusic.width - width - 24
                y: 26
                height: 40
                spacing: 8
                z: 2
                QButton {
                    height: 40
                    radius: 20
                    text: filePage.setMode === 3 ? "取消选择" : "选择"
                    iconCharacter: "\uf09f"
                    buttonColor: filePage.setMode === 3 ? Style.containColor : Style.primaryColor
                    onClicked: {
                        if(filePage.setMode === 3) {
                            filePage.clearChoose();
                        } else {
                            filePage.setMode = 3;
                        }
                    }
                }
                QButton {
                    height: 40; width: 100
                    radius: 20
                    iconCharacter: "\uf10d"
                    text: "导入"
                    onClicked: {
                        musicfileDialog.open();
                    }
                }
            }

            TextField {
                id: filterInput1
                x: folderMusic.width - width - 24
                y: 76
                z: 3
                width: 200
                height: 36
                leftPadding: 12
                rightPadding: 38
                placeholderText: "搜索与过滤"
                placeholderTextColor: Style.textColor
                color: Style.textColor
                font.pixelSize: Style.settings.text
                verticalAlignment: Text.AlignVCenter
                selectionColor: Style.containColor
                onTextChanged: filterDebounce1.restart()
                background: Rectangle {
                    radius: Style.settings.labelRadius
                    color: Style.primaryColor
                    border.width: 2
                    border.color: filterInput1.focus ? Style.themeColor : Style.sideColor
                }
                SButton {
                    visible: filterInput1.text !== ""
                    y: 2
                    x: parent.width - 34
                    width: 32
                    height: 32
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf025"
                    iconSize: 15
                    buttonColor: "transparent"
                    shadowEnabled: false
                    onClicked: {
                        filterInput1.text = "";
                        Songs.clearSearch();
                        folderMusic.searching = false;
                    }
                }
                Timer {
                    id: filterDebounce1
                    interval: 250
                    onTriggered: {
                        const t = filterInput1.text.trim();
                        if (t === "") {
                            Songs.clearSearch();
                            folderMusic.searching = false;
                        } else {
                            folderMusic.searching = true;
                            Songs.startSearch(t);
                        }
                    }
                }
            }

            QLocalView {
                id: fileView
                x: 24
                y: 128
                width: folderMusic.width - 32
                height: folderMusic.height - 128
                model: folderMusic.searching ? songSearchSort : songSort
                clip: true
                reuseItems: false
                property int transY: 0
                transform: Translate { y: fileView.transY }
                populate: Transition {
                    id: localFileLoadAnime2
                    enabled: Style.settings.premiumAnime
                    SequentialAnimation {
                        NumberAnimation {
                            properties: "opacity"
                            from: 0
                            to: 0
                            duration: Math.min(localFileLoadAnime2.ViewTransition.index, 12) * 40
                        }
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
                }
                // 0 立即播放 / 1 下一首播放 / 2 加入列表 / 3 音频详情 / 4 编辑元数据 / 5 从文件夹移除
                onMenuClicked: (index, choice) => {
                    const r = filePage.menuRow
                    if (!r || !r.path) return
                    const item = { name: r.title, path: r.path, songer: r.artist, source: -1 }
                    if (choice === 0) {
                        Playback.playItem(item)
                    } else if (choice === 1) {
                        Playback.playNext(item)
                        Style.warned("已设为下一首播放", 1)
                    } else if (choice === 2) {
                        const added = Playback.enqueue(item)
                        Style.warned(added ? "已加入播放列表" : "已在播放列表中", added ? 1 : 0)
                    } else if (choice === 3) {
                        audioInfoDialog.showInfo(r.path)
                    } else if (choice === 4) {
                        metaDialog.edit(r.path, 0)
                    } else if (choice === 5) {
                        Songs.deleteSong(r.songId)
                        Style.warned("已从文件夹移除", 1)
                    }
                }
                delegate: Rectangle {
                    required property int songId
                    required property string name
                    required property string path
                    required property string singer
                    required property string tagTitle
                    required property string tagArtist
                    required property string tagCoverUrl
                    required property int index
                    id: listfile
                    height: 60
                    width: fileView.width - 16
                    radius: Style.settings.labelRadius
                    property bool chosen: filePage.setMode === 3 && filePage.chooseIndex.indexOf(listfile.songId) !== -1
                    color: listfile.chosen || Playback.player.source == listfile.path ? Style.containColor : "transparent"
                    property int transY: 0
                    transform: Translate { y: listfile.transY }

                    ListView.onReused: { listfile.transY = 0; listfile.opacity = 1; }
                    ListView.onPooled: { listfile.transY = 0; listfile.opacity = 1; }

                    readonly property string coverUrl: {
                        if (listfile.tagCoverUrl !== "") return listfile.tagCoverUrl;
                        if (!listfile.path) return "qrc:/QueMusic/resources/app/musicpic.png";
                        return MusicApi.readLocalCoverHint(listfile.path) || "qrc:/QueMusic/resources/app/musicpic.png";
                    }
                    property string songTitle: listfile.tagTitle || listfile.name
                    property string artistName: listfile.tagArtist || listfile.singer || ""

                    Behavior on color { ColorAnimation { duration: 120 } }

                    QPicture {
                        y: 8
                        x: 8
                        z: 4
                        width: 44
                        height: 44
                        source: listfile.coverUrl
                        radius1: 10
                        radius2: 10
                        radius3: 10
                        radius4: 10
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Style.hoverColor
                        opacity: fileArea.containsMouse ? 1 : 0
                        z: 1
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }

                    Label {
                        height: 60
                        x: 80
                        z: 3
                        width: parent.width / 2 - 108
                        text: listfile.songTitle
                        color: Style.fontColor
                        font.bold: true
                        font.pixelSize: Style.settings.textmain
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    Label {
                        height: 60
                        x: parent.width / 2 - 24
                        width: parent.width / 2 - 120
                        z: 2
                        visible: listfile.artistName !== ""
                        text: listfile.artistName
                        color: Style.textColor
                        font.pixelSize: Style.settings.textTip
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: fileArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.LeftButton) {
                                if(filePage.setMode === 3) {
                                    filePage.toggleChoose(listfile.songId);
                                    return;
                                }
                                Playback.playLocalSong(listfile.path, listfile.songTitle);
                                const musicName = listfile.songTitle;
                                const musicPath = listfile.path;
                                const listIndex = Options.queue.indexOfName(musicName);
                                if (listIndex == -1) {
                                    Options.queue.append({ name: musicName, path: musicPath, songer: listfile.artistName, source: -1 });
                                    Options.queue.playListIndex = Options.queue.count - 1;
                                }
                            } else {
                                fileView.menu.index = index;
                                filePage.menuRow = { path: listfile.path, title: listfile.songTitle,
                                                     artist: listfile.artistName, songId: listfile.songId };
                                fileView.menu.popup();
                            }
                            forceActiveFocus();
                        }
                        Row {
                            anchors.right: parent.right
                            anchors.rightMargin: 16
                            spacing: 2
                            y: 12
                            height: 36
                            SButton {
                                id: fileListAdd
                                iconCharacter: "\uf095"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                tipText: "加入播放列表"
                                onClicked: {
                                    const musicName = listfile.songTitle;
                                    const musicPath = listfile.path;
                                    const listIndex = Options.queue.indexOfName(musicName);
                                    if (listIndex == -1) {
                                        Options.queue.append({ name: musicName, path: musicPath, songer: listfile.artistName, source: -1 });
                                        Style.warned("已加入播放列表", 1);
                                    }
                                }
                            }
                            SButton {
                                id: fileOpen
                                iconCharacter: "\uf107"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                tipText: "打开所在文件夹"
                                onClicked: {
                                    filePage.openSongFolder(listfile.path);
                                }
                            }
                            SButton {
                                id: fileDelete
                                iconCharacter: "\uf08e"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                shadowEnabled: false
                                tipText: "从当前文件夹移除"
                                onClicked: {
                                    Songs.deleteSong(listfile.songId);
                                    Style.warned("已从文件夹移除", 1);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    AnimatorWindow {
        id: localFolderMusic
        mainTarget: fileMain
        winIndex: 1
        property bool searching: false

        content: Item {
            anchors.fill: parent

            QMenu {
                id: localSortMenu
                model: localSortOptions.map(o => o.label)
                current: filePage.localSortMenuIndex
                onClicked: (i) => {
                    filePage.localSortField = localSortOptions[i].field;
                    filePage.localSortReversed = localSortOptions[i].desc;
                    localFileView.scrollTop();
                }
            }

            // 顶栏
            Row {
                x: 136
                y: 76
                height: 36
                spacing: 6
                QButton {
                    height: 36; width: 96
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf00e"
                    text: "播放"
                    shadowEnabled: false
                    buttonColor: Style.sideColor
                    tipText: "播放当前文件夹全部歌曲"
                    onClicked: {
                        filePage.addAllLocalFilesToList(true);
                    }
                }
                SButton {
                    width: 36
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf095"
                    shadowEnabled: false
                    buttonColor: Style.sideColor
                    tipText: "全部加入播放列表"
                    onClicked: {
                        filePage.addAllLocalFilesToList(false);
                    }
                }
                SButton {
                    width: 36
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf10c"
                    shadowEnabled: false
                    buttonColor: Style.sideColor
                    tipText: "刷新当前文件夹"
                    onClicked: {
                        localFileModel.clearSearch();
                        localFolderMusic.searching = false;
                        filterInput2.text = "";
                        localFileModel.rescan();
                        localFileView.scrollTop();
                        Style.warned("已刷新当前列表", 1);
                    }
                }
                SButton {
                    id: localSortBtn
                    width: 48
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf10b"
                    shadowEnabled: false
                    buttonColor: Style.sideColor
                    tipText: "排序方式（再次点击反向）"
                    onClicked: localSortMenu.popup(localSortBtn, 0, localSortBtn.height + 6)
                }
            }

            Row {
                x: localFolderMusic.width - width - 24
                y: 26
                height: 40
                spacing: 8
                z: 2
                QButton {
                    height: 40
                    radius: 20
                    text: filePage.setMode === 4 ? "取消选择" : "选择"
                    iconCharacter: "\uf09f"
                    buttonColor: filePage.setMode === 4 ? Style.containColor : Style.primaryColor
                    onClicked: {
                        if(filePage.setMode === 4) {
                            filePage.clearChoose();
                        } else {
                            filePage.setMode = 4;
                        }
                    }
                }
                QButton {
                    height: 40
                    radius: 20
                    text: "文件夹中显示"
                    iconCharacter: "\uf0fb"
                    onClicked: {
                        Qt.openUrlExternally(localFileModel.folder);
                    }
                }
            }

            TextField {
                id: filterInput2
                x: localFolderMusic.width - width - 24
                y: 76
                z: 3
                width: 200
                height: 36
                leftPadding: 12
                rightPadding: 38
                placeholderText: "搜索与过滤"
                placeholderTextColor: Style.textColor
                color: Style.textColor
                font.pixelSize: Style.settings.text
                verticalAlignment: Text.AlignVCenter
                selectionColor: Style.containColor
                onTextChanged: filterDebounce2.restart()
                background: Rectangle {
                    radius: Style.settings.labelRadius
                    color: Style.primaryColor
                    border.width: 2
                    border.color: filterInput2.focus ? Style.themeColor : Style.sideColor
                }
                SButton {
                    visible: filterInput2.text !== ""
                    y: 2
                    x: parent.width - 34
                    width: 32
                    height: 32
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf025"
                    iconSize: 15
                    buttonColor: "transparent"
                    shadowEnabled: false
                    onClicked: {
                        filterInput2.text = "";
                        localFileModel.clearSearch();
                        localFolderMusic.searching = false;
                    }
                }
                Timer {
                    id: filterDebounce2
                    interval: 250
                    onTriggered: {
                        const t = filterInput2.text.trim();
                        if (t === "") {
                            localFileModel.clearSearch();
                            localFolderMusic.searching = false;
                        } else {
                            localFolderMusic.searching = true;
                            localFileModel.startSearch(t);
                        }
                    }
                }
            }

            QLocalView {
                id: localFileView
                x: 24
                y: 128
                width: localFolderMusic.width - 32
                height: localFolderMusic.height - 128
                model: localFolderMusic.searching ? localFileModel.searchResults : localFileModel
                clip: true
                reuseItems: false
                populate: Transition {
                    id: localFileLoadAnime
                    enabled: Style.settings.premiumAnime
                    SequentialAnimation {
                        NumberAnimation {
                            properties: "opacity"
                            from: 0
                            to: 0
                            duration: Math.min(localFileLoadAnime.ViewTransition.index, 12) * 40
                        }
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
                }
                // 0 立即播放 / 1 下一首播放 / 2 加入列表 / 3 音频详情 / 4 编辑元数据 / 5 移入回收站
                onMenuClicked: (index, choice) => {
                    const r = filePage.menuRow
                    if (!r || !r.path) return
                    const item = { name: r.title, path: r.path, songer: r.artist, source: -1 }
                    if (choice === 0) {
                        Playback.playItem(item)
                    } else if (choice === 1) {
                        Playback.playNext(item)
                        Style.warned("已设为下一首播放", 1)
                    } else if (choice === 2) {
                        const added = Playback.enqueue(item)
                        Style.warned(added ? "已加入播放列表" : "已在播放列表中", added ? 1 : 0)
                    } else if (choice === 3) {
                        audioInfoDialog.showInfo(r.path)
                    } else if (choice === 4) {
                        metaDialog.edit(r.path, 1)
                    } else if (choice === 5) {
                        filePage.deleteLocalFile(r.path, r.title)
                    }
                }
                delegate: Rectangle {
                    required property url fileUrl
                    required property string fileName
                    required property string title
                    required property string artist
                    required property var model
                    required property int index
                    id: listLocalFile
                    height: 60
                    width: localFileView.width - 16
                    radius: Style.settings.labelRadius
                    property bool chosen: filePage.setMode === 4 && filePage.chooseIndex.indexOf(listLocalFile.fileUrl.toString()) !== -1
                    color: listLocalFile.chosen || Playback.player.source == listLocalFile.fileUrl ? Style.containColor : "transparent"
                    property int transY: 0
                    transform: Translate { y: listLocalFile.transY }

                    // reuseItems 下非 model 提供的属性不会随复用自动恢复，按官方建议手动复位
                    ListView.onReused: { listLocalFile.transY = 0; listLocalFile.opacity = 1; }
                    ListView.onPooled: { listLocalFile.transY = 0; listLocalFile.opacity = 1; }

                    readonly property string rowPath: listLocalFile.fileUrl ? listLocalFile.fileUrl.toString() : ""
                    readonly property string coverUrl: {
                        if (model.coverUrl !== "") return model.coverUrl;
                        if (!rowPath) return "qrc:/QueMusic/resources/app/musicpic.png";
                        return MusicApi.readLocalCoverHint(rowPath) || "qrc:/QueMusic/resources/app/musicpic.png";
                    }
                    property string songTitle: listLocalFile.title || listLocalFile.fileName
                    property string artistName: listLocalFile.artist || ""

                    Behavior on color { ColorAnimation { duration: 120 } }

                    QPicture {
                        y: 8
                        x: 8
                        z: 4
                        width: 44
                        height: 44
                        source: listLocalFile.coverUrl
                        radius1: 10
                        radius2: 10
                        radius3: 10
                        radius4: 10
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Style.hoverColor
                        opacity: localFileArea.containsMouse ? 1 : 0
                        z: 1
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }


                    Label {
                        height: 60
                        x: 80
                        z: 3
                        width: parent.width / 2 - 108
                        text: listLocalFile.songTitle
                        color: Style.fontColor
                        font.bold: true
                        font.pixelSize: Style.settings.textmain
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }
                    Label {
                        x: parent.width / 2 - 24
                        height: 60
                        z: 2
                        width: parent.width / 2 - 120
                        visible: listLocalFile.artistName !== ""
                        text: listLocalFile.artistName
                        color: Style.textColor
                        font.pixelSize: Style.settings.textTip
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: localFileArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: (mouse) => {
                            if (mouse.button === Qt.LeftButton) {
                                if(filePage.setMode === 4) {
                                    filePage.toggleChoose(listLocalFile.fileUrl.toString());
                                    return;
                                }
                                Playback.playLocalSong(listLocalFile.fileUrl.toString(), listLocalFile.songTitle);
                                const musicName = listLocalFile.songTitle;
                                const musicPath = listLocalFile.fileUrl.toString();
                                const listIndex = Options.queue.indexOfName(musicName);
                                if (listIndex == -1) {
                                    Options.queue.append({ name: musicName, path: musicPath, songer: listLocalFile.artistName, source: -1 });
                                    Options.queue.playListIndex = Options.queue.count - 1;
                                }
                            } else {
                                localFileView.menu.index = listLocalFile.index;
                                filePage.menuRow = { path: listLocalFile.fileUrl.toString(), title: listLocalFile.songTitle,
                                                     artist: listLocalFile.artistName, songId: -1 };
                                localFileView.menu.popup();
                            }
                            forceActiveFocus();
                        }
                        Row {
                            anchors.right: parent.right
                            anchors.rightMargin: 16
                            spacing: 2
                            y: 12
                            height: 36
                            SButton {
                                iconCharacter: "\uf095"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                tipText: "加入播放列表"
                                onClicked: {
                                    const musicName = listLocalFile.songTitle;
                                    const musicPath = listLocalFile.fileUrl.toString();
                                    const listIndex = Options.queue.indexOfName(musicName);
                                    if (listIndex == -1) {
                                        Options.queue.append({ name: musicName, path: musicPath, songer: listLocalFile.artistName, source: -1 });
                                    }
                                }
                            }
                            SButton {
                                iconCharacter: "\uf107"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                tipText: "打开所在文件夹"
                                onClicked: {
                                    filePage.openSongFolder(listLocalFile.fileUrl.toString());
                                }
                            }
                            SButton {
                                iconCharacter: "\uf08e"
                                width: 36
                                height: 36
                                radius: 18
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                shadowEnabled: false
                                tipText: "从本地文件夹移除（移入回收站）"
                                onClicked: filePage.deleteLocalFile(listLocalFile.fileUrl.toString(), listLocalFile.fileName)
                            }
                        }
                    }
                }
            }
        }
    }

    // 批量删除进度提示
    Popup {
        id: deleteProgressDialog
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose
        width: 380
        height: deleteContentCol.implicitHeight + 40
        property int processed: 0
        property int total: 0

        background: Rectangle {
            color: Style.primaryColor
            radius: Style.settings.cubeRadius
            border.width: 1
            border.color: Style.sideColor
        }

        contentItem: Column {
            id: deleteContentCol
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Text {
                text: "正在移入回收站"
                font.pixelSize: 18
                font.bold: true
                color: Style.fontColor
            }

            Text {
                text: deleteProgressDialog.total > 0
                      ? "已处理 " + deleteProgressDialog.processed + " / " + deleteProgressDialog.total
                      : "正在准备…"
                font.pixelSize: 13
                color: Style.fontColor
            }

            Rectangle {
                width: parent.width
                height: 8
                radius: 4
                color: Style.sideColor

                Rectangle {
                    width: parent.width * (deleteProgressDialog.total > 0
                                           ? Math.min(1, deleteProgressDialog.processed / deleteProgressDialog.total)
                                           : 0)
                    height: parent.height
                    radius: 4
                    color: Style.themeColor
                    Behavior on width { NumberAnimation { duration: 120 } }
                }
            }

            Text {
                width: parent.width
                text: "删除过程中请勿关闭程序"
                font.pixelSize: 12
                color: Style.fontColor
                opacity: 0.7
            }
        }
    }

    Connections {
        target: localFileModel
        function onDeleteProgress(processed: int, total: int): void {
            deleteProgressDialog.processed = processed;
            deleteProgressDialog.total = total;
        }
        function onDeleteFinished(removed: int, failedCount: int): void {
            deleteProgressDialog.close();
            if (failedCount > 0)
                Style.warned("已移入回收站 " + removed + " 个，失败 " + failedCount + " 个", 0);
            else
                Style.warned("已将 " + removed + " 个文件移入回收站", 1);
        }
    }

    Connections {
        target: folderMusic
        function onVisibleChanged(): void {
            if (!folderMusic.visible) filePage.clearChoose();
        }
    }
    Connections {
        target: localFolderMusic
        function onVisibleChanged(): void {
            if (!localFolderMusic.visible) filePage.clearChoose();
        }
    }

    // 选择模式
    Rectangle {
        id: chooseArea
        x: 0
        y: filePage.height - 60
        opacity: visible ? 1 : 0
        width: filePage.width
        height: 60
        z: 21
        visible: filePage.setMode !== 0
        gradient: Gradient {
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 1.0; color: Style.sideColor }
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
            buttonColor: filePage.isAllChosen() ? Style.themeColor : Style.fullColor
            textColor: filePage.isAllChosen() ? Style.primaryColor : Style.fontColor
            text: "全选"
            visible: filePage.setMode >= 3
            onClicked: filePage.toggleAllChoose()
        }
        Rectangle {
            x: 118
            y: 12
            width: 92
            height: 36
            radius: 20
            color: "transparent"//Style.fullColor
            Text {
                anchors.centerIn: parent
                text: "已选择:" + filePage.chooseIndex.length + "项"
                color: Style.textColor
                font.pixelSize: Style.settings.textmain
            }
        }
        QButton {
            y: 12
            x: chooseArea.width - 348
            shadowEnabled: false
            width: 132
            height: 36
            radius: 20
            borderWidth: 1
            buttonColor: Style.themeColor
            textColor: Style.primaryColor
            text: "加入播放列表"
            visible: filePage.setMode >= 3
            onClicked: filePage.addChosenToList()
        }
        QButton {
            y: 12
            x: chooseArea.width - 208
            shadowEnabled: false
            width: 92
            height: 36
            radius: 20
            buttonColor: "#fa4642"
            textColor: Style.primaryColor
            text: "删除"
            borderWidth: 1
            onClicked: {
                const tip = filePage.setMode === 4 ? "这将把这些文件移入回收站，是否删除？"
                        : filePage.setMode === 3 ? "这将从文件夹移除这些歌曲，无法恢复，是否删除？"
                        : "这将删除这些文件夹，无法恢复，是否删除？";
                Options.dialog.openSimpleDialog("删除", tip,
                    function() {
                        filePage.deleteChosen();
                    }
                );
            }
        }
        QButton {
            y: 12
            x: chooseArea.width - 108
            shadowEnabled: false
            width: 92
            height: 36
            radius: 20
            buttonColor: Style.themeColor
            textColor: Style.primaryColor
            borderWidth: 1
            text: "完成"
            onClicked: filePage.clearChoose()
        }
    }

    // 音频详情：TagLib 只读表头，同步调用
    QOptionDialog {
        id: audioInfoDialog
        title: "音频详情"
        cancelText: ""
        confirmText: "关闭"
        property var rows: []

        function showInfo(path: string): void {
            const info = MusicApi.readLocalAudioInfo(path)
            const rows = []
            const add = (label, value) => {
                const text = value === undefined || value === null ? "" : String(value)
                if (text !== "" && text !== "0") rows.push({ label: label, value: text })
            }
            add("标题", info.title)
            add("歌手", info.artist)
            add("专辑", info.album)
            add("流派", info.genre)
            add("年份", info.year)
            add("音轨号", info.track)
            add("时长", info.duration > 0
                ? Math.floor(info.duration / 60) + ":" + String(info.duration % 60).padStart(2, "0") : "")
            add("格式", info.format)
            add("比特率", info.bitrate > 0 ? info.bitrate + " kbps" : "")
            add("采样率", info.sampleRate > 0 ? info.sampleRate + " Hz" : "")
            add("声道", info.channels)
            add("大小", info.size > 0 ? (info.size / 1048576).toFixed(2) + " MB" : "")
            add("文件名", info.fileName)
            audioInfoDialog.rows = rows
            audioInfoDialog.open()
        }

        options: Column {
            width: parent.width
            spacing: 8
            Repeater {
                model: audioInfoDialog.rows
                delegate: Row {
                    id: infoRow
                    required property string label
                    required property string value
                    width: parent.width
                    height: 24
                    spacing: 12
                    Text {
                        width: 68
                        height: parent.height
                        text: infoRow.label
                        color: Style.textColor
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        opacity: 0.65
                    }
                    Text {
                        width: parent.width - 80
                        height: parent.height
                        text: infoRow.value
                        color: Style.fontColor
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideMiddle
                    }
                }
            }
        }
    }

    // 编辑元数据：标签 + 封面 + 歌词，统一走 TagLib 写回
    QOptionDialog {
        id: metaDialog
        title: "编辑元数据"
        confirmText: "保存"
        cancelText: "取消"
        property string filePath: ""
        property int source: 0 // 0 我的文件夹 / 1 本地文件夹
        property var values: ({})
        property string coverPick: "" // 用户新选的封面图，空表示不修改
        // 打开时读一次；放在 binding 里每次重算都会同步开 TagLib
        property string embeddedCover: ""

        readonly property string coverPreview: {
            if (coverPick !== "")
                return coverPick
            if (embeddedCover !== "")
                return embeddedCover
            return "qrc:/QueMusic/resources/app/musicpic.png"
        }

        function edit(path: string, src: int): void {
            const info = MusicApi.readLocalAudioInfo(path)
            filePath = path
            source = src
            coverPick = ""
            embeddedCover = path !== "" ? (coverHelper.findEmbeddedCover(path) || "") : ""
            values = {
                title: info.title || info.fileName.replace(/\.[^.]+$/, ""),
                artist: info.artist || "",
                album: info.album || "",
                genre: info.genre || "",
                year: info.year > 0 ? String(info.year) : "",
                track: info.track > 0 ? String(info.track) : ""
            }
            lyricsArea.text = MusicApi.readLocalLyricsText(path)
            metaDialog.open()
        }

        onConfirm: {
            values.lyrics = lyricsArea.text
            values.cover = coverPick
            MusicApi.writeLocalMetadata(filePath, values)
        }

        options: Column {
            width: parent.width
            spacing: 12
            Row {
                width: parent.width
                height: 96
                spacing: 12
                QPicture {
                    width: 96
                    height: 96
                    source: metaDialog.coverPreview
                    radius1: 10
                    radius2: 10
                    radius3: 10
                    radius4: 10
                }
                Column {
                    width: parent.width - 108
                    spacing: 8
                    Text {
                        text: "封面"
                        color: Style.textColor
                        font.pixelSize: Style.settings.textmain
                        opacity: 0.65
                    }
                    QButton {
                        width: 108
                        height: 32
                        radius: 16
                        text: "选择图片"
                        onClicked: coverDialog.open()
                    }
                }
            }
            Repeater {
                model: [
                    { label: "标题", key: "title" },
                    { label: "歌手", key: "artist" },
                    { label: "专辑", key: "album" },
                    { label: "流派", key: "genre" },
                    { label: "年份", key: "year" },
                    { label: "音轨号", key: "track" }
                ]
                delegate: Row {
                    id: metaRow
                    required property string key
                    required property string label
                    width: parent.width
                    height: 36
                    spacing: 12
                    Text {
                        width: 56
                        height: parent.height
                        text: metaRow.label
                        color: Style.textColor
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        opacity: 0.65
                    }
                    QInput {
                        width: parent.width - 68
                        height: 36
                        inputText: metaDialog.values[metaRow.key] !== undefined
                                   ? String(metaDialog.values[metaRow.key]) : ""
                        onInputTextChanged: metaDialog.values[metaRow.key] = inputText
                    }
                }
            }
            Text {
                text: "歌词（可带 [mm:ss.xx] 时间轴）"
                color: Style.textColor
                font.pixelSize: Style.settings.textmain
                opacity: 0.65
            }
            TextArea {
                id: lyricsArea
                width: parent.width
                color: Style.fontColor
                font.pixelSize: Style.settings.textmain
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                background: Rectangle {
                    radius: Style.settings.labelRadius
                    color: Style.fullColor
                    border.width: 1
                    border.color: Style.sideColor
                }
            }
        }
    }

    FileDialog {
        id: coverDialog
        title: "选择封面图片"
        nameFilters: ["图片文件 (*.jpg *.jpeg *.png *.bmp *.gif)"]
        onAccepted: metaDialog.coverPick = selectedFile.toString()
    }

    Connections {
        target: MusicApi
        function onLocalMetadataSaved(filePath: string, ok: bool): void {
            if (!ok) {
                Style.warned("元数据保存失败，文件可能只读或格式不支持", 0);
                return;
            }
            Style.warned("元数据已保存", 1);
            // 只让改动的这首重读标签，其余沿用缓存
            if (metaDialog.source === 0)
                Songs.rescanTags(filePath);
            else
                localFileModel.rescan();
        }
    }
}
