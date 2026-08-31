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
    property list<int> chooseIndex: []
    signal loaded()
    signal cancelChoose()

    // 在播放列表中查找同名/同路径歌曲，避免重复添加本地文件
    function findIndexByValue(model, key, targetValue) {
        for (var i = 0; i < model.count; i++) {
            var element = model.get(i);
            if (element && element[key] === targetValue) {
                return i;
            }
        }
        return -1; // 未找到返回 -1
    }

    // 把「我的文件夹」歌曲模型里的歌全部加入播放列表，play=true 时立即播放
    function addAllSongModelToList(play) {
        if (songModel.count === 0) {
            Style.warned("当前文件夹没有歌曲", 0);
            return;
        }
        var playFirst = -1;
        var added = 0;
        for (var i = 0; i < songModel.count; i++) {
            var item = songModel.get(i);
            if (!item || !item.name || !item.path) continue;
            if (filePage.findIndexByValue(playListModel, "path", item.path) !== -1) continue;
            playListModel.append({ name: item.name, path: item.path, songer: item.singer || "", source: -1 });
            if (playFirst === -1) playFirst = playListModel.count - 1;
            added++;
        }
        if (added === 0) {
            Style.warned("列表中的歌曲都已在播放列表中", 0);
        } else {
            Style.warned("成功加入播放列表 " + added + " 首", 1);
        }
        if (play && playFirst !== -1) {
            playListModel.playListIndex = playFirst;
            musicControlMin.refreshMusicPlay();
        }
    }

    // 把「本地文件夹」里扫描到的音频文件全部加入播放列表，play=true 时立即播放
    function addAllLocalFilesToList(play) {
        if (localFileModel.count === 0) {
            Style.warned("当前文件夹没有音频文件", 0);
            return;
        }
        var playFirst = -1;
        var added = 0;
        for (var i = 0; i < localFileModel.count; i++) {
            var name = localFileModel.get(i, "fileName");
            var fileUrl = localFileModel.get(i, "fileUrl");
            if (!name || !fileUrl) continue;
            var path = fileUrl.toString();
            if (filePage.findIndexByValue(playListModel, "path", path) !== -1) continue;
            playListModel.append({ name: name, path: path, songer: "", source: -1 });
            if (playFirst === -1) playFirst = playListModel.count - 1;
            added++;
        }
        if (added === 0) {
            Style.warned("列表中的歌曲都已在播放列表中", 0);
        } else {
            Style.warned("成功加入播放列表 " + added + " 首", 1);
        }
        if (play && playFirst !== -1) {
            playListModel.playListIndex = playFirst;
            musicControlMin.refreshMusicPlay();
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

    // 刷新当前浏览的音频列表：重新从磁盘/数据库读取
    function refreshSongList() {
        if (songModel.folderId >= 0) {
            songModel.loadByFolder(songModel.folderId);
        }
        if (localFileModel.folder.toString()) {
            var folder = localFileModel.folder;
            localFileModel.folder = "";
            localFileModel.folder = folder;
        }
        Style.warned("已刷新当前列表", 1);
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
            tabWidth: 120
            width: 244
            rectXy: Qt.rect(0, 12, 244, 40)
            blurSource: fileChildPage
            onTabChange: (index) => {
                fileChildPage.stack(index);
                filePage.setMode = 0;
                filePage.chooseIndex = [];
                filePage.cancelChoose();
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
                        buttonColor: filePage.setMode === 1 ? Style.themes.containColor : Style.themes.fullColor
                        onClicked: {
                            if(filePage.setMode === 1) {
                                filePage.setMode = 0;
                                filePage.chooseIndex = [];
                                filePage.cancelChoose();
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
                            //standardButtons: Dialog.Ok | Dialog.Cancel
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
                        filePage.loaded()
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
                        //property int index
                        property int folderId
                        onConfirm: {
                            if(input!=="") {
                                //myfileModel.setProperty(index, "name", input)
                                //var folderId = myFolderModel.data(myFolderModel.index(folderIndex), 256)
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
                        color: "#00000000"//index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"
                        Connections {
                            target: filePage
                            function onCancelChoose() {
                                listfolder.color = "#00000000"
                            }
                        }

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
                                    if(listfolder.color == "#00000000") {
                                        listfolder.color = Style.themes.containColor;
                                        filePage.chooseIndex.push(model.folderId);
                                    } else {
                                        listfolder.color = "#00000000";
                                        filePage.chooseIndex = filePage.chooseIndex.filter(value => value !== model.folderId);
                                    }
                                } else {
                                    filePage.folderNumber = index;
                                    window.exitIndex = 1;
                                    songModel.folderId = model.folderId;
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
                                            //editDialog.index = index
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
                                            //myfileModel.remove( index, 1 )
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
                FolderListModel {
                    id: localFileModel
                    nameFilters: ["*.mp3","*.wav","*.aac","*.flac","*.ogg","*.eac3","*.wma","*.ac3","*.alac","*.mkv","*.wmv","*.avi","*.mpeg4"]
                    showDirs: false
                }

                FolderDialog {
                    id: folderDialog
                    title: "选择音乐的文件夹"
                    onAccepted: {
                        // 获取选中的文件夹URL（file:// 格式）
                        var folderUrl = folderDialog.selectedFolder;
                        var folderPath = folderUrl.toString();
                        var folderName = folderPath.split('/').pop(); // 使用 '/' 分割，取最后一部分
                        //localFolderModel.append({ name: folderName, path: folderUrl, local: "true" })
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
                        buttonColor: filePage.setMode === 2 ? Style.themes.containColor : Style.themes.fullColor
                        onClicked: {
                            if(filePage.setMode === 2) {
                                filePage.setMode = 0;
                                filePage.chooseIndex = [];
                                filePage.cancelChoose()
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
                    headerModel: ["标题","","","菜单"]

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
                                //localFolderModel.setProperty(index, "name", input)
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
                        color: "#00000000"//index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"
                        Connections {
                            target: filePage
                            function onCancelChoose() {
                                listLocalfolder.color = "#00000000"
                            }
                        }

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
                            id: foldersArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: {
                                if(filePage.setMode === 2) {
                                    var isChoose = false;
                                    if(listLocalfolder.color == "#00000000") {
                                        listLocalfolder.color = Style.themes.containColor;
                                        filePage.chooseIndex.push(model.folderId);
                                    } else {
                                        listLocalfolder.color = "#00000000";
                                        filePage.chooseIndex = filePage.chooseIndex.filter(value => value !== model.folderId);
                                    }
                                } else {
                                    localFileModel.folder = model.path;
                                    window.exitIndex = 1
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
                                        globalDialog.openSimpleDialog("删除", "这将删除本文件夹，无法恢复，是否删除？",
                                            function() {
                                                localFolderModel.deleteFolder(model.folderId);
                                                Style.warned("成功删除一个本地文件夹",1);
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

        content: Item {
            anchors.fill: parent
            Connections {
                target: filePage
                function onLoaded() {
                    console.log("更新音乐文件夹列表成功:",folderMusic.musicList);
                }
            }

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

            // 顶栏
            Row {
                x: 144
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
                    iconCharacter: "\uf0c8"
                    shadowEnabled: false
                    buttonColor: Style.themes.sideColor
                    tipText: "刷新当前文件夹"
                    onClicked: {
                        filePage.refreshSongList();
                    }
                }
            }

            QButton {
                x: localFolderMusic.width - 124
                y: 44
                height: 40; width: 100
                radius: 20
                z: 10
                iconCharacter: "\uf10d"
                text: "导入"
                onClicked: {
                    musicfileDialog.open();
                }
            }

            QListView {
                id: fileView
                x: 24
                y: 128
                width: folderMusic.width - 32
                height: folderMusic.height - 128
                model: songModel//parent.visible ? folderMusic.foldercontent : []
                clip: true
                headerModel: ["标题","","","菜单"]
                delegate: Rectangle {
                    id: listfile
                    height: 60
                    width: fileView.width - 16
                    radius: Style.settings.labelRadius
                    color: mainMedia.noTitle == model.name ? Style.themes.containColor : "transparent"

                    // 列表内封面：内嵌封面 -> 同目录封面 -> .json 封面 -> 默认图标
                    property string coverUrl: {
                        if (!model.path) return "";
                        var path = model.path;
                        var embedded = coverHelper.findEmbeddedCover(path);
                        if (embedded) return embedded;
                        var local = coverHelper.findLocalCover(path);
                        if (local) return local;
                        var meta = MusicApi.readLocalMetadata(path);
                        if (meta && meta.cover) return meta.cover;
                        return "";
                    }

                    Behavior on color { ColorAnimation { duration: 120 } }

                    Rectangle {
                        y: 8
                        x: 8
                        z: 4
                        width: 44
                        height: 44
                        color: Style.themes.containColor
                        radius: 10
                        Text {
                            anchors.fill: parent
                            text: "\uf044"
                            font.family: iconFont.name
                            font.pixelSize: Style.settings.texticon
                            color: Style.themes.fontColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        QPicture {
                            anchors.fill: parent
                            source: listfile.coverUrl
                            visible: listfile.coverUrl !== ""
                            radius1: 10
                            radius2: 10
                            radius3: 10
                            radius4: 10
                        }
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
                        x: 80
                        y: 0
                        z: 3
                        width: 140
                        height: 60
                        text: model.name
                        color: Style.themes.fontColor
                        font.bold: true
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        visible: true
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    MouseArea {
                        id: fileArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            window.playLocalSong(model.path, model.name);
                            var musicName = model.name;
                            var musicPath = model.path;
                            var listIndex = listfile.findIndexByValue(playListModel, "name", musicName);
                            if (listIndex == -1) {
                                playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
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
                                    var musicName = model.name;
                                    var musicPath = model.path;
                                    var listIndex = listfile.findIndexByValue(playListModel, "name", musicName);
                                    if (listIndex == -1) {
                                        playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
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
                                    //myfileModel.get(filePage.folderNumber).music.remove(index)
                                    songModel.deleteSong(model.songId);
                                    Style.warned("成功删除一个音乐",1);
                                }
                            }
                        }
                    }

                    function findIndexByValue(model, key, targetValue) {
                        for (var i = 0; i < model.count; i++) {
                            var element = model.get(i);
                            if (element[key] === targetValue) {
                                return i; // 返回找到的索引
                            }
                        }
                        return -1; // 未找到返回 -1
                    }
                }
            }
        }
    }

    AnimatorWindow {
        id: localFolderMusic
        mainTarget: fileMain
        winIndex: 1

        content: Item {
            anchors.fill: parent
            // 顶栏
            Row {
                x: 144
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
                    iconCharacter: "\uf0c8"
                    shadowEnabled: false
                    buttonColor: Style.themes.sideColor
                    tipText: "刷新当前文件夹"
                    onClicked: {
                        filePage.refreshSongList();
                    }
                }
            }

            QButton {
                x: localFolderMusic.width - width - 24
                y: 44
                height: 40
                radius: 20
                z: 10
                text: "文件夹中显示"
                iconCharacter: "\uf0fb"
                onClicked: {
                    Qt.openUrlExternally(localFileModel.folder);
                }
            }

            QListView {
                id: localFileView
                x: 24
                y: 128
                width: folderMusic.width - 32
                height: folderMusic.height - 128
                model: localFileModel
                clip: true
                headerModel: ["标题","","","菜单"]
                populate: Transition {
                    id: localFileLoadAnime
                    SequentialAnimation {
                        PropertyAction {
                            property: "opacity"
                            value: 0
                        }
                        PauseAnimation {
                            duration: localFileLoadAnime.ViewTransition.index * 80
                        }
                        ParallelAnimation {
                            NumberAnimation {
                                properties: "x"
                                from: 400
                                to: 0
                                duration: 350
                                easing.type: Easing.OutExpo
                            }
                            NumberAnimation {
                                properties: "opacity"
                                from: 0
                                to: 1
                                duration: 350
                                easing.type: Easing.OutExpo
                            }
                        }
                    }
                }
                delegate: Rectangle {
                    id: listLocalFile
                    height: 60
                    width: localFileView.width - 16
                    radius: Style.settings.labelRadius
                    color: mainMedia.source == model.fileUrl ? Style.themes.containColor : "transparent"

                    // 列表内封面：内嵌封面 -> 同目录封面 -> .json 封面 -> 默认图标
                    property string coverUrl: {
                        if (!model.fileUrl) return "";
                        var path = model.fileUrl.toString();
                        var embedded = coverHelper.findEmbeddedCover(path);
                        if (embedded) return embedded;
                        var local = coverHelper.findLocalCover(path);
                        if (local) return local;
                        var meta = MusicApi.readLocalMetadata(path);
                        if (meta && meta.cover) return meta.cover;
                        return "";
                    }

                    Behavior on color { ColorAnimation { duration: 120 } }

                    Rectangle {
                        y: 8
                        x: 8
                        z: 4
                        width: 44
                        height: 44
                        color: Style.themes.containColor
                        radius: 10
                        Text {
                            anchors.fill: parent
                            text: "\uf044"
                            font.family: iconFont.name
                            font.pixelSize: Style.settings.texticon
                            color: Style.themes.fontColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        QPicture {
                            anchors.fill: parent
                            source: listLocalFile.coverUrl
                            visible: listLocalFile.coverUrl !== ""
                            radius1: 10
                            radius2: 10
                            radius3: 10
                            radius4: 10
                        }
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
                        x: 80
                        y: 0
                        z: 3
                        width: 140
                        height: 60
                        text: model.fileName
                        color: Style.themes.fontColor
                        font.bold: true
                        font.pixelSize: Style.settings.textmain
                        verticalAlignment: Text.AlignVCenter
                        visible: true
                        Behavior on color { ColorAnimation { duration: 120 } }
                    }

                    MouseArea {
                        id: localFileArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            window.playLocalSong(model.fileUrl.toString(), model.fileName);
                            var musicName = model.fileName;
                            var musicPath = model.fileUrl.toString();
                            var listIndex = listLocalFile.findIndexByValue(playListModel, "name", musicName);
                            if (listIndex == -1) {
                                playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
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
                                    var musicName = model.fileName;
                                    var musicPath = model.fileUrl.toString();
                                    var listIndex = listLocalFile.findIndexByValue(playListModel, "name", musicName);
                                    if (listIndex == -1) {
                                        playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
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
                                    globalDialog.openSimpleDialog("移除本地文件", "这将把「" + targetName + "」从当前文件夹移入回收站，是否继续？",
                                        function() {
                                            if (MusicApi.moveLocalFileToTrash(targetPath)) {
                                                Style.warned("已将文件移入回收站", 1);
                                                var folder = localFileModel.folder;
                                                localFileModel.folder = "";
                                                localFileModel.folder = folder;
                                            } else {
                                                Style.warned("移动文件失败", 0);
                                            }
                                        }
                                    );
                                }
                            }
                        }
                    }

                    function findIndexByValue(model, key, targetValue) {
                        for (var i = 0; i < model.count; i++) {
                            var element = model.get(i);
                            if (element[key] === targetValue) {
                                return i; // 返回找到的索引
                            }
                        }
                        return -1; // 未找到返回 -1
                    }
                }
            }
        }
    }

    // 选择模式
    Rectangle {
        id: chooseArea
        x: 0
        y: visible ? filePage.height - 60 : filePage.height
        width: filePage.width
        height: 60
        visible: filePage.setMode !== 0
        color: Style.themes.sideColor
        Behavior on y { NumberAnimation { duration: 420; easing.type: Easing.OutExpo } }
        Rectangle {
            x: 16
            y: 12
            width: 92
            height: 36
            radius: 20
            color: Style.themes.fullColor
            Text {
                anchors.centerIn: parent
                text: "多选模式"
                color: Style.themes.textColor
                font.pixelSize: Style.settings.textmain
            }
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
            x: chooseArea.width - 208
            shadowEnabled: false
            width: 92
            height: 36
            radius: 20
            buttonColor: "#fa4642"
            textColor: Style.themes.primaryColor
            text: "删除"
            onClicked: {
                switch(filePage.setMode) {
                case 1:
                    globalDialog.openSimpleDialog("删除", "这将删除这些文件夹，无法恢复，是否删除？",
                        function() {
                            for(var i=0;i<filePage.chooseIndex.length;i++) {
                                myFolderModel.deleteFolder(filePage.chooseIndex[i]);
                            }
                            filePage.chooseIndex = [];
                            Style.warned("成功删除" + filePage.chooseIndex.length + "个我的文件夹",1);
                        }
                    );
                    break;
                case 2:
                    globalDialog.openSimpleDialog("删除", "这将删除这些文件夹，无法恢复，是否删除？",
                        function() {
                            for(var i=0;i<filePage.chooseIndex.length;i++) {
                                localFolderModel.deleteFolder(localFolderModel[filePage.chooseIndex[i]].folderId);
                            }
                            filePage.chooseIndex = [];
                            Style.warned("成功删除" + filePage.chooseIndex.length + "个本地文件夹",1);
                        }
                    );
                    break;
                default:
                    break;
                }
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
            text: "完成"
            onClicked: {
                filePage.setMode = 0;
                filePage.chooseIndex = [];
                filePage.cancelChoose()
            }
        }
    }
}