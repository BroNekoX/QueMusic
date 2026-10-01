// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 播放中枢：切歌 / 淡变防爆音 / 音量 / 跳转 / A-B 循环 / 睡眠定时 / 播放历史 / 真随机
pragma Singleton
import QtQuick
import QueMusic 1.0

QtObject {
    id: root

    property AudioEngine player
    property QueueModel queue

    // 当前曲目的标题/歌手与待读歌词路径：原先散在 main.qml，靠 window.xxx 跨文件来回传
    property string musicTitle: "QueMusic"
    property string musicArtist: ""
    // 当前曲目的身份标识（评论等功能按它反查平台）：hash 用未升级品质的原始值
    property string musicHash: ""
    property int musicSource: -1
    property string localLyricsRequestPath: ""
    property int autoSkipCount: 0

    readonly property int count: queue ? queue.count : 0

    // 循环模式（列表循环/单曲循环/随机播放/暂停操作）：持久化的值可能越界，统一钳到 0..3
    readonly property int cycleIndex: Math.max(0, Math.min(3, Options.settings.cycleIndex))
    readonly property string cycleIcon: ["\uf118", "\uf115", "\uf0e2", "\uf03b"][root.cycleIndex]
    readonly property string cycleTip: ["列表循环", "单曲循环", "随机播放", "暂停操作"][root.cycleIndex]

    property bool muted: false
    readonly property real outVolume: muted ? 0 : Options.settings.musicVolume

    // 唯一出口：设索引 + 记录播放历史 + 直接起播
    function goTo(i: int): void {
        if (i < 0 || i >= count) return
        queue.playListIndex = i
        notePlayed(i)
        startTrack(i)
    }

    function next(random: bool): void {
        if (count === 0) return
        const cur = queue.playListIndex
        goTo(random ? nextShuffle() : (cur + 1 >= count ? 0 : cur + 1))
    }

    function previous(): void {
        if (count === 0) return
        const cur = queue.playListIndex
        goTo(cur > 0 ? cur - 1 : count - 1)
    }

    // 追加到队尾；已在队列返回 false
    function enqueue(item: var): bool {
        if (!queue || !item || !item.path || indexOfPath(item.path) !== -1) return false
        queue.append({ name: item.name, path: item.path, songer: item.songer, source: item.source })
        return true
    }

    // 插到当前曲目之后；没有当前曲目时追加到队尾
    function playNext(item: var): void {
        if (!queue || !item || !item.path) return
        const at = queue.playListIndex + 1
        queue.insert(at <= 0 ? count : at,
                     { name: item.name, path: item.path, songer: item.songer, source: item.source })
    }

    // 已在队列则直接跳转，否则追加到队尾再起播
    function playItem(item: var): void {
        if (!queue || !item || !item.path) return
        let i = indexOfPath(item.path)
        if (i < 0) {
            if (!enqueue(item)) return
            i = count - 1
        }
        goTo(i)
    }

    // WebDAV：已缓存则直接播本地；否则带鉴权头直连，同时后台缓存（落地后回填元数据/封面/歌词）
    function playWebDav(url: string, name: string): void {
        const cached = WebDavCache.localPathFor(url)
        if (cached !== "") {
            playLocalSong(cached, name)
            return
        }
        localLyricsRequestPath = ""
        player.noTitle = name
        musicTitle = name
        musicArtist = ""
        MusicApi.lyricsData = []
        MusicApi.lyricsTranslate = []
        const auth = WebDav.authHeaderOfUrl(url)
        player.sourceHeaders = auth !== "" ? { "Authorization": auth } : {}
        const sidecars = WebDav.sidecarsOf(url)
        WebDavCache.cache(url, auth, sidecars.lyricsUrl || "", sidecars.coverUrl || "")
        swap(function() {
            player.source = url
            player.play()
        })
    }



    function startTrack(index: int): void {
        const e = queue.get(index)
        // 用 == 而非 ===，且不要对 e 取反：AOT 下这两种写法会导致切歌闪退
        if (e.path == undefined) return
        if (e.source == 3) playWebDav(e.path, e.name)
        else if (e.source < 0) playLocalSong(e.path, e.name)
        else MusicApi.getMusicInfo(e.path, 0, e.source)
    }

    function noteNowPlaying(): void {
        if (queue.playListIndex < 0 || count === 0 || !musicTitle) return
        const e = queue.get(queue.playListIndex)
        pushHistory({ title: musicTitle, artist: musicArtist, path: e.path, source: e.source,
cover: player.urlStr || "",
                      duration: player ? Math.floor(player.duration / 1000) : 0, time: Date.now() })
    }

    // 本地播放：立即起播不阻塞，元数据/封面/歌词就绪后回填
    function playLocalSong(path: string, name: string): void {
        localLyricsRequestPath = path
        player.noTitle = name
        musicTitle = name
        musicArtist = ""
        MusicApi.lyricsData = []
        MusicApi.lyricsTranslate = []
        MusicApi.readLocalMetadataAsync(path)
        swap(function() {
            player.source = path
            player.play()
        })
    }

    function indexOfPath(p: string): int { return queue ? queue.indexOfPath(p) : -1 }

    // 真随机：洗牌牌堆一轮不重复，并避开最近播放
    property var bag: []
    property var recent: []
    onCountChanged: bag = []

    function nextShuffle(): int {
        if (count < 2) return 0
        const keep = Math.max(0, Options.settings.shuffleAvoid)
        if (bag.length === 0) {
            const skip = recent.slice(recent.length - keep)
            const cur = queue ? queue.playListIndex : -1
            bag = []
            for (let i = 0; i < count; i++)
                if (i !== cur && skip.indexOf(i) === -1) bag.push(i)
            if (bag.length === 0)
                for (let i = 0; i < count; i++) bag.push(i)
            for (let k = bag.length - 1; k > 0; k--) {
                const r = Math.floor(Math.random() * (k + 1))
                const t = bag[k]; bag[k] = bag[r]; bag[r] = t
            }
        }
        return bag.pop()
    }

    function notePlayed(i: int): void {
        recent.push(i)
        const keep = Math.max(0, Options.settings.shuffleAvoid)
        if (recent.length > keep) recent = recent.slice(recent.length - keep)
    }

    readonly property int fadeOutMs: 120
    readonly property int fadeInMs: 220
    readonly property int guardMs: 1500
    readonly property bool fadeOn: Options.settings.fadeEnabled && Options.settings.fadeMs > 0

    property bool armed: false          // 已静音，等待新音轨起播后淡回
    property var nextAction: null       // 静音后要执行的动作
    property var afterFade: null        // 淡变结束后的回调
    property bool keepSilent: false     // 停止类操作：执行后不淡回

    // 淡入：引擎在新数据段真正出声的那一刻开始推进
    function fadeIn(): void {
        armed = false
        guardTimer.stop()
        afterFade = null
        if (player) player.fadeInOnNextAudio(fadeOn ? fadeInMs : 0)
    }

    // 通用淡变：暂停/睡眠等即时过渡
    function fadeTo(v: real, then: var): void {
        afterFade = then || null
        if (player) player.fadeTo(v, fadeOn ? Options.settings.fadeMs : 0)
        if (then) {
            fadeTimer.interval = fadeOn ? Options.settings.fadeMs : 0
            fadeTimer.restart()
        }
    }

    // 换源/重播/停止：action 在静音后执行；fadeBack=false 时不淡回
    function swap(action: var, fadeBack: var): void {
        if (!action) return
        nextAction = action
        keepSilent = fadeBack === false
        if (!player || !player.playing || !fadeOn) { runNext(); return }
        armed = true
        player.fadeOut(fadeOutMs)
        swapTimer.restart()
    }

    function runNext(): void {
        const a = nextAction
        nextAction = null
        if (a) a()
        if (keepSilent) {
            keepSilent = false
            armed = false
            guardTimer.stop()
            return
        }
        guardTimer.restart()            // 兜底：起播事件丢失也能恢复音量
        armFadeIn()
    }

    // 补一次淡入（不碰 guardTimer，供 runNext 与兜底定时器共用）
    function armFadeIn(): void {
        if (!player) return
        player.fadeInOnNextAudio(fadeOn ? fadeInMs : 0)
    }

    function finishSwap(): void { armed = false }

    function togglePlay(): void {
        if (!player) return
        if (player.playing) { fadeTo(0, function() { player.pause() }); return }
        player.play()
    }

    property Timer swapTimer: Timer {
        interval: root.fadeOutMs
        onTriggered: root.runNext()
    }
    property Timer fadeTimer: Timer {
        onTriggered: {
            const f = root.afterFade
            root.afterFade = null
            if (f) f()
        }
    }
    property Timer guardTimer: Timer {
        interval: root.guardMs
        // 只调用 root 的函数，不在嵌套对象里直接访问属性（与备份同构）
        onTriggered: root.armFadeIn()
    }

    // 音量
    readonly property real volumeStep: Options.settings.volumeStep / 100
    function setVolume(v: real): void { Options.settings.musicVolume = Math.max(0, Math.min(1, v)) }
    function stepVolume(d: real): void { setVolume(Options.settings.musicVolume + d) }
    function toggleMute(): void { muted = !muted }

    // 跳转
    function seekBy(ms: int): void {
        if (!player) return
        player.position = Math.max(0, Math.min(player.duration, player.position + ms))
    }
    function seekBack(): void { seekBy(-Options.settings.seekStep * 1000) }
    function seekForward(): void { seekBy(Options.settings.seekStep * 1000) }

    // A-B 循环
    property int abA: -1
    property int abB: -1
    readonly property bool abArmed: abA >= 0 && abB > abA

    function setAbPoint(which: int): void {
        if (!player) return
        const p = player.position
        if (which === 0) {
            abA = p
            if (abB >= 0 && abB <= abA) abB = -1
        } else {
            abB = p > abA ? p : -1
        }
    }
    function clearAb(): void { abA = -1; abB = -1 }

    property Connections mediaHook: Connections {
        target: root.player
        function onPositionChanged(): void {
            if (root.abArmed && root.player.position >= root.abB)
                root.player.position = root.abA
        }
    }

    // 睡眠定时
    // 0.关闭 1.倒计时 2.播完本首
    property int sleepMode: 0
    property int sleepRemain: 0
    readonly property string sleepLabel: sleepMode === 0 ? "关闭"
                                       : sleepMode === 2 ? "本首结束"
                                       : fmt(sleepRemain * 1000)

    function armSleep(mode: int, minutes: var): void {
        sleepMode = mode
        sleepRemain = mode === 1 ? Math.max(1, minutes || Options.settings.sleepMinutes) * 60 : 0
    }
    function stopSleep(): void { sleepMode = 0; sleepRemain = 0 }

    function sleepEnd(): void {
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

    function pushHistory(e: var): void {
        if (history.count > 0 && history.get(0).path === e.path) {
            history.set(0, e)
            queueSave()
            return
        }
        for (let i = 1; i < history.count; i++) {
            if (history.get(i).path === e.path) { history.remove(i, 1); break }
        }
        history.insert(0, e)
        const limit = Math.max(20, Options.settings.historyLimit)
        if (history.count > limit) history.remove(limit, history.count - limit)
        queueSave()
    }

    function saveQueue(): void {
        const out = []
        for (let i = 0; i < count; i++) {
            const e = queue.get(i)
            out.push({ name: e.name, path: e.path, songer: e.songer, source: e.source })
        }
        Options.settings.lastQueue = JSON.stringify(out)
        Options.settings.lastQueueIndex = queue.playListIndex
    }
    function restoreSession(): void {
        if (!Options.settings.autoRestoreQueue) return
        try {
            const arr = JSON.parse(Options.settings.lastQueue || "[]")
            for (let i = 0; i < arr.length; i++) queue.append(arr[i])
            const idx = Options.settings.lastQueueIndex
            if (idx < 0 || idx >= count) return
            queue.playListIndex = idx
        } catch (err) {}
    }
    function clearHistory(): void { history.clear(); queueSave() }

    function loadHistory(): void {
        history.clear()
        try {
            const arr = JSON.parse(Options.settings.playHistory || "[]")
            for (let i = 0; i < arr.length; i++) history.append(arr[i])
        } catch (err) {}
    }

    // 防抖落盘：连续切歌只在静默 2s 后写一次
    function queueSave(): void { saveTimer.restart() }
    function flush(): void { saveTimer.stop(); writeHistory() }

    function writeHistory(): void {
        const out = []
        for (let i = 0; i < history.count; i++) {
            const e = history.get(i)
            out.push({ title: e.title, artist: e.artist, path: e.path, source: e.source,
                       cover: e.cover, duration: e.duration, time: e.time })
        }
        Options.settings.playHistory = JSON.stringify(out)
    }

    property Timer saveTimer: Timer {
        interval: 2000
        onTriggered: root.writeHistory()
    }

    // 工具：时长格式化（mm:ss）。全应用只此一份，桌面部件的本地副本已删
    function fmt(ms: real): string {
        if (isNaN(ms) || ms < 0)
            return "00:00"
        const s = Math.floor(ms / 1000)
        const m = Math.floor(s / 60)
        return (m < 10 ? "0" : "") + m + ":" + ("0" + (s % 60)).slice(-2)
    }
}
