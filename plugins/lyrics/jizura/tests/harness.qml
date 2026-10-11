// SPDX-License-Identifier: MIT
//
// Standalone harness: runs the plugin without QueMusic.
//
//   qml tests/harness.qml
//   qml tests/harness.qml -- --grab 8 --out tests/out
//
// It fakes the host injection (a lyric list, a playback clock and cover
// colours) and loads ../jizura.qml through a Loader, so the entry file under
// test is exactly the one the host loads.  --grab writes PNG frames so the
// result can be eyeballed, and any runtime error the bundle records is shown
// on screen.

import QtQuick
import QtQuick.Window

Window {
    id: win
    width: 1280
    height: 720
    visible: true
    title: "JIZURA × QueMusic — harness"
    color: "#0b0b0c"

    property real t: 0
    property int grabbed: 0
    property int grabCount: 0
    property string outDir: "."
    property var errorLines: []

    property var song: [
        { time: 600,   text: "夜明けの色を覚えてる" },
        { time: 5200,  text: "ほどけた声が遠くで鳴った" },
        { time: 9800,  text: "ねえ、まだ間に合うかな" },
        { time: 14300, text: "透明なままじゃ終われない" },
        { time: 18900, text: "君の名前を風が運ぶ" },
        { time: 23400, text: "I still remember the light" },
        { time: 27900, text: "握った手のひらの熱" },
        { time: 32400, text: "もう一度だけ歌わせて" }
    ]

    Loader {
        id: loader
        anchors.fill: parent
        // --source lets the same harness load an installed copy, so the real
        // plugin directory layout can be verified too
        source: win.sourceUrl.length ? win.sourceUrl : "../jizura.qml"
        asynchronous: true
        onLoaded: win.wire()
    }
    // ------------------------------------------------------------- panel mode
    // `--panel` instantiates the plugin's `styleOptions` component so the
    // settings block can be checked (and screenshotted) outside the app's
    // style dialog.  Needs QueMusic's QML module on the import path.
    property bool showPanel: false

    Rectangle {
        id: panelFrame
        visible: win.showPanel
        x: 16
        y: 16
        width: 560
        height: Math.min(win.height - 32, panelLoader.item ? panelLoader.item.implicitHeight + 32 : 200)
        radius: 16
        color: "#fdfdff"
        border.width: 1
        border.color: "#e9e9ee"

        Loader {
            id: panelLoader
            anchors.fill: parent
            anchors.margins: 16
            asynchronous: false
            sourceComponent: win.showPanel && loader.item ? loader.item.styleOptions : null
            onLoaded: {
                if (item)
                    console.log("HARNESS panel loaded, implicitHeight=" + item.implicitHeight);
            }
            onStatusChanged: {
                if (status === Loader.Error && win.showPanel)
                    console.log("HARNESS panel FAILED to load");
            }
        }
    }

    // ------------------------------------------------------------- burn mode
    // `--burn N` renders N frames in one tight loop instead of waiting for the
    // frame clock.  A real reproduction used to need three minutes of playback;
    // this reaches the same frame count in seconds, which is what makes A/B
    // testing a suspected Qt bug practical.
    property int burnCount: 0
    property int burnDelay: 4000

    Timer {
        interval: win.burnDelay
        repeat: false
        running: win.burnCount > 0
        onTriggered: win.burn()
    }

    function burn(): void {
        const it = loader.item;
        if (!it || !it.plan) {
            console.log("HARNESS burn: no plan");
            Qt.quit();
            return;
        }
        const end = it.plan.duration * 1000 - 800;
        const t0 = Date.now();
        let t = 0, n = 0;
        for (let i = 0; i < win.burnCount; i++) {
            t += 64;
            if (t > end)
                t = 0;
            it.position = t;
            it.render();
            n++;
        }
        const ms = Date.now() - t0;
        console.log("HARNESS burn done: " + n + " frames in " + ms + " ms  ("
                    + (n / (ms / 1000)).toFixed(0) + " fps-equivalent)  quality=q"
                    + it.qualityLevel + "  worst frame " + it.worstMs + "ms");
        Qt.quit();
    }

    property string sourceUrl: ""
    property string rendererMode: "curve"

    function wire(): void {
        const it = loader.item;
        it.mediaActive = true;
        it.playing = true;
        it.lyricsModel = win.song;
        it.translateModel = [];
        it.currentIndex = 0;
        it.title = "夜明けの色";
        it.artist = "harness";
        it.mainColor = "#f5a50c";
        it.secondColor = "#16f4d4";
        it.thirdColor = "#c2185b";
        it.lyricSize = 10;
        it.position = 0;
        it.rendererMode = win.rendererMode;
        // exercise the host's style dialog contract
        if (it.styleOptions)
            console.log("HARNESS styleOptions: ok");
        else
            console.log("HARNESS styleOptions: MISSING");
    }

    Timer {
        interval: 16
        repeat: true
        running: loader.item !== null
        onTriggered: {
            win.t += 64;                      // 4x realtime so a run is quick
            const it = loader.item;
            if (!it)
                return;
            // keep the fake playback inside the planned song, so a grab never
            // lands past the end
            const end = (it.plan ? it.plan.duration : 40) * 1000 - 800;
            if (win.t > end && end > 0)
                win.t = 0;
            it.position = win.t;
        }
    }

    // track the worst frame of the run in split form
    property real recMax: 0
    property real appMax: 0
    property real allMax: 0
    property int frameCount: 0

    Timer {
        interval: 40
        repeat: true
        running: loader.item !== null
        onTriggered: {
            const it = loader.item;
            if (!it)
                return;
            win.frameCount++;
            if (it.recordMs > win.recMax) win.recMax = it.recordMs;
            if (it.applyMs > win.appMax) win.appMax = it.applyMs;
            if (it.lastMs > win.allMax) win.allMax = it.lastMs;
        }
    }

    Timer {
        interval: 250
        repeat: true
        running: loader.item !== null
        onTriggered: {
            const e = loader.item.jz.errors();
            win.errorLines = e;
            if (e.length && !win.printedStack) {
                win.printedStack = true;
                const st = loader.item.jz.stacks();
                if (st.length)
                    console.log("HARNESS STACK\n" + st[0]);
            }
        }
    }
    property bool printedStack: false

    // -------------------------------------------------------- stats-only mode
    // `--run N` plays for N seconds without grabbing and reports the frame-time
    // distribution, which is the number that decides whether a song is smooth.
    property int runSeconds: 0
    property var samples: []
    property var qHist: ({})

    Timer {
        interval: 40
        repeat: true
        running: loader.item !== null && win.runSeconds > 0
        onTriggered: {
            const it = loader.item;
            if (!it)
                return;
            win.samples.push(it.lastMs);
            win.qHist["q" + it.qualityLevel] = (win.qHist["q" + it.qualityLevel] || 0) + 1;
        }
    }

    Timer {
        interval: 1000
        repeat: true
        running: loader.item !== null && win.runSeconds > 0
        onTriggered: {
            win.runSeconds--;
            if (win.runSeconds > 0)
                return;
            const s = win.samples.slice().sort(function (a, b) { return a - b; });
            const n = s.length;
            if (!n) {
                Qt.quit();
                return;
            }
            const sum = s.reduce(function (a, b) { return a + b; }, 0);
            console.log("HARNESS stats  frames=" + n
                        + "  avg=" + (sum / n).toFixed(2)
                        + "  p50=" + s[Math.floor(n * 0.5)]
                        + "  p90=" + s[Math.floor(n * 0.9)]
                        + "  p99=" + s[Math.floor(n * 0.99)]
                        + "  max=" + s[n - 1]
                        + "  over16ms=" + s.filter(function (v) { return v > 16; }).length
                        + "  quality=" + JSON.stringify(win.qHist));
            if (win.cycles > 0) {
                console.log("HARNESS lifecycle: " + win.cycles + " create/destroy cycles done");
            }
            Qt.quit();
        }
    }

    // ------------------------------------------------------- lifecycle stress
    // `--cycles N` unloads and reloads the plugin every 2 s, which is what the
    // host does whenever the immersive page is hidden - the state the plugin has
    // to survive repeatedly within one process.
    property int cycles: 0
    property int cycleLeft: 0
    property bool cycled: false

    Timer {
        interval: 2000
        repeat: true
        running: win.cycleLeft > 0
        onTriggered: {
            win.cycleLeft--;
            // unload
            loader.source = "";
            // reload on the next tick so the destruction is fully processed
            reloadTimer.restart();
        }
    }

    Timer {
        id: reloadTimer
        interval: 60
        repeat: false
        onTriggered: {
            loader.source = win.sourceUrl.length ? win.sourceUrl : "../jizura.qml";
            win.cycles++;
        }
    }

    Component.onCompleted: {
        const args = Qt.application.arguments;
        for (let i = 0; i < args.length; i++) {
            if (args[i] === "--grab")
                grabCount = parseInt(args[i + 1]);
            if (args[i] === "--out")
                outDir = args[i + 1];
            if (args[i] === "--quality" && loader.item)
                loader.item.qualityMode = args[i + 1];
            if (args[i] === "--long")
                longLyrics = true;
            if (args[i] === "--renderer")
                rendererMode = args[i + 1];
            if (args[i] === "--burn")
                burnCount = parseInt(args[i + 1]);
            if (args[i] === "--win") {
                // a small window makes each paint cheap, so a crash that scales
                // with the number of paints shows up much sooner
                const wh = args[i + 1].split("x");
                win.width = parseInt(wh[0]);
                win.height = parseInt(wh[1]);
            }
            if (args[i] === "--run")
                runSeconds = parseInt(args[i + 1]);
            if (args[i] === "--source")
                sourceUrl = args[i + 1];
            if (args[i] === "--panel")
                showPanel = true;
            if (args[i] === "--cycles") {
                cycleLeft = parseInt(args[i + 1]);
                // keep the lifecycle stress going even after --run expires
                runSeconds = Math.max(runSeconds, cycleLeft * 2 + 4);
            }
        }
    }

    Timer {
        interval: 2200
        repeat: true
        running: win.grabCount > 0
        onTriggered: win.grabOne()
    }

    function grabOne(): void {
        const it = loader.item;
        if (!it) {
            Qt.quit();
            return;
        }
        const name = outDir + "/harness_" + String(grabbed).padStart(2, "0") + "_t" + (t / 1000).toFixed(2) + ".png";
        win.contentItem.grabToImage(function (result) {
            result.saveToFile(name);
            console.log("HARNESS " + name
                        + "  cuts=" + (it.plan ? it.plan.cuts.length : 0)
                        + "  q" + it.qualityLevel
                        + "  rec " + it.recordMs + "ms"
                        + "  apply " + it.applyMs + "ms"
                        + "  |  worst rec " + win.recMax + " apply " + win.appMax + " total " + win.allMax
                        + "  (" + win.frameCount + " frames)"
                        + "\n            worst frame " + it.worstMs + "ms  " + it.worstInfo);
            it.resetWorst();
            win.grabbed++;
            if (win.grabbed >= win.grabCount) {
                console.log("HARNESS done");
                Qt.quit();
            }
        });
        t += 6400;
    }

    // ------------------------------------------------------------- diagnostics
    Column {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 2

        Text {
            color: "#9fe8c0"; font.pixelSize: 13; font.family: "Consolas"
            text: "t=" + (win.t / 1000).toFixed(1) + "s   "
                + (loader.item && loader.item.plan
                    ? loader.item.plan.cuts.length + " cuts, " + loader.item.plan.events.length + " events"
                    : "no plan")
                + (loader.item && loader.item.planFailed ? "   PLAN FAILED" : "")
        }
        Text {
            color: "#9fe8c0"; font.pixelSize: 13; font.family: "Consolas"
            text: loader.item
                ? ("design " + loader.item.designW + "x" + loader.item.designH
                   + "   q" + loader.item.qualityLevel
                   + "   record " + loader.item.frameEma.toFixed(2) + " ms"
                   + "   " + loader.item.liveShapes + " shapes / " + loader.item.liveTexts + " texts")
                : "loading…"
        }
        Text {
            color: "#ffc080"; font.pixelSize: 12; font.family: "Consolas"
            text: win.errorLines.length ? win.errorLines.slice(0, 3).join("\n") : ""
        }
    }
}
