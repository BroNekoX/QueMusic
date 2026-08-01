// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
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
    signal loaded()

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
                fileChildPage.stack(index)
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

                QBlurCard {
                    x: myFile.width - 128
                    y: 10
                    z: 1
                    width: 112
                    height: 40
                    //cardColor: Style.themes.primaryBlurColor
                    borderRadius: Style.settings.noControlRadius ? Style.settings.labelRadius : 20
                    rectXy: Qt.rect(x, 10, 112, 40)
                    blurSource: folderView
                    shadowEffect: true

                    Row {
                        anchors.fill: parent
                        anchors.margins: 2
                        // 列表
                        SButton {
                            width: 36
                            height: 36
                            radius: 18
                            iconCharacter: "\uf074"
                            buttonColor: "transparent"
                            hoverColor: Style.themes.hoverColor
                            onClicked: {

                            }
                        }
                        // 选择
                        SButton {
                            width: 36
                            height: 36
                            radius: 18
                            iconCharacter: "\uf09f"
                            buttonColor: "transparent"
                            hoverColor: Style.themes.hoverColor
                            onClicked: {

                            }
                        }
                        // 添加
                        SButton {
                            width: 36
                            height: 36
                            radius: 18
                            iconCharacter: "\uf0f8"
                            buttonColor: "transparent"
                            hoverColor: Style.themes.hoverColor
                            QAlertDialog {
                                id: dialog
                                title: "新建文件夹"
                                message: "为文件夹设定一个名称："
                                isInput: true
                                //standardButtons: Dialog.Ok | Dialog.Cancel
                                onConfirm: {
                                    if(input!=="") {
                                        myFolderModel.addFolder(input, "my", "");
                                        mainWarn.tiped("成功添加一个文件夹",1);
                                    } else {
                                        mainWarn.tiped("请输入文件名",0);
                                    }
                                }
                            }
                            onClicked: dialog.open()
                        }
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
                        color: index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"

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
                                filePage.folderNumber = index
                                window.exitIndex = 1
                                songModel.folderId = model.folderId
                                folderView.openFilePage(model.name,"")
                            }
                            Row {
                                anchors.right: parent.right
                                anchors.rightMargin: 20
                                spacing: 5
                                z: 2
                                y: 12
                                height: 40
                                SButton {
                                    iconCharacter: "\uf050"
                                    width: 40
                                    height: 40
                                    radius: 40
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                    shadowEnabled: false
                                    onClicked: {
                                    }
                                }
                                SButton {
                                    iconCharacter: "\uf005"
                                    width: 40
                                    height: 40
                                    radius: 40
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                    shadowEnabled: false

                                    onClicked: {
                                        if(model.folderId !== 1) {
                                            editDialog.input = model.name;
                                            //editDialog.index = index
                                            editDialog.folderId = model.folderId;
                                            editDialog.open();
                                        } else {
                                            mainWarn.tiped("无法修改默认文件夹名称",0);
                                        }
                                    }
                                }
                                SButton {
                                    iconCharacter: "\uf08e"
                                    width: 40
                                    height: 40
                                    radius: 40
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                    shadowEnabled: false
                                    onClicked: {
                                        if(model.folderId !== 1) {
                                            //myfileModel.remove( index, 1 )
                                            myFolderModel.deleteFolder(model.folderId);
                                            mainWarn.tiped("成功删除一个我的文件夹",1);
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

                QBlurCard {
                    x: localFile.width - 128
                    y: 10
                    z: 1
                    width: 112
                    height: 40
                    //cardColor: Style.themes.primaryBlurColor
                    borderRadius: Style.settings.noControlRadius ? Style.settings.labelRadius : 20
                    rectXy: Qt.rect(x, 10, 112, 40)
                    blurSource: localFolderView
                    shadowEffect: true

                    Row {
                        anchors.fill: parent
                        anchors.margins: 2
                        // 列表
                        SButton {
                            width: 36
                            height: 36
                            radius: 18
                            iconCharacter: "\uf074"
                            buttonColor: "transparent"
                            onClicked: {

                            }
                        }
                        // 选择
                        SButton {
                            width: 36
                            height: 36
                            radius: 18
                            iconCharacter: "\uf09f"
                            buttonColor: "transparent"
                            onClicked: {

                            }
                        }
                        // 导入
                        SButton {
                            width: 36
                            height: 36
                            radius: 18
                            iconCharacter: "\uf00a"
                            buttonColor: "transparent"
                            onClicked: {
                                folderDialog.open();
                            }
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
                        color: index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"

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
                                localFileModel.folder = model.path;
                                window.exitIndex = 1
                                localFolderMusic.opened(model.name,"");
                            }

                            Row {
                                anchors.right: parent.right
                                anchors.rightMargin: 20
                                spacing: 5
                                z: 2
                                y: 12
                                height: 40
                                SButton {
                                    iconCharacter: "\uf050"
                                    width: 40
                                    height: 40
                                    radius: 40
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                    shadowEnabled: false
                                    onClicked: {
                                    }
                                }
                                SButton {
                                    iconCharacter: "\uf005"
                                    width: 40
                                    height: 40
                                    radius: 40
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                    shadowEnabled: false

                                    onClicked: {
                                        editLocalDialog.input = model.name
                                        editLocalDialog.index = model.folderId
                                        editLocalDialog.open()
                                    }
                                }
                                SButton {
                                    iconCharacter: "\uf08e"
                                    width: 40
                                    height: 40
                                    radius: 40
                                    buttonColor: "transparent"
                                    hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                    shadowEnabled: false
                                    onClicked: {
                                        localFolderModel.deleteFolder(model.folderId);
                                        mainWarn.tiped("成功删除一个本地文件夹",1);
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

                    // 将URL转换为本地文件路径（去掉 'file:///' 前缀）
                    for (var i = 0; i < fileUrls.length; i++) {
                        var fileUrl = fileUrls[i];
                        var filePath = fileUrl.toString();
                        if (filePath.startsWith("file:///")) {
                            filePath = filePath.substring(8); // 在Windows上通常是 file:///C:/...，需要去掉前8个字符
                            // 对于 Linux/macOS，可能是 file:///home/...，同样适用
                        }

                        // 从完整路径中提取纯文件名（例如从 'C:/Users/me/doc.txt' 提取 'doc.txt'）
                        var fileName = filePath.split('/').pop(); // 使用 '/' 分割，取最后一部分

                        console.log("文件URL: ", fileUrl);
                        console.log("文件路径: ", filePath);
                        console.log("文件名: ", fileName);

                        // 现在你可以使用 fileName 或 filePath 进行后续操作，例如显示、读取等
                        let musics = [];
                        //musics.push({name:fileName,path:filePath,songer:""});
                        //myfileModel.get(filePage.folderNumber).music.append(musics);
                        songModel.addSong(songModel.folderId, fileName, filePath, "");
                        mainWarn.tiped("成功导入音乐",1);
                        //filePage.loaded()
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
                    onClicked: {
                    }
                }
                SButton {
                    width: 36
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf095"
                    shadowEnabled: false
                    buttonColor: Style.themes.sideColor
                    onClicked: {
                    }
                }
                SButton {
                    width: 36
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf0c8"
                    shadowEnabled: false
                    buttonColor: Style.themes.sideColor
                    onClicked: {
                    }
                }
            }

            QButton {
                x: folderMusic.width - 136
                y: 44
                height: 40; width: 120
                radius: 20
                z: 10
                iconCharacter: "\uf0f8"
                text: "导入"
                onClicked: {
                    onClicked: musicfileDialog.open();
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
                //reuseItems: true
                headerModel: ["标题","","","菜单"]
                delegate: Rectangle {
                    id: listfile
                    height: 64
                    width: fileView.width - 16
                    radius: Style.settings.labelRadius
                    property color isBackDisplay: index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"
                    color: mainMedia.noTitle == model.name ? Style.themes.onPrimaryColor : isBackDisplay

                    Behavior on color { ColorAnimation { duration: 120 } }

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
                            text: "\uf044"
                            font.family: iconFont.name
                            font.pixelSize: Style.settings.texticon
                            color: Style.themes.fontColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Qt.rgba(0.5,0.5,0.5,0.2)
                        opacity: fileArea.containsMouse ? 1 : 0
                        z: 1
                        Behavior on opacity { NumberAnimation { duration: 80 } }
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
                        id: fileArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            mainMedia.urlLocal = true;
                            mainMedia.source = model.path;
                            mainMedia.noTitle = model.name;
                            console.log("url:", model.path);
                            mainMedia.play();
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
                            anchors.rightMargin: 20
                            spacing: 5
                            z: 2
                            y: 12
                            height: 40
                            SButton {
                                id: fileListAdd
                                iconCharacter: "\uf095"
                                width: 40
                                height: 40
                                radius: 40
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                onClicked: {
                                    var musicName = model.name;
                                    var musicPath = model.path;
                                    var listIndex = listfile.findIndexByValue(playListModel, "name", musicName);
                                    if (listIndex == -1) {
                                        playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
                                        mainWarn.tiped("成功加入播放列表",1);
                                    }
                                }
                            }
                            SButton {
                                id: fileOpen
                                iconCharacter: "\uf107"
                                width: 40
                                height: 40
                                radius: 40
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                onClicked: {
                                }
                            }
                            SButton {
                                id: fileDelete
                                iconCharacter: "\uf08e"
                                width: 40
                                height: 40
                                radius: 40
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                shadowEnabled: false
                                onClicked: {
                                    //myfileModel.get(filePage.folderNumber).music.remove(index)
                                    songModel.deleteSong(model.songId);
                                    mainWarn.tiped("成功删除一个音乐",1);
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
                    onClicked: {
                    }
                }
                SButton {
                    width: 36
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf095"
                    shadowEnabled: false
                    buttonColor: Style.themes.sideColor
                    onClicked: {
                    }
                }
                SButton {
                    width: 36
                    height: 36
                    radius: Style.settings.labelRadius
                    iconCharacter: "\uf0c8"
                    shadowEnabled: false
                    buttonColor: Style.themes.sideColor
                    onClicked: {
                    }
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
                //reuseItems: true
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
                    height: 64
                    width: localFileView.width - 16
                    radius: Style.settings.labelRadius
                    property color isBackDisplay: index % 2 === 0 ? Style.themes.blurOverlayColor : "transparent"
                    color: mainMedia.noTitle == model.fileName ? Style.themes.onPrimaryColor : isBackDisplay

                    Behavior on color { ColorAnimation { duration: 120 } }

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
                            text: "\uf044"
                            font.family: iconFont.name
                            font.pixelSize: Style.settings.texticon
                            color: Style.themes.fontColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: Style.settings.labelRadius
                        color: Qt.rgba(0.5,0.5,0.5,0.2)
                        opacity: localFileArea.containsMouse ? 1 : 0
                        z: 1
                        Behavior on opacity { NumberAnimation { duration: 80 } }
                    }


                    Label {
                        x: 80
                        y: 0
                        z: 3
                        width: 140
                        height: 64
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
                            mainMedia.urlLocal = true;
                            mainMedia.source = model.fileUrl;
                            mainMedia.noTitle = model.fileName;
                            console.log("url:", model.fileUrl);
                            mainMedia.play();
                            var musicName = model.fileName;
                            var musicPath = model.fileUrl;
                            var listIndex = listLocalFile.findIndexByValue(playListModel, "name", musicName);
                            if (listIndex == -1) {
                                playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
                                playListModel.playListIndex = playListModel.count - 1;
                            }
                        }
                        Row {
                            anchors.right: parent.right
                            anchors.rightMargin: 20
                            spacing: 5
                            z: 2
                            y: 12
                            height: 40
                            SButton {
                                iconCharacter: "\uf095"
                                width: 40
                                height: 40
                                radius: 40
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                onClicked: {
                                    var musicName = model.fileName;
                                    var musicPath = model.fileUrl;
                                    var listIndex = listLocalFile.findIndexByValue(playListModel, "name", musicName);
                                    if (listIndex == -1) {
                                        playListModel.append({ name: musicName, path: musicPath, songer: "", source: -1 });
                                    }
                                }
                            }
                            SButton {
                                iconCharacter: "\uf107"
                                width: 40
                                height: 40
                                radius: 40
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(0.5,0.5,0.5,0.2)
                                shadowEnabled: false
                                onClicked: {
                                }
                            }
                            SButton {
                                iconCharacter: "\uf08e"
                                width: 40
                                height: 40
                                radius: 40
                                buttonColor: "transparent"
                                hoverColor: Qt.rgba(1.0,0.5,0.5,0.8)
                                shadowEnabled: false
                                onClicked: {
                                    myfileModel.get(filePage.folderNumber).music.remove(index);
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
}