// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick

// 播放增强：A-B 循环 / 睡眠定时 / 播放历史 / 真随机 / 精确跳转 / 静音 / 淡入淡出
QtObject {
    id: root

    property var player: null
    property var queue: null
    signal playIndex(int index)

    readonly property int count: queue ? queue.count : 0
    readonly property real outVolume: muted ? 0 : Options.settings.musicVolume * fade

    // A-B 循环
    property int abA: -1
    property int abB: -1
    readonly property bool abArmed: abA >= 0 && abB > abA

    // 睡眠定时 0.关闭 1.倒计时 2.播完本首
    property int sleepMode: 0
    property int sleepRemain: 0
    readonly property string sleepLabel: sleepMode === 0 ? "关闭"
                                       : sleepMode === 2 ? "本首结束"
                                       : fmt(sleepRemain * 1000)

    property bool muted: false
    property real fade: 1
    property var pendingAction: null

    property ListModel history: ListModel {}
    property var shuffleBag: []
    property var recent: []

    onCountChanged: shuffleBag = []

    function fmt(ms) {
        var s = Math.max(0, Math.floor(ms / 1000))
        return Math.floor(s / 60) + ":" + ("0" + (s % 60)).slice(-2)
    }

    // ---- 音量 ----
    function setVolume(v) { Options.settings.musicVolume = Math.max(0, Math.min(1, v)) }
    function stepVolume(d) { setVolume(Options.settings.musicVolume + d) }
    function toggleMute() { muted = !muted }

    // ---- 跳转 ----
    function seekBy(ms) {
        if (!player) return
        player.position = Math.max(0, Math.min(player.duration, player.position + ms))
    }
    function seekBack() { seekBy(-Options.settings.seekStep * 1000) }
    function seekForward() { seekBy(Options.settings.seekStep * 1000) }

    // ---- 淡入淡出 ----
    function fadeIn() { fade = 0; fadeTo(1) }

    function fadeTo(v, then) {
        pendingAction = null
        fadeAnime.stop()
        if (!Options.settings.fadeEnabled || Options.settings.fadeMs <= 0) {
            fade = v
            if (then) then()
            return
        }
        pendingAction = then || null
        fadeAnime.duration = Options.settings.fadeMs
        fadeAnime.to = v
        fadeAnime.start()
    }

    function togglePlay() {
        if (!player) return
        if (player.playing) {
            fadeTo(0, function() { player.pause(); fade = 1 })
            return
        }
        fade = 0
        player.play()
        fadeTo(1)
    }

    NumberAnimation {
        id: fadeAnime
        target: root
        property: "fade"
        easing.type: Easing.Linear
        onStopped: {
            var f = root.pendingAction
            root.pendingAction = null
            if (f) f()
        }
    }

    // ---- A-B 循环 ----
    function setAbPoint(which) {
        if (!player) return
        var p = player.position
        if (which === 0) {
            abA = p
            if (abB >= 0 && abB <= abA) abB = -1
        } else {
            abB = p > abA ? p : -1
        }
    }
    function clearAb() { abA = -1; abB = -1 }

    Connections {
        target: root.player
        function onPositionChanged() {
            if (root.abArmed && root.player.position >= root.abB)
                root.player.position = root.abA
        }
    }

    // ---- 睡眠定时 ----
    function armSleep(mode, minutes) {
        sleepMode = mode
        sleepRemain = mode === 1 ? Math.max(1, minutes || Options.settings.sleepMinutes) * 60 : 0
    }
    function stopSleep() { sleepMode = 0; sleepRemain = 0 }

    function sleepEnd() {
        sleepMode = 0
        sleepRemain = 0
        fadeTo(0, function() { if (player) player.pause(); fade = 1 })
        Style.warned("睡眠定时：已停止播放", 1)
    }

    Timer {
        interval: 1000
        repeat: true
        running: root.sleepMode === 1 && root.sleepRemain > 0
        onTriggered: {
            root.sleepRemain--
            if (root.sleepRemain <= 0) root.sleepEnd()
        }
    }

    // ---- 播放历史 ----
    function pushHistory(e) {
        if (history.count > 0 && history.get(0).path === e.path) {
            history.set(0, e)
            queueSave()
            return
        }
        for (var i = 1; i < history.count; i++) {
            if (history.get(i).path === e.path) {
                history.remove(i, 1)
                break
            }
        }
        history.insert(0, e)
        var limit = Math.max(20, Options.settings.historyLimit)
        if (history.count > limit) history.remove(limit, history.count - limit)
        queueSave()
    }

    function clearHistory() { history.clear(); queueSave() }

    function queueSave() { saveTimer.restart() }

    function flush() {
        saveTimer.stop()
        writeHistory()
    }

    function writeHistory() {
        var out = []
        for (var i = 0; i < history.count; i++) {
            var e = history.get(i)
            out.push({ title: e.title, artist: e.artist, path: e.path, source: e.source,
                       cover: e.cover, duration: e.duration, time: e.time })
        }
        Options.settings.playHistory = JSON.stringify(out)
    }

    function loadHistory() {
        history.clear()
        try {
            var arr = JSON.parse(Options.settings.playHistory || "[]")
            for (var i = 0; i < arr.length; i++) history.append(arr[i])
        } catch (err) {}
    }

    Timer {
        id: saveTimer
        interval: 2000
        onTriggered: root.writeHistory()
    }

    // ---- 随机牌堆 ----
    function buildBag() {
        var n = root.count
        var cur = queue ? queue.playListIndex : -1
        var i, bag = []
        for (i = 0; i < n; i++)
            if (i !== cur && recent.indexOf(i) === -1) bag.push(i)
        if (bag.length === 0) for (i = 0; i < n; i++) bag.push(i)
        for (var k = bag.length - 1; k > 0; k--) {
            var r = Math.floor(Math.random() * (k + 1))
            var t = bag[k]; bag[k] = bag[r]; bag[r] = t
        }
        return bag
    }

    function nextShuffle() {
        if (root.count === 0) return -1
        if (root.count === 1) return 0
        if (shuffleBag.length === 0) shuffleBag = buildBag()
        return shuffleBag.pop()
    }

    function notePlayed(i) {
        recent.push(i)
        var keep = Math.max(0, Options.settings.shuffleAvoid)
        if (recent.length > keep) recent = recent.slice(recent.length - keep)
    }

    // ---- 切歌 ----
    function goTo(i) {
        if (i < 0 || i >= root.count) return
        queue.playListIndex = i
        notePlayed(i)
        playIndex(i)
    }
    function next(random) {
        if (root.count === 0) return
        if (random) { goTo(nextShuffle()); return }
        var i = queue.playListIndex
        goTo(i + 1 >= root.count ? 0 : i + 1)
    }
    function previous() {
        if (root.count === 0) return
        var i = queue.playListIndex
        goTo(i - 1 < 0 ? root.count - 1 : i - 1)
    }
}
