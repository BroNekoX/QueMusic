// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 歌曲收藏夹选择：全应用共用一份，各收藏按钮调 Options.favPicker.show(item)。
pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

QOptionDialog {
    id: root
    title: "收藏到"
    cancelText: "取消收藏"
    cancelIcon: "\uf025"
    confirmText: "完成"
    headerHeight: 24
    property var item

    function show(song: var): void {
        item = song
        open()
    }

    // 取消收藏：收藏过就撤掉，没收藏就不管
    onCancel: {
        if (root.item && FavoriteSongs.isFavorite(root.item.id, "song")) {
            FavoriteSongs.removeFavorite(root.item.id, "song")
            Options.warned("已取消收藏", 0)
        }
    }

    options: Column {
        width: root.width - 40
        spacing: 4
        Repeater {
            model: FavoriteFolders
            delegate: Rectangle {
                id: folderRow
                required property int folderId
                required property string name
                required property bool isDefault
                width: parent.width
                height: 48
                radius: Style.labelRadius
                color: folderArea.containsMouse ? Theme.hoverColor : "transparent"
                Behavior on color { ColorAnimation { duration: 80 } }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 14
                    text: "\uf0f5"
                    font.family: Fonts.icon
                    font.pixelSize: Style.texticon
                    color: Theme.textColor
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 52
                    width: parent.width - 66
                    text: folderRow.name
                    color: Theme.fontColor
                    font.pixelSize: Style.textmain
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: folderArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (!root.item)
                            return
                        FavoriteSongs.addToFolder(root.item.id, root.item.title, root.item.artist,
                                                  root.item.cover, root.item.source,
                                                  root.item.duration || 0, folderRow.folderId)
                        Options.warned("已收藏到「" + folderRow.name + "」", 1)
                        root.close()
                    }
                }
            }
        }
        Text {
            visible: FavoriteFolders.count === 0
            width: parent.width
            height: 48
            text: "暂无收藏夹"
            color: Theme.textColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
