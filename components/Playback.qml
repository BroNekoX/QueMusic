// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick

// 播放中枢：换源防爆音 / 淡入淡出 / A-B 循环 / 睡眠定时 / 播放历史 / 真随机 / 跳转 / 音量
QtObject {
    id: root

    property var player: null
    property var queue: null
    signal playIndex(int index)
    property bool muted: false

    readonly property int count: queue ? queue.count : 0
    readonly property real outVolume: muted ? 0 : Options.settings.musicVolume * factor

    // 音量淡变：factor 是 AudioOutput 的音量乘数
    // 换源直接换 source 会在波形任意相位硬截断解码缓冲产生阶跃爆音，且
    // QAudioOutput 音量只在缓冲推入设备时生效（已排队的旧音频仍带旧音量），
    // 所以淡出后要静音保持 drainMs 排空设备缓冲，起播后再淡回。
    // 暂停/停止后 factor 保持 0 不回弹——设备缓冲里还留着安静采样，
    // 此刻把乘数跳回 1 会把它们瞬间放大出「刺」声；恢复播放统一 fadeIn()。
    property real factor: 1
    property bool armed: false          // 已静音，等待新音轨起播后淡回
    property var nextAction: null       // 静音后要执行的动作
    property var afterFade: null        // 淡变自然结束后的回调
    property bool keepSilent: false     // 停止类操作：执行后不淡回

    readonly property int fadeOutMs: 120
    readonly property int drainMs: 260
    readonly property int fadeInMs: 220
    readonly property int guardMs: 1500

    readonly property bool fadeOn: Options.settings.fadeEnabled && Options.settings.fadeMs > 0

    // 恢复音量：起播、兜底定时共用这一个入口
    function fadeIn() {
        armed = false
        guardTimer.stop()
        afterFade = null
        if (!fadeOn) {
            factor = 1
            return
        }
        fadeAnime.stop()
        fadeAnime.duration = fadeInMs
        fadeAnime.to = 1
        fadeAnime.start()
    }

    // 通用淡变：暂停/睡眠等即时过渡，不排空设备缓冲
    function fadeTo(v, then) {
        fadeAnime.stop()
        if (!fadeOn) {
            factor = v
            if (then) then()
            return
        }
        afterFade = then || null
        fadeAnime.duration = Options.settings.fadeMs
        fadeAnime.to = v
        fadeAnime.start()
    }

    // 换源/重播/停止：action 在静音状态下执行；fadeBack=false 时不淡回
    function swap(action, fadeBack) {
        if (!action) return
        nextAction = action
        keepSilent = fadeBack === false
        if (!player || !player.playing) {
            runNext()
            return
        }
        armed = true
        afterFade = function() { drainTimer.restart() }
        fadeAnime.duration = fadeOutMs
        fadeAnime.to = 0
        fadeAnime.start()
    }

    function runNext() {
        var a = nextAction
        nextAction = null
        if (a) a()
        if (keepSilent) {
            keepSilent = false
            armed = false
            guardTimer.stop()
            return
        }
        guardTimer.restart()            // 兜底：起播事件丢失也能恢复音量
        if (player && player.playing) fadeIn()
    }

    // 新音轨起播（onPlayingChanged）触发；音量未恢复就补一次淡入
    function finishSwap() {
        if (factor < 1) fadeIn()
    }

    function togglePlay() {
        if (!player) return
        if (player.playing) {
            fadeTo(0, function() { player.pause() })
            return
        }
        player.play()
        if (factor < 1) fadeIn()
    }

    property NumberAnimation fadeAnime: NumberAnimation {
        target: root
        property: "factor"
        //easing.type: Easing.InOutQuad
        onFinished: {
            var f = root.afterFade
            root.afterFade = null
            if (f) f()
        }
    }
    property Timer drainTimer: Timer {
        interval: root.drainMs
        onTriggered: root.runNext()
    }
    property Timer guardTimer: Timer {
        interval: root.guardMs
        onTriggered: root.fadeIn()
    }

    // 音量
    function setVolume(v) { Options.settings.musicVolume = Math.max(0, Math.min(1, v)) }
    function stepVolume(d) { setVolume(Options.settings.musicVolume + d) }
    function toggleMute() { muted = !muted }

    // 跳转
    function seekBy(ms) {
        if (!player) return
        player.position = Math.max(0, Math.min(player.duration, player.position + ms))
    }
    function seekBack() { seekBy(-Options.settings.seekStep * 1000) }
    function seekForward() { seekBy(Options.settings.seekStep * 1000) }

    // A-B 循环
    property int abA: -1
    property int abB: -1
    readonly property bool abArmed: abA >= 0 && abB > abA

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

    property Connections mediaHook: Connections {
        target: root.player
        function onPositionChanged() {
            if (root.abArmed && root.player.position >= root.abB)
                root.player.position = root.abA
        }
    }

    // 睡眠定时 0.关闭 1.倒计时 2.播完本首
    property int sleepMode: 0
    property int sleepRemain: 0
    readonly property string sleepLabel: sleepMode === 0 ? "关闭"
                                       : sleepMode === 2 ? "本首结束"
                                       : fmt(sleepRemain * 1000)

    function armSleep(mode, minutes) {
        sleepMode = mode
        sleepRemain = mode === 1 ? Math.max(1, minutes || Options.settings.sleepMinutes) * 60 : 0
    }
    function stopSleep() { sleepMode = 0; sleepRemain = 0 }

    function sleepEnd() {
        stopSleep()
        fadeTo(0, function() { if (player) player.pause() })
        Style.warned("睡眠定时：已停止播放", 1)
    }

    property Timer sleepTimer: Timer {
        interval: 1000
        repeat: true
        running: root.sleepMode === 1 && root.sleepRemain > 0
        onTriggered: {
            root.sleepRemain--
            if (root.sleepRemain <= 0) root.sleepEnd()
        }
    }

    // 播放历史
    property ListModel history: ListModel {}

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

    function loadHistory() {
        history.clear()
        try {
            var arr = JSON.parse(Options.settings.playHistory || "[]")
            for (var i = 0; i < arr.length; i++) history.append(arr[i])
        } catch (err) {}
    }

    // 防抖落盘：连续切歌只在静默 2s 后写一次
    function queueSave() { saveTimer.restart() }
    function flush() { saveTimer.stop(); writeHistory() }

    function writeHistory() {
        var out = []
        for (var i = 0; i < history.count; i++) {
            var e = history.get(i)
            out.push({ title: e.title, artist: e.artist, path: e.path, source: e.source,
                       cover: e.cover, duration: e.duration, time: e.time })
        }
        Options.settings.playHistory = JSON.stringify(out)
    }

    property Timer saveTimer: Timer {
        interval: 2000
        onTriggered: root.writeHistory()
    }

    // 队列操作
    function indexOfPath(p) {
        for (var i = 0; i < root.count; i++)
            if (queue.get(i).path === p) return i
        return -1
    }

    // 播放一首曲目：已在队列则直接跳转，否则追加到队尾
    function playItem(item) {
        if (!queue || !item || !item.path) return
        var i = indexOfPath(item.path)
        if (i < 0) {
            queue.append({ name: item.name, path: item.path, songer: item.songer, source: item.source })
            i = root.count - 1
        }
        goTo(i)
    }

    // 真随机：洗牌牌堆，一轮内不重复，并避开最近播放
    property var shuffleBag: []
    property var recent: []
    onCountChanged: shuffleBag = []

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

    function notePlayed(i) {
        recent.push(i)
        var keep = Math.max(0, Options.settings.shuffleAvoid)
        if (recent.length > keep) recent = recent.slice(recent.length - keep)
    }

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
    function nextShuffle() {
        if (root.count === 0) return -1
        if (root.count === 1) return 0
        if (shuffleBag.length === 0) shuffleBag = buildBag()
        return shuffleBag.pop()
    }

    function fmt(ms) {
        var s = Math.max(0, Math.floor(ms / 1000))
        return Math.floor(s / 60) + ":" + ("0" + (s % 60)).slice(-2)
    }
}
