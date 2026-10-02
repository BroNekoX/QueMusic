// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 系统媒体控件（Windows SMTC）：向系统推送播放状态与曲目信息，并接收系统的播放/暂停/切歌/跳转请求。
// 原生 WinRT 调用会同步回调进 QML，故统一经 Qt.callLater 延后，避免重入切歌逻辑。
import QtQuick
import QueMusic 1.0

QtObject {
    id: root

    property AudioEngine player
    property QueueModel queue
    property var targetWindow

    property bool queued: false
    property bool infoQueued: false
    property real lastTimeline: 0
    property real lastPosition: 0

    function push(): void {
        if (queued)
            return
        queued = true
        Qt.callLater(function() {
            root.queued = false
            root.updateControls()
            if (!mgr.available || !player)
                return
            if (player.duration > 0)
                mgr.updateTimeline(0, player.duration)
            if (player.playbackState === AudioEngine.PlayingState)
                mgr.setPlaybackStatus(WindowsSmtcManager.Playing)
            else if (player.playbackState === AudioEngine.PausedState)
                mgr.setPlaybackStatus(WindowsSmtcManager.Paused)
            else if (player.playbackState === AudioEngine.StoppedState)
                mgr.setPlaybackStatus(WindowsSmtcManager.Stopped)
            else
                mgr.setPlaybackStatus(WindowsSmtcManager.Closed)
        })
    }

    function pushInfo(): void {
        if (infoQueued)
            return
        infoQueued = true
        Qt.callLater(function() {
            root.infoQueued = false
            if (!mgr.available || !player)
                return
            let mediaId = ""
            if (queue && queue.count > 0 && queue.playListIndex >= 0) {
                const item = queue.get(queue.playListIndex)
                if (item)
                    mediaId = item.path
            }
            mgr.updateMediaInfo(Playback.musicTitle, Playback.musicArtist, player.album,
                                player.urlStr, mediaId)
        })
    }

    function updateControls(): void {
        if (!mgr.available || !queue)
            return
        mgr.setControlsEnabled(true, true, queue.playListIndex < queue.count - 1,
                               queue.playListIndex > 0)
    }

    property WindowsSmtcManager mgr: WindowsSmtcManager {
        id: mgr

        Component.onCompleted: {
            initialize(root.targetWindow)
            root.updateControls()
        }

        onPlayPressed: Playback.togglePlay()
        onPausePressed: Playback.togglePlay()
        onNextPressed: Playback.next(false)
        onPreviousPressed: Playback.previous()
        onSeekRequested: (pos) => {
            if (root.player)
                root.player.position = pos
        }
    }

    property Connections playerWatch: Connections {
        target: root.player

        function onSourceChanged(): void {
            Playback.clearAb()
            root.push()
        }
        function onDurationChanged(): void {
            root.push()
        }
        function onMetaDataChanged(): void {
            root.pushInfo()
        }
        function onPlaybackStateChanged(): void {
            root.push()
        }
        function onPositionChanged(): void {
            if (!mgr.available)
                return
            const now = Date.now()
            const jump = Math.abs(root.player.position - root.lastPosition)
            if (now - root.lastTimeline < 5000 && jump < 3000
                    && root.player.duration - root.player.position > 5000)
                return
            root.lastTimeline = now
            root.lastPosition = root.player.position
            mgr.updateTimeline(root.player.position, root.player.duration)
        }
    }

    property Connections metaWatch: Connections {
        target: Playback
        function onMusicTitleChanged(): void { root.pushInfo() }
        function onMusicArtistChanged(): void { root.pushInfo() }
    }

    property Connections queueWatch: Connections {
        target: root.queue
        function onCountChanged(): void { root.push() }
        function onPlayListIndexChanged(): void { root.push() }
    }
}
