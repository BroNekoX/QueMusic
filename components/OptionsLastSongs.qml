// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 最后播放的歌曲记录（有类型，供 qmlcachegen 做 AOT 编译）
import QtCore

Settings {
    category: "LastMedia"

    property string name: ""
    property string artist: ""
    property string cover: ""
    property string hash: ""
    property int source: -1
    property int position: 0
}
