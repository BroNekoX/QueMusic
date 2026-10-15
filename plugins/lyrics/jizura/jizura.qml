// SPDX-License-Identifier: MIT
//
// JIZURA × QueMusic — lyric interface plugin entry point.
//
// JIZURA (MIT, https://github.com/hakoniwa/JIZURA) arranges a lyric file into a
// motion-designed PV.  Its arrangement engine runs unmodified inside QML; every
// frame is recorded into a display list that QtQuick.Shapes draws.
//
//   js/jizura.bundle.js  JIZURA runtime + the Canvas2D shim (generated)
//   qml/JzStage.qml      the render stage: five Shape/Text layers
//   qml/JzLayer.qml      the per-layer pools
//   qml/JzMetrics.qml    Qt font metrics for JIZURA's text layouts
//   qml/JzOptions.qml    the settings block shown in the host's dialog
//
// Host contract: declare the properties you use, implement timerFunction().

pragma ComponentBehavior: Bound

import QtQuick
import QtCore
import QtQuick.Shapes

import "qml"
import "js/jizura.bundle.js" as Jz
import "js/labels.js" as Labels

Item {
    id: root

    // ------------------------------------------------------- host injection
    property real position: 0
    property bool playing: false
    property bool mediaActive: false
    property real playbackRate: 1
    property var lyricsModel: []
    property var translateModel: []
    property int currentIndex: 0
    property int lyricMove: 0
    property string title: ""
    property string artist: ""
    property string coverUrl: ""
    property color mainColor: "#00ee66"
    property color secondColor: "#00b1ee"
    property color thirdColor: "#9d4edd"
    property int lyricSize: 10
    property int hideHeight: 0
    property bool openTranslate: true
    property bool basicCd: false
    property int lyricType: 0
    property var requestStyle

    function request(k, v): void {
        if (requestStyle)
            requestStyle(k, v);
    }

    // the host calls this every 320 ms; the render loop is frame-driven, so
    // only the lyric index needs checking here (it drives a cheap re-plan test).
    function timerFunction(): void {
        if (!mediaActive)
            return;
        checkPlan();
    }

    // ------------------------------------------------------------- settings
    // The panel (host-owned, may outlive this plugin) and this item never
    // reference each other; both read and write this settings category and the
    // plugin pulls its own copy on a timer.  A QML Settings only notifies about
    // its own writes, hence the pull.
    readonly property string settingsCategory: "JizuraLyrics"
    property string styleKey: "noir"
    property string qualityMode: "auto"
    property bool useCoverColors: true
    property bool unify: false
    property bool typeset: false
    property int seedMode: 1
    property int fixedSeed: 20260922
    property real fxMotion: 0.7
    property real fxGlitch: 0.55
    property real fxChroma: 0.7
    property real fxDecor: 0.5
    property real fxDensity: 0.55
    property real textScale: 1.0
    property string komaMode: "smooth"     // smooth (every frame) | onTwos | auto

    function pullSettings(): void {
        settings.sync();                       // pick up writes made elsewhere
        if (styleKey !== settings.styleKey) styleKey = settings.styleKey;
        if (qualityMode !== settings.qualityMode) qualityMode = settings.qualityMode;
        if (useCoverColors !== settings.useCoverColors) useCoverColors = settings.useCoverColors;
        if (unify !== settings.unify) unify = settings.unify;
        if (typeset !== settings.typeset) typeset = settings.typeset;
        if (seedMode !== settings.seedMode) seedMode = settings.seedMode;
        if (fixedSeed !== settings.fixedSeed) fixedSeed = settings.fixedSeed;
        if (fxMotion !== settings.fxMotion) fxMotion = settings.fxMotion;
        if (fxGlitch !== settings.fxGlitch) fxGlitch = settings.fxGlitch;
        if (fxChroma !== settings.fxChroma) fxChroma = settings.fxChroma;
        if (fxDecor !== settings.fxDecor) fxDecor = settings.fxDecor;
        if (fxDensity !== settings.fxDensity) fxDensity = settings.fxDensity;
        if (textScale !== settings.textScale) textScale = settings.textScale;
        if (rendererMode !== settings.rendererMode) rendererMode = settings.rendererMode;
        if (komaMode !== settings.komaMode) komaMode = settings.komaMode;
    }

    Rectangle {
        z: 5
        x: 0
        y: 0 - root.hideHeight
        width: root.width
        height: 75
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#66000000" }
            GradientStop { position: 1.0; color: "transparent" }
        }

    }

    Timer {
        interval: 250
        repeat: true
        running: root.live
        onTriggered: root.pullSettings()
    }

    Settings {
        id: settings
        category: root.settingsCategory
        property string styleKey: "noir"
        property string qualityMode: "auto"      // auto | high | medium | low
        property bool useCoverColors: true
        property bool unify: false
        property bool typeset: false
        property int seedMode: 1                 // 0 fixed, 1 per song, 2 new each play
        property int fixedSeed: 20260922
        property real fxMotion: 0.7
        property real fxGlitch: 0.55
        property real fxChroma: 0.7
        property real fxDecor: 0.5
        property real fxDensity: 0.55
        property real textScale: 1.0
        property string rendererMode: "geometry"   // Qt's own default Shape backend
        property string komaMode: "smooth"         // smooth | onTwos | auto
    }

    // ----------------------------------------------------------------- state
    readonly property var jz: Jz.JZB.JZ
    property var plan: null
    property string plannedKey: ""
    property int planAttempts: 0
    property bool planFailed: false
    property int qualityLevel: 0                 // 0 full, 1 no ghosts, 2 no transitions/post
    property real frameEma: 0
    property real lastMs: 0
    property real recordMs: 0
    property real applyMs: 0
    property int cooldown: 0
    property bool ready: false
    property string pendingKey: ""
    property string pendingAspect: ""     // JZ.aspectFor() returns a name like "16:9", not a number

    // see checkPlan(): the plan is only built once the inputs stop changing
    Timer {
        id: planDebounce
        interval: 220
        repeat: false
        onTriggered: root.buildPending()
    }

    Component.onCompleted: {
        jz.install({ fontFamilies: Qt.fontFamilies(), metrics: metrics });
        root.pullSettings();
        shapeRenderer = rendererMode === "curve" ? Shape.CurveRenderer : Shape.GeometryRenderer;
        ready = true;
        checkPlan();
    }

    Component.onDestruction: {
        live = false;
        plan = null;
        jz.dispose();
    }

    // render loop switch (false while paused / unloading)
    property bool live: true

    property string rendererMode: settings.rendererMode
    property int shapeRenderer: Shape.GeometryRenderer
    onRendererModeChanged: {
        stage.release();
        shapeRenderer = rendererMode === "curve" ? Shape.CurveRenderer : Shape.GeometryRenderer;
    }

    JzMetrics { id: metrics }

    // C++ 计算内核 (crates/cpp/lyricpvcore 的 LyricPvCore)。实测路径->SVG 与颜色
    // 归一化合计占一帧 ~40%, 放在 C++ 里做。加载失败时 (宿主没链接该模块) 只有
    // 这一个 Item 失败, PV 退回纯 JS 计算, 不会整个主题起不来。
    Loader {
        id: coreLoader
        source: Qt.resolvedUrl("qml/JzCore.qml")
        asynchronous: false
        onLoaded: {
            const ok = root.jz.setCore(item.core);
            console.log("JIZURA: LyricPvCore " + (ok ? "已启用（路径/颜色计算在 C++）"
                                                    : "不可用，退回 JS 计算"));
        }
        onStatusChanged: {
            if (status === Loader.Error) {
                console.warn("JIZURA: LyricPvCore 不可用，计算退回 JS");
                root.jz.setCore(null);
            }
        }
    }

    // ------------------------------------------------------------- rendering
    JzStage {
        id: stage
        anchors.fill: parent
        cover: true
        contentScale: root.compositionScale
        shapeRenderer: root.shapeRenderer
    }

    // lyricSize (host, 0..20) and textScale (plugin) both nudge the overall
    // composition; JIZURA sizes its own text to the frame.
    readonly property real compositionScale: textScale * (0.92 + Math.max(0, Math.min(20, lyricSize)) * 0.008)
    readonly property int liveShapes: stage.liveShapes
    readonly property int liveTexts: stage.liveTexts

    // the design box has to be known before the planner runs
    property int designW: 1920
    property int designH: 1080

    Binding {
        target: stage
        property: "designW"
        value: root.designW
    }
    Binding {
        target: stage
        property: "designH"
        value: root.designH
    }

    FrameAnimation {
        running: root.live && root.plan !== null && root.mediaActive && (root.playing || root.seekPending)
        onTriggered: root.render()
    }

    // when paused, a seek still needs one frame
    property bool seekPending: false
    onPositionChanged: {
        if (!playing && mediaActive) {
            seekPending = true;
            idle.restart();
        }
    }
    Timer {
        id: idle
        interval: 24
        repeat: false
        onTriggered: {
            root.render();
            root.seekPending = false;
        }
    }

    function render(): void {
        if (!plan)
            return;
        const t0 = Date.now();
        const t = Math.max(0, (position + lyricMove)) / 1000;
        const list = jz.frame(t, {
            noGhost: qualityLevel >= 1,
            noTrans: qualityLevel >= 2,
            noPost: qualityLevel >= 2
        });
        const t1 = Date.now();
        stage.apply(list);
        const t2 = Date.now();
        recordMs = t1 - t0;
        applyMs = t2 - t1;
        lastMs = t2 - t0;
        frameEma = frameEma === 0 ? lastMs : frameEma * 0.9 + lastMs * 0.1;
        if (trackWorst && lastMs > worstMs) {
            worstMs = lastMs;
            worstInfo = jz.describeCut(t);
        }
        adapt();
    }

    property bool trackWorst: true
    property real worstMs: 0
    property string worstInfo: ""
    function resetWorst(): void {
        worstMs = 0;
        worstInfo = "";
    }

    function budgetFor(q): int {
        return q === 0 ? 1600 : q === 1 ? 900 : q === 2 ? 480 : 260;
    }
    function textBudgetFor(q): int {
        return q === 0 ? 300 : q === 1 ? 220 : 160;
    }

    function applyQuality(q): void {
        qualityLevel = q;
        jz.setBudget(budgetFor(q));
        jz.setTextBudget(textBudgetFor(q));
        jz.setBackgrounds(q < 3);
        jz.setDecorations(q < 4);
    }

    function qualityForMode(m): int {
        if (m === "high") return 0;
        if (m === "medium") return 2;
        if (m === "low") return 4;
        return qualityLevel;
    }

    function adapt(): void {
        if (qualityMode !== "auto") {
            const q = qualityForMode(qualityMode);
            if (q !== qualityLevel)
                applyQuality(q);
            return;
        }
        if (cooldown > 0) {
            cooldown--;
            return;
        }
        // Step up on a single bad frame; step down only with real headroom and a
        // long cooldown, so the stage prefers being smooth over being pretty.
        if ((frameEma > 4.0 || lastMs > 10) && qualityLevel < 4) {
            applyQuality(qualityLevel + 1);
            cooldown = 45;
        } else if (frameEma < 1.5 && lastMs < 4 && qualityLevel > 0) {
            applyQuality(qualityLevel - 1);
            cooldown = 420;
        }
    }

    // ---------------------------------------------------------------- planning
    // Re-plan only when the song or the settings actually change; JIZURA plans
    // are absolute-time, so seeking never needs a rebuild.
    function lyricsKey(): string {
        const m = lyricsModel;
        if (!m || m.length === 0)
            return "";
        return m.length + ":" + (m[0] ? m[0].time : 0) + ":" + (m[m.length - 1] ? m[m.length - 1].time : 0)
             + ":" + (m[0] ? m[0].text : "");
    }

    function settingsKey(): string {
        return [styleKey, useCoverColors, unify, typeset, seedMode, fixedSeed,
                fxMotion, fxGlitch, fxChroma, fxDecor, fxDensity].join("|");
    }

    function checkPlan(): void {
        if (!ready || !mediaActive) {
            planDebounce.stop();
            pendingKey = "";
            plan = null;
            stage.clear();
            plannedKey = "";
            return;
        }
        const aspect = jz.aspectFor(width > 0 ? width : 16, height > 0 ? height : 9);
        const key = lyricsKey() + "||" + settingsKey() + "||" + aspect + "||" + title + "||" + artist;
        if (key === plannedKey) {
            planDebounce.stop();
            pendingKey = "";
            return;
        }
        if (key === pendingKey)
            return;                       // unchanged since the last probe: let the timer run
        pendingKey = key;
        pendingAspect = aspect;
        planDebounce.restart();
    }

    function buildPending(): void {
        if (!ready || !mediaActive || !pendingKey)
            return;
        plannedKey = pendingKey;
        build(pendingAspect);
    }

    function build(aspect): void {
        plan = null;
        planFailed = false;
        const lines = [];
        const m = lyricsModel;
        for (let i = 0; i < m.length; i++) {
            const l = m[i];
            if (!l || l.isOther)
                continue;
            const txt = (l.text || "").trim();
            if (!txt)
                continue;
            lines.push({ time: l.time || 0, text: txt });
        }
        if (lines.length < 2) {
            stage.clear();
            return;
        }

        qualityLevel = -1;
        // auto starts one step down and earns its way back up
        applyQuality(qualityMode === "high" ? 0 : qualityMode === "medium" ? 2 : qualityMode === "low" ? 4 : 2);
        cooldown = 0;
        frameEma = 0;

        const d = Jz.JZB.J.designSize(aspect);
        designW = d[0];
        designH = d[1];

        // a stable per-song seed keeps a song looking the same on every replay
        const seed = seedMode === 0 ? fixedSeed
                   : seedMode === 1 ? (hash(title + "|" + artist) >>> 0)
                   : (Date.now() & 0x7fffffff);

        const colors = useCoverColors
            ? { enabled: false, accentOn: true,
                accent: cssColor(mainColor), ghostA: cssColor(secondColor), ghostB: cssColor(thirdColor) }
            : null;

        // an unknown style key (e.g. a stale setting from a removed pack) would
        // make the planner fall over, so fall back to noir
        const wantStyle = Jz.JZB.J.STYLES[styleKey] ? styleKey : "noir";
        const project = jz.makeProject({
            lyrics: jz.lyricsToLrc(lines),
            title: title, artist: artist, style: wantStyle,
            seed: seed, aspect: aspect, fps: 60, lang: "auto",
            unify: unify, typeset: typeset,
            colors: colors,
            fx: { motion: fxMotion, glitch: fxGlitch, chroma: fxChroma, decor: fxDecor, density: fxDensity }
        });

        const res = jz.plan(project, null);
        planAttempts++;
        if (!res.plan) {
            planFailed = true;
            stage.clear();
            return;
        }
        plan = res.plan;
        applyKoma();
        render();
    }

    // JIZURA quantises all motion to `koma` drawings per second — the anime
    // "on twos" trick, 12 or 8, picked per mood by the planner — and its
    // renderer still re-records the whole frame on every tick, so quantising
    // saves no work at all: it only caps the picture at 8-12 updates/s while
    // the app's own fps counter keeps showing the (full) render rate.  koma is
    // read at draw time, so overriding it here is enough.
    function applyKoma(): void {
        if (!plan || !plan.fx)
            return;
        if (komaMode === "onTwos") {
            plan.fx.koma = 12;
            plan.fx.onTwos = true;
        } else if (komaMode === "smooth") {
            // both knobs: J.komaOf() treats a missing koma as "12 unless
            // onTwos === false", so set the flag as well as the number
            plan.fx.koma = 0;
            plan.fx.onTwos = false;
        }
        // "auto" keeps the mood's own choice
    }

    // steps per second the picture really advances at: exactly the reciprocal
    // of J.stepDur(), which is what the renderer quantises its time with.
    // A function, not a binding: applyKoma() mutates plan.fx after `plan` has
    // already been assigned, so a binding on `plan` would report the planner's
    // original value and never refresh.
    function pictureStepsNow(): real {
        return (plan && plan.fx) ? 1 / Jz.JZB.J.stepDur(plan.fx, plan.fps || 60) : 0;
    }

    onKomaModeChanged: {
        applyKoma();
        if (plan)
            render();
    }

    function hash(s): int {
        let h = 2166136261;
        for (let i = 0; i < s.length; i++) {
            h ^= s.charCodeAt(i);
            h = Math.imul(h, 16777619);
        }
        return h >>> 0;
    }

    function cssColor(c): string {
        // force a "|#rrggbb" value the runtime can mix and contrast-check
        return "#" + c.toString().slice(-6);
    }

    onMediaActiveChanged: checkPlan()
    onLyricsModelChanged: checkPlan()
    onWidthChanged: checkPlan()
    onHeightChanged: checkPlan()
    onStyleKeyChanged: checkPlan()
    onUseCoverColorsChanged: checkPlan()
    onUnifyChanged: checkPlan()
    onTypesetChanged: checkPlan()
    onSeedModeChanged: checkPlan()
    onFixedSeedChanged: checkPlan()
    onTitleChanged: checkPlan()
    onArtistChanged: checkPlan()

    // -------------------------------------------------------------- diagnostics
    // Opt-in overlay: quality level, live Shape/Text counts and record time.
    Text {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        visible: settings.qualityMode === "debug"
        color: "#9fe8c0"
        font.pixelSize: 12
        font.family: "Consolas"
        text: "JIZURA  q" + root.qualityLevel + "  " + root.frameEma.toFixed(1) + "ms  "
              + root.liveShapes + " 图形  " + root.liveTexts + " 文本  "
              + root.pictureStepsNow().toFixed(0) + " 步/秒"
              + (root.planFailed ? "  编排失败" : "")
    }

    // The panel is loaded through a Loader rather than instantiated inline: it
    // is the only part of the plugin that imports `QueMusic 1.0`, and if that
    // import ever fails, an inline instantiation would take the whole PV down
    // with it instead of just leaving the settings block empty.
    Component {
        id: styleOptionsComponent
        Item {
            id: panelHost
            width: parent ? parent.width : 0
            implicitHeight: panelLoader.item ? panelLoader.item.implicitHeight : 0

            Loader {
                id: panelLoader
                width: panelHost.width
                height: panelHost.implicitHeight
                asynchronous: false
                source: Qt.resolvedUrl("qml/JzOptions.qml")
                onLoaded: {
                    item.styleKeys = root.styleKeys;
                }
                onStatusChanged: {
                    if (status === Loader.Error)
                        console.warn("JIZURA: settings panel failed to load");
                }
            }
        }
    }
    property Component styleOptions: styleOptionsComponent

    // Style catalogue straight from the registries inside the bundle; the
    // engine's own names are Japanese, so js/labels.js supplies the Chinese.
    readonly property var styleKeys: jz.styles().map(function (s) { return s.key })
    readonly property var styleLabels: root.styleKeys.map(function (k) { return Labels.styleName(k) })
    readonly property var styleDescs: root.styleKeys.map(function (k) { return Labels.styleDesc(k) })
}
