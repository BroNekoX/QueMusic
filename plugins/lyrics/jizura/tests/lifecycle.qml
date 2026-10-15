// SPDX-License-Identifier: MIT
//
// Loads and unloads the plugin the way layout/PlayerMaxCenter.qml does - an
// asynchronous Loader, the host's property injection, the 320 ms heartbeat and
// the settings-panel loader - because the plain harness never exercised the
// teardown path.
//
//   qml tests/lifecycle.qml -- --passes 10 --pattern pageDlgClose

import QtQuick
import QtQuick.Window

Window {
    id: win

    width: 1280
    height: 720
    visible: true

    property bool pageVisible: false
    property bool dialogVisible: false
    property int passes: 0
    property int maxPasses: 12
    property string pattern: "pageDlgClose"
    property int pos: 0
    property string pluginUrl: "../jizura.qml"
    property bool asyncLoad: true
    property bool neverUnload: false
    property bool doInject: true
    property int uniformMs: 0
    property string injectMode: "all"   // all | bindings | requestStyle
    property var onlyKeys: []

    // --------------------------------------------------- host's inject (verbatim)
    readonly property var lyrics: [
        { time: 800, text: "夜明けの色を覚えてる", info: null },
        { time: 5000, text: "ほどけた声が遠くで鳴った", info: null },
        { time: 9000, text: "ねえ、まだ間に合うかな", info: null },
        { time: 13000, text: "透明なままじゃ終われない", info: null },
        { time: 17000, text: "君の名前を風が運ぶ", info: null },
        { time: 21000, text: "もう一度だけ歌わせて", info: null } ]

    function applyStyleRequest(key, value): void {
        if (key === "basicCd")
            win.basicCdValue = value === true;
    }
    property bool basicCdValue: false

    function inject(it): void {
        if (!it)
            return;
        const src = {
            position: () => win.pos,
            playbackRate: () => 1.0,
            playing: () => true,
            mediaActive: () => true,
            lyricsModel: () => win.lyrics,
            translateModel: () => [],
            currentIndex: () => 0,
            title: () => "夜明けの色",
            artist: () => "harness",
            coverUrl: () => "",
            mainColor: () => "#4aa8ff",
            secondColor: () => "#ff66aa",
            thirdColor: () => "#66ffaa",
            hideHeight: () => 0,
            lyricSize: () => 10,
            basicCd: () => win.basicCdValue,
            lyricType: () => 0,
            lyricMove: () => 0,
            openTranslate: () => true
        };
        const keys = win.onlyKeys.length ? win.onlyKeys : Object.keys(src);
        if (win.injectMode === "direct") {
            // plain values, no bindings at all: the plugin still gets
            // mediaActive=true and therefore still renders, but nothing is
            // bound to the host - separates "renders" from "has bindings"
            for (const k of keys)
                if (k in it)
                    it[k] = src[k]();
            if ("requestStyle" in it)
                it.requestStyle = win.applyStyleRequest;
            return;
        }
        if (win.injectMode === "declarative" || win.injectMode === "declarativeR")
            return;   // handled by the Binding elements below
        if (win.injectMode === "explicit") {
            // same bindings, but installed one by one with a literal property
            // name - no for-in iterator alive across the Qt.binding() calls
            it.position = Qt.binding(src.position);
            it.playbackRate = Qt.binding(src.playbackRate);
            it.playing = Qt.binding(src.playing);
            it.mediaActive = Qt.binding(src.mediaActive);
            it.lyricsModel = Qt.binding(src.lyricsModel);
            it.translateModel = Qt.binding(src.translateModel);
            it.currentIndex = Qt.binding(src.currentIndex);
            it.title = Qt.binding(src.title);
            it.artist = Qt.binding(src.artist);
            it.coverUrl = Qt.binding(src.coverUrl);
            it.mainColor = Qt.binding(src.mainColor);
            it.secondColor = Qt.binding(src.secondColor);
            it.thirdColor = Qt.binding(src.thirdColor);
            it.hideHeight = Qt.binding(src.hideHeight);
            it.lyricSize = Qt.binding(src.lyricSize);
            it.basicCd = Qt.binding(src.basicCd);
            it.lyricType = Qt.binding(src.lyricType);
            it.lyricMove = Qt.binding(src.lyricMove);
            it.openTranslate = Qt.binding(src.openTranslate);
        } else if (win.injectMode !== "requestStyle")
            for (const k of keys)
                if (k in it)
                    it[k] = Qt.binding(src[k]);
        if (win.injectMode !== "bindings")
            if ("requestStyle" in it)
                it.requestStyle = win.applyStyleRequest;
    }

    // ------------------------------------------------------ host-equivalent
    Loader {
        id: lyricLoader
        anchors.fill: parent
        active: win.neverUnload ? true : win.pageVisible
        asynchronous: win.asyncLoad
        source: win.pluginUrl
        onLoaded: if (win.doInject) win.inject(lyricLoader.item)
    }

    Timer {
        interval: 320
        repeat: true
        running: true
        onTriggered: if (lyricLoader.item) lyricLoader.item.timerFunction()
    }

    // ---- declarative injection, for A/B against the Qt.binding() loop ----
    // `declarativeR` additionally passes requestStyle, `declarative` does not.
    readonly property bool declOn: win.injectMode.startsWith("declarative")
                                   && win.pageVisible && lyricLoader.item !== null
    Binding { target: lyricLoader.item; property: "position";       value: win.pos;   when: win.declOn }
    Binding { target: lyricLoader.item; property: "playbackRate";   value: 1.0;       when: win.declOn }
    Binding { target: lyricLoader.item; property: "playing";        value: true;      when: win.declOn }
    Binding { target: lyricLoader.item; property: "mediaActive";    value: true;      when: win.declOn }
    Binding { target: lyricLoader.item; property: "lyricsModel";    value: win.lyrics; when: win.declOn }
    Binding { target: lyricLoader.item; property: "translateModel"; value: [];        when: win.declOn }
    Binding { target: lyricLoader.item; property: "currentIndex";   value: 0;         when: win.declOn }
    Binding { target: lyricLoader.item; property: "title";          value: "夜明けの色"; when: win.declOn }
    Binding { target: lyricLoader.item; property: "artist";         value: "harness"; when: win.declOn }
    Binding { target: lyricLoader.item; property: "coverUrl";       value: "";        when: win.declOn }
    Binding { target: lyricLoader.item; property: "mainColor";      value: "#4aa8ff"; when: win.declOn }
    Binding { target: lyricLoader.item; property: "secondColor";    value: "#ff66aa"; when: win.declOn }
    Binding { target: lyricLoader.item; property: "thirdColor";     value: "#66ffaa"; when: win.declOn }
    Binding { target: lyricLoader.item; property: "hideHeight";     value: 0;         when: win.declOn }
    Binding { target: lyricLoader.item; property: "lyricSize";      value: 10;        when: win.declOn }
    Binding { target: lyricLoader.item; property: "basicCd";        value: false;     when: win.declOn }
    Binding { target: lyricLoader.item; property: "lyricType";      value: 0;         when: win.declOn }
    Binding { target: lyricLoader.item; property: "lyricMove";      value: 0;         when: win.declOn }
    Binding { target: lyricLoader.item; property: "openTranslate";  value: true;      when: win.declOn }

    Loader {
        id: optLoader
        width: 420
        height: 620
        active: win.dialogVisible
        sourceComponent: (lyricLoader.item && lyricLoader.item.styleOptions)
                         ? lyricLoader.item.styleOptions : null
    }

    // keep the injected position moving so the render loop actually runs
    Timer {
        interval: 32
        repeat: true
        running: lyricLoader.item !== null
        onTriggered: {
            win.pos = (win.pos + 32) % 30000;
            if (win.injectMode === "direct" && lyricLoader.item)
                lyricLoader.item.position = win.pos;
        }
    }

    // ------------------------------------------------------------ timeline
    // [delay ms, pageVisible, dialogVisible]
    readonly property var scripts: ({
        pageDlgClose: [
            [2500, true,  false],     // open the page, let the 1.2 MB library compile and render
            [1200, true,  true],      // open the settings dialog -> our styleOptions is created
            [1500, false, true],      // close the page with the panel still alive   <-- suspect
            [500,  false, false],
            [500,  true,  false]      // reopen straight away
        ],
        rapidClose: [
            [2500, true,  false],
            [150,  false, false],     // yank it away right after it finished loading
            [150,  true,  false],
            [150,  false, false]
        ],
        recycle: [
            [1500, true,  false],     // open the page, load and render
            [400,  false, false]      // close it
        ],
        cancel: [
            [60,   true,  false],     // start loading, then pull it away mid-flight
            [260,  false, false],
            [60,   true,  false],
            [260,  false, false],
            [60,   true,  false],
            [260,  false, false],
            [2500, false, false]
        ],
        cancelSlow: [
            [1100, true,  false],     // still shorter than compiling 1.2 MB
            [200,  false, false],
            [1100, true,  false],
            [200,  false, false],
            [2500, false, false]
        ],
        loadOnly: [
            [3000, true,  false]
        ],
        loadUnload: [
            [3000, true,  false],
            [4000, false, false]
        ],
        loadUnloadReload: [
            [3000, true,  false],
            [300,  false, false],
            [300,  true,  false],
            [4000, false, false]
        ],
        safeOrder: [
            [2500, true,  false],
            [1000, true,  true],
            [800,  true,  false],     // close the panel first, then the page
            [600,  false, false]
        ]
    })

    property int step: 0
    property var current: scripts[pattern]

    function startDelay(ms) {
        delay.interval = win.uniformMs > 0 ? win.uniformMs : ms;
        delay.start();
    }

    function advance(): void {
        const s = win.current[win.step];
        win.pageVisible = s[1];
        win.dialogVisible = s[2];
        win.step++;
        if (win.step < win.current.length) {
            win.startDelay(s[0]);
            return;
        }
        win.step = 0;
        win.passes++;
        console.log("LIFECYCLE pass " + win.passes + " pattern=" + win.pattern
                    + " pluginAlive=" + (lyricLoader.item !== null)
                    + " panelAlive=" + (optLoader.item !== null));
        if (win.passes >= win.maxPasses) {
            console.log("LIFECYCLE survived " + win.passes + " passes");
            Qt.quit();
            return;
        }
        win.startDelay(s[0]);
    }

    Timer {
        id: delay
        repeat: false
        onTriggered: win.advance()
    }

    Component.onCompleted: {
        const args = Qt.application.arguments;
        for (let i = 0; i < args.length; i++) {
            if (args[i] === "--passes")
                maxPasses = parseInt(args[i + 1]);
            if (args[i] === "--pattern")
                pattern = args[i + 1];
            if (args[i] === "--plugin")
                pluginUrl = args[i + 1];
            if (args[i] === "--sync")
                asyncLoad = false;
            if (args[i] === "--never-unload")
                neverUnload = true;
            if (args[i] === "--no-inject")
                doInject = false;
            if (args[i] === "--uniform")
                uniformMs = parseInt(args[i + 1]);
            if (args[i] === "--inject-mode")
                injectMode = args[i + 1];
            if (args[i] === "--only")
                onlyKeys = args[i + 1].split(",");
        }
        console.log("LIFECYCLE pattern=" + pattern + " passes=" + maxPasses
                    + " plugin=" + pluginUrl + " async=" + asyncLoad
                    + " neverUnload=" + neverUnload);
        win.startDelay(200);
    }
}
