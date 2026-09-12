// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import Qt.labs.folderlistmodel
import QueMusic 1.0
import 'qrc:/QueMusic/components'
Item {
    id: filePage

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
        for (var i = 0; i < localSortOptions.length; i++)
            if (localSortOptions[i].field === localSortField && localSortOptions[i].desc === localSortReversed) return i
        return 0
    }

    function toggleChoose(key) {
        filePage.chooseIndex = filePage.chooseIndex.indexOf(key) === -1
            ? filePage.chooseIndex.concat([key])
            : filePage.chooseIndex.filter(value => value !== key);
    }

    function clearChoose() {
        filePage.chooseIndex = [];
        filePage.setMode = 0;
    }

    /*Connections {
        target: songModel
        function onMetadataReady(count) {
            if (count > 0)
                mainWarn.tiped("歌曲信息已就绪（" + count + " 首）", 1);
        }
        function onSearchFinished(count) {
            if (folderMusic.searching)
                mainWarn.tiped(count > 0 ? "找到 " + count + " 个结果" : "没有找到匹配的歌曲", count > 0 ? 1 : 0);
        }
    }
    Connections {
        target: localFileModel
        function onMetadataReady(count) {
            if (count > 0)
                mainWarn.tiped("歌曲信息已就绪（" + count + " 首）", 1);
        }
        function onSearchFinished(count) {
            if (localFolderMusic.searching)
                mainWarn.tiped(count > 0 ? "找到 " + count + " 个结果" : "没有找到匹配的歌曲", count > 0 ? 1 : 0);
        }
    }*/

    function chooseTotal() {
        if (filePage.setMode === 3) return folderMusic.searching ? songModel.searchResults.count : songModel.rowCount();
        if (filePage.setMode === 4) return localFolderMusic.searching ? localFileModel.searchResults.count : localFileModel.count;
        return 0;
    }

    function isAllChosen() {
        var total = filePage.chooseTotal();
        return total > 0 && filePage.chooseIndex.length >= total;
    }

    function toggleAllChoose() {
        if (filePage.isAllChosen()) {
            filePage.chooseIndex = [];
            return;
        }
        var all = [];
        var i = 0;
        if (filePage.setMode === 3) {
            if (folderMusic.searching) {
                for (i = 0; i < songModel.searchResults.count; i++) all.push(songModel.searchResults.getRow(i).songId);
            } else {
                for (i = 0; i < songModel.rowCount(); i++) all.push(songModel.get(i).songId);
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

    function addChosenToList() {
        var count = filePage.chooseIndex.length;
        if (count === 0) {
            Style.warned("请先选择歌曲", 0);
            return;
        }
        var added = 0;
        if (filePage.setMode === 3) {
            for (var i = 0; i < songModel.rowCount(); i++) {
                var song = songModel.get(i);
                if (!song || !song.path || filePage.chooseIndex.indexOf(song.songId) === -1) continue;
                if (playListModel.indexOfPath(song.path) !== -1) continue;
                playListModel.append({ name: song.name, path: song.path, songer: song.singer || "", source: -1 });
                added++;
            }
        } else if (filePage.setMode === 4) {
            for (var j = 0; j < localFileModel.count; j++) {
                var path = localFileModel.get(j, "fileUrl").toString();
                if (filePage.chooseIndex.indexOf(path) === -1) continue;
                if (playListModel.indexOfPath(path) !== -1) continue;
                var title = localFileModel.get(j, "title") || "";
                var artist = localFileModel.get(j, "artist") || "";
                if (title === "") {
                    var meta = coverHelper.loadFullMetadata(path);
                    title = meta.title;
                    artist = artist || meta.artist;
                }
                playListModel.append({ name: title || localFileModel.get(j, "fileName"), path: path, songer: artist || "", source: -1 });
                added++;
            }
        }
        Style.warned(added === 0 ? "所选歌曲都已在播放列表中" : "成功加入播放列表 " + added + " 首", added === 0 ? 0 : 1);
    }

    function deleteChosen() {
        var count = filePage.chooseIndex.length;
        if (count === 0) {
            Style.warned(filePage.setMode < 3 ? "请先选择文件夹" : "请先选择歌曲", 0);
            return;
        }
        var moved = false;
        for (var i = 0; i < count; i++) {
            var key = filePage.chooseIndex[i];
            switch (filePage.setMode) {
            case 1: myFolderModel.deleteFolder(key); break;
            case 2: localFolderModel.deleteFolder(key); break;
            case 3: songModel.deleteSong(key); break;
            case 4: moved = MusicApi.moveLocalFileToTrash(key) || moved; break;
            }
        }
        if (moved) {
            var folder = localFileModel.folder;
            localFileModel.folder = "";
            localFileModel.folder = folder;
        }
        filePage.chooseIndex = [];
        Style.warned("成功删除" + count + (filePage.setMode < 3 ? "个文件夹" : "首音乐"), 1);
    }

    // 把「我的文件夹」歌曲模型里的歌全部加入播放列表，play=true 时立即播放
    function addAllSongModelToList(play) {
        var searching = folderMusic.searching;
        var total = searching ? songModel.searchResults.count : songModel.rowCount();
        if (total === 0) {
            Style.warned("当前文件夹没有歌曲", 0);
            return;
        }
        var playFirst = -1;
        var added = 0;
        for (var i = 0; i < total; i++) {
            var item = searching ? songModel.searchResults.getRow(i) : songModel.get(i);
            if (!item || !item.name || !item.path) continue;
            if (playListModel.indexOfPath(item.path) !== -1) continue;
            playListModel.append({ name: item.tagTitle || item.name, path: item.path, songer: item.tagArtist || item.singer || "", source: -1 });
            if (playFirst === -1) playFirst = playListModel.count - 1;
            added++;
        }
        if (added === 0) {
            Style.warned("列表中的歌曲都已在播放列表中", 0);
        } else {
            Style.warned("成功加入播放列表 " + added + " 首", 1);
        }
        if (play && playFirst !== -1) {
            Playback.goTo(playFirst);
        }
    }

    // 把「本地文件夹」里扫描到的音频文件全部加入播放列表，play=true 时立即播放
    function addAllLocalFilesToList(play) {
        var searching = localFolderMusic.searching;
        var total = searching ? localFileModel.searchResults.count : localFileModel.count;
        if (total === 0) {
            Style.warned("当前文件夹没有音频文件", 0);
            return;
        }
        var playFirst = -1;
        var added = 0;
        for (var i = 0; i < total; i++) {
            var row = searching ? localFileModel.searchResults.getRow(i) : null;
            var name = row ? (row.title || row.name) : localFileModel.get(i, "fileName");
            var path = row ? row.fileUrl.toString() : localFileModel.get(i, "fileUrl").toString();
            if (!name || !path) continue;
            if (playListModel.indexOfPath(path) !== -1) continue;
            playListModel.append({ name: name, path: path, songer: row ? (row.artist || "") : "", source: -1 });
            if (playFirst === -1) playFirst = playListModel.count - 1;
            added++;
        }
        if (added === 0) {
            Style.warned("列表中的歌曲都已在播放列表中", 0);
        } else {
            Style.warned("成功加入播放列表 " + added + " 首", 1);
        }
        if (play && playFirst !== -1) {
            Playback.goTo(playFirst);
        }
    }

    // 打开某个歌曲文件的所在文件夹（本地浏览器）
    function openSongFolder(songPath) {
        var p = songPath || "";
        if (p.startsWith("file:///"))
            p = p.substring(8);
        var idx = Math.max(p.lastIndexOf('/'), p.lastIndexOf('\\'));
        var dir = idx > 0 ? p.substring(0, idx) : p;
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
                color: Style.themes.fontColor
            }
        }

        QBlurTapBar {
            x: 24
            y: 80
            z: 5
            model: ["我的文件夹","本地文件夹"]
            tabWidth: 110
            width: 226
            rectXy: Qt.rect(0, 12, 244, 40)
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
            pageList: [myFile,localFile]
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
                        buttonColor: filePage.setMode === 1 ? Style.themes.containColor : Style.themes.primaryColor
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
                                    myFolderModel.addFolder(input, "my", "");
                                    Style.warned("成功添加一个文件夹",1);
                                } else {
                                    Style.warned("请输入文件名",0);
                                }
                            }
                        }
                        onClicked: dialog.open()
                    }
                }

                QListView {
                    id: folderView
                    anchors.fill: parent
                    model: myFolderModel
                    clip: true
                    topMargin: 60
                    headerModel: ["标题","","","菜单"]
                    function openFilePage(title,image) {
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
                                myFolderModel.renameFolder(editDialog.folderId, input);
                                mainWarn.tiped("成功修改文件夹名称",1);
                            } else {
                                mainWarn.tiped("请输入文件名",0);
                            }
                        }
                    }
                    delegate: Rectangle {
                        id: listfolder
                        height: 64
                        width: folderView.width - 16
                        radius: Style.settings.labelRadius
                        property bool chosen: filePage.setMode === 1 && filePage.chooseIndex.indexOf(model.folderId) !== -1
                        color: listfolder.chosen ? Style.themes.containColor : "#00000000"

                        Rectangle {
                            anchors.fill: parent
                            radius: Style.settings.labelRadius
                            color: Style.themes.hoverColor
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
                            color: Style.themes.containColor
                            radius: 10
                            Text {
                                anchors.fill: parent
                                text: "\uf0f5"
                                font.family: iconFont.name
                                font.pixelSize: Style.settings.texticon
                                color: Style.themes.fontColor
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
                            text: model.name
                            color: Style.themes.fontColor
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
                                    filePage.toggleChoose(model.folderId);
                                } else {
                                    filePage.folderNumber = index;
                                    window.exitIndex = 1;
                                    songModel.folderId = model.folderId;
                                    songModel.clearSearch();
                                    folderMusic.searching = false;
                                    folderView.openFilePage(model.name,"");
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
                                    tipText: model.path ? "打开文件夹位置" : "应用逻辑文件夹（无磁盘路径）"
                                    onClicked: {
                                        if (model.path) {
                                            Qt.openUrlExternally(model.path);
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
                                        if(model.folderId !== 1) {
                                            editDialog.input = model.name;
                                            editDialog.folderId = model.folderId;
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
                                        if(model.folderId !== 1) {
                                            globalDialog.openSimpleDialog("删除", "这将删除本文件夹，无法恢复，是否删除？",
                                                function() {
                                                    myFolderModel.deleteFolder(model.folderId);
                                                    Style.warned("成功删除一个我的文件夹",1);
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
                    nameFilters: ["*.mp3","*.wav","*.aac","*.flac","*.ogg","*.eac3","*.wma","*.ac3","*.alac","*.mkv","*.wmv","*.avi","*.mpeg4"]
                    showDirs: false
                    sortField: filePage.localSortField
                    sortReversed: filePage.localSortReversed
                    onScanFinished: function(count) {
                        if (count > 0)
                            mainWarn.tiped("已加载 " + count + " 个音频文件", 1);
                    }
                }

                FolderDialog {
                    id: folderDialog
                    title: "选择音乐的文件夹"
                    onAccepted: {
                        // 获取选中的文件夹URL（file:// 格式）
                        var folderUrl = folderDialog.selectedFolder;
                        var folderPath = folderUrl.toString();
                        var folderName = folderPath.split('/').pop(); // 使用 '/' 分割，取最后一部分
                        localFolderModel.addFolder(folderName, "local", folderPath);
                        mainWarn.tiped("成功定位一个本地文件夹",1);

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
                        buttonColor: filePage.setMode === 2 ? Style.themes.containColor : Style.themes.primaryColor
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

                QListView {
                    id: localFolderView
                    anchors.fill: parent
                    model: localFolderModel
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
                                localFolderModel.renameFolder(editLocalDialog.index, input);
                            } else {
                                mainWarn.opened("请输入文件名",0);
                            }
                        }
                    }
                    delegate: Rectangle {
                        id: listLocalfolder
                        height: 64
                        width: localFolderView.width - 16
                        radius: Style.settings.labelRadius
                        property bool chosen: filePage.setMode === 2 && filePage.chooseIndex.indexOf(model.folderId) !== -1
                        color: listLocalfolder.chosen ? Style.themes.containColor : "#00000000"

                        Rectangle {
                            anchors.fill: parent
                            radius: Style.settings.labelRadius
                            color: Style.themes.hoverColor
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
                            color: Style.themes.containColor
                            radius: 10
                            Text {
                                anchors.fill: parent
                                text: "\uf0f5"
                                font.family: iconFont.name
                                font.pixelSize: Style.settings.texticon
                                color: Style.themes.fontColor
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }


                        Label {
                            x: 80
                            z: 3
                            width: parent.width / 2 - 108
                            height: 64
                            text: model.name
                            color: Style.themes.fontColor
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
                            text: model.path.substring(8)
                            color: Style.themes.textColor
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
                                    filePage.toggleChoose(model.folderId);
                                } else {
                                    localFileModel.folder = model.path;
                                    window.exitIndex = 1
                                    localFileModel.clearSearch();
                                    localFolderMusic.searching = false;
                                    localFolderMusic.opened(model.name,"");
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
                                        if (model.path) {
                                            Qt.openUrlExternally(model.path);
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
                                        editLocalDialog.input = model.name
                                        editLocalDialog.index = model.folderId
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
                                        globalDialog.openSimpleDialog("删除", "这将移除本文件夹，是否删除？",
                                            function() {
                                                localFolderModel.deleteFolder(model.folderId);
                                                Style.warned("成功移除一个本地文件夹",1);
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
                nameFilters: ["音频文件 (*.mp3 *.wav *.aac *.flac *.ogg *.eac3 *.wma *.ac3 *.alac *.mkv *.wmv *.avi *.mpeg4)"]
                onAccepted: {
                    // 获取选中的文件URL（file:// 格式）
                    var fileUrls = musicfileDialog.selectedFiles;

                    // 先批量转换为本地路径，再一次交给 C++ 侧事务写入，
                    // 避免上千首歌曲重复打开DB/刷新列表导致界面假死。
                    var importList = [];
                    for (var i = 0; i < fileUrls.length; i++) {
                        var filePath = fileUrls[i].toString();
                        if (filePath.startsWith("file:///")) {
                            filePath = filePath.substring(8);// 去前8字符：file:///
                        }
                        var fileName = filePath.split('/').pop(); // 使用 '/' 分割，取最后一部分
                        if (fileName && filePath) {
                            importList.push({ name: fileName, path: filePath, singer: "" });
                        }
                    }

                    if (importList.length > 0) {
                        var added = songModel.addSongs(songModel.folderId, importList);
                        Style.warned("成功导入 " + added + " 首音乐", 1);
                    }
                }
                onRejected: {
                    console.log("操作取消");
                }
            }

            QSortModel {
                id: songSort
                model: songModel
                options: [
                    { label: "默认顺序", mode: 0, desc: false },
                    { label: "文件名 A→Z", mode: 1, desc: false },
                    { label: "文件名 Z→A", mode: 1, desc: true }
                ]
                nameRole: "name"
            }

            QSortModel {
                id: songSearchSort
                model: songModel.searchResults
                options: songSort.options
                nameRole: "name"
            }

            QMenu {
                id: songSortMenu
                model: songSort.options.map(o => o.label)
                current: songSort.menuIndex
                blurSource: null
                masked: true
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
                    buttonColor: Style.themes.sideColor
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
                    buttonColor: Style.themes.sideColor
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
                    buttonColor: Style.themes.sideColor
                    tipText: "刷新当前文件夹"
                    onClicked: {
                        songModel.clearSearch();
                        folderMusic.searching = false;
                        filterInput1.text = "";
                        if (songModel.folderId >= 0) {
                            songModel.loadByFolder(songModel.folderId);
                        }
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
                    buttonColor: Style.themes.sideColor
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
                    buttonColor: filePage.setMode === 3 ? Style.themes.containColor : Style.themes.primaryColor
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
                placeholderTextColor: Style.themes.textColor
                color: Style.themes.textColor
                font.pixelSize: Style.settings.text
                verticalAlignment: Text.AlignVCenter
                selectionColor: Style.themes.containColor
                onTextChanged: filterDebounce1.restart()
                background: Rectangle {
                    radius: Style.settings.labelRadius
                    color: Style.themes.primaryColor
                    border.width: 2
                    border.color: filterInput1.focus ? Style.themes.themeColor : Style.themes.sideColor
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
                        songModel.clearSearch();
                        folderMusic.searching = false;
                    }
                }
                Timer {
                    id: filterDebounce1
                    interval: 250
                    onTriggered: {
                        var t = filterInput1.text.trim();
                        if (t === "") {
                            songModel.clearSearch();
                            folderMusic.searching = false;
                        } else {
                            folderMusic.searching = true;
                            songModel.startSearch(t);
                        }
                    }
                }
            }

            QListView {
                id: fileView
                x: 24
                y: 128
                width: folderMusic.width - 32
                height: folderMusic.height - 128
                model: folderMusic.searching ? songSearchSort : songSort
                clip: true
                reuseItems: true
                headerModel: ["标题","歌手","","菜单"]
                property int transY: 0
                transform: Translate { y: fileView.transY }
                populate: Transition {
                    id: localFileLoadAnime2
                    SequentialAnimation {
                        PropertyAction {
                            property: "opacity"
                            value: 0
                        }
                        PauseAnimation {
                            duration: localFileLoadAnime2.ViewTransition.index * 40
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
                delegate: Rectangle {
                    id: listfile
                    height: 60
                    width: fileView.width - 16
                    radius: Style.settings.labelRadius
                    property bool chosen: filePage.setMode === 3 && filePage.chooseIndex.indexOf(model.songId) !== -1
                    color: listfile.chosen || mainMedia.source == model.path ? Style.themes.containColor : "transparent"
                    property int transY: 0
                    transform: Translate { y: listfile.transY }

                    readonly property string coverUrl: {
                        if (model.tagCoverUrl !== "") return model.tagCoverUrl;
                        if (!model.path) return "qrc:/QueMusic/resources/app/musicpic.png";
                        return MusicApi.readLocalCoverHint(model.path) || "qrc:/QueMusic/resources/app/musicpic.png";
                    }
                    property string songTitle: model.tagTitle || model.name
                    property string artistName: model.tagArtist || model.singer || ""

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
                        color: Style.themes.hoverColor
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
                        color: Style.themes.fontColor
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
                        color: Style.themes.textColor
                        font.pixelSize: Style.settings.textTip
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: fileArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if(filePage.setMode === 3) {
                                filePage.toggleChoose(model.songId);
                                return;
                            }
                            window.playLocalSong(model.path, listfile.songTitle);
                            var musicName = listfile.songTitle;
                            var musicPath = model.path;
                            var listIndex = playListModel.indexOfName(musicName);
                            if (listIndex == -1) {
                                playListModel.append({ name: musicName, path: musicPath, songer: listfile.artistName, source: -1 });
                                playListModel.playListIndex = playListModel.count - 1;
                            }
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
                                    var musicName = listfile.songTitle;
                                    var musicPath = model.path;
                                    var listIndex = playListModel.indexOfName(musicName);
                                    if (listIndex == -1) {
                                        playListModel.append({ name: musicName, path: musicPath, songer: listfile.artistName, source: -1 });
                                        Style.warned("成功加入播放列表",1);
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
                                    filePage.openSongFolder(model.path);
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
                                    songModel.deleteSong(model.songId);
                                    Style.warned("成功移除一个音乐",1);
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
                blurSource: null
                masked: true
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
                    buttonColor: Style.themes.sideColor
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
                    buttonColor: Style.themes.sideColor
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
                    buttonColor: Style.themes.sideColor
                    tipText: "刷新当前文件夹"
                    onClicked: {
                        localFileModel.clearSearch();
                        localFolderMusic.searching = false;
                        filterInput2.text = "";
                        var folder = localFileModel.folder;
                        localFileModel.folder = "";
                        localFileModel.folder = folder;
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
                    buttonColor: Style.themes.sideColor
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
                    buttonColor: filePage.setMode === 4 ? Style.themes.containColor : Style.themes.primaryColor
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
                placeholderTextColor: Style.themes.textColor
                color: Style.themes.textColor
                font.pixelSize: Style.settings.text
                verticalAlignment: Text.AlignVCenter
                selectionColor: Style.themes.containColor
                onTextChanged: filterDebounce2.restart()
                background: Rectangle {
                    radius: Style.settings.labelRadius
                    color: Style.themes.primaryColor
                    border.width: 2
                    border.color: filterInput2.focus ? Style.themes.themeColor : Style.themes.sideColor
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
                        var t = filterInput2.text.trim();
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

            QListView {
                id: localFileView
                x: 24
                y: 128
                width: localFolderMusic.width - 32
                height: localFolderMusic.height - 128
                model: localFolderMusic.searching ? localFileModel.searchResults : localFileModel
                clip: true
                reuseItems: true
                headerModel: ["标题","歌手","","菜单"]
                populate: Transition {
                    id: localFileLoadAnime
                    SequentialAnimation {
                        PropertyAction {
                            property: "opacity"
                            value: 0
                        }
                        PauseAnimation {
                            duration: localFileLoadAnime.ViewTransition.index * 40
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
                delegate: Rectangle {
                    id: listLocalFile
                    height: 60
                    width: localFileView.width - 16
                    radius: Style.settings.labelRadius
                    property bool chosen: filePage.setMode === 4 && filePage.chooseIndex.indexOf(model.fileUrl.toString()) !== -1
                    color: listLocalFile.chosen || mainMedia.source == model.fileUrl ? Style.themes.containColor : "transparent"
                    property int transY: 0
                    transform: Translate { y: listLocalFile.transY }

                    readonly property string rowPath: model.fileUrl ? model.fileUrl.toString() : ""
                    readonly property string coverUrl: {
                        if (model.coverUrl !== "") return model.coverUrl;
                        if (!rowPath) return "qrc:/QueMusic/resources/app/musicpic.png";
                        return MusicApi.readLocalCoverHint(rowPath) || "qrc:/QueMusic/resources/app/musicpic.png";
                    }
                    property string songTitle: model.title || model.fileName
                    property string artistName: model.artist || ""

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
                        color: Style.themes.hoverColor
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
                        color: Style.themes.fontColor
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
                        color: Style.themes.textColor
                        font.pixelSize: Style.settings.textTip
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    MouseArea {
                        id: localFileArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            if(filePage.setMode === 4) {
                                filePage.toggleChoose(model.fileUrl.toString());
                                return;
                            }
                            window.playLocalSong(model.fileUrl.toString(), listLocalFile.songTitle);
                            var musicName = listLocalFile.songTitle;
                            var musicPath = model.fileUrl.toString();
                            var listIndex = playListModel.indexOfName(musicName);
                            if (listIndex == -1) {
                                playListModel.append({ name: musicName, path: musicPath, songer: listLocalFile.artistName, source: -1 });
                                playListModel.playListIndex = playListModel.count - 1;
                            }
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
                                    var musicName = listLocalFile.songTitle;
                                    var musicPath = model.fileUrl.toString();
                                    var listIndex = playListModel.indexOfName(musicName);
                                    if (listIndex == -1) {
                                        playListModel.append({ name: musicName, path: musicPath, songer: listLocalFile.artistName, source: -1 });
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
                                    filePage.openSongFolder(model.fileUrl.toString());
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
                                onClicked: {
                                    var targetName = model.fileName;
                                    var targetPath = model.fileUrl.toString();
                                    globalDialog.openSimpleDialog("删除本地文件", "这将把「" + targetName + "」从当前文件夹移入回收站，是否继续？",
                                        function() {
                                            if (MusicApi.moveLocalFileToTrash(targetPath)) {
                                                filePage.refreshSongList();
                                                Style.warned("已将文件移入回收站", 1);
                                            } else {
                                                Style.warned("移动文件失败", 0);
                                            }
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

    Connections {
        target: folderMusic
        function onVisibleChanged() {
            if (!folderMusic.visible) filePage.clearChoose();
        }
    }
    Connections {
        target: localFolderMusic
        function onVisibleChanged() {
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
            GradientStop { position: 1.0; color: Style.themes.sideColor }
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
            buttonColor: filePage.isAllChosen() ? Style.themes.themeColor : Style.themes.fullColor
            textColor: filePage.isAllChosen() ? Style.themes.primaryColor : Style.themes.fontColor
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
            color: "transparent"//Style.themes.fullColor
            Text {
                anchors.centerIn: parent
                text: "已选择:" + filePage.chooseIndex.length + "项"
                color: Style.themes.textColor
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
            buttonColor: Style.themes.themeColor
            textColor: Style.themes.primaryColor
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
            textColor: Style.themes.primaryColor
            text: "删除"
            borderWidth: 1
            onClicked: {
                var tip = filePage.setMode === 4 ? "这将把这些文件移入回收站，是否删除？"
                        : filePage.setMode === 3 ? "这将从文件夹移除这些歌曲，无法恢复，是否删除？"
                        : "这将删除这些文件夹，无法恢复，是否删除？";
                globalDialog.openSimpleDialog("删除", tip,
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
            buttonColor: Style.themes.themeColor
            textColor: Style.themes.primaryColor
            borderWidth: 1
            text: "完成"
            onClicked: filePage.clearChoose()
        }
    }
}