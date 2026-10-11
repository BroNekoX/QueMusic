// SPDX-License-Identifier: MIT
//
// JIZURA × QueMusic — the render stage: the five compositing layers, plus the
// scaling that maps the planner's design box onto the item (cover or fit).

import QtQuick
import QtQuick.Shapes

Item {
    id: stage

    // design box produced by the planner (JIZURA's own 1080p-ish resolutions)
    property int designW: 1920
    property int designH: 1080

    // false -> letterbox (fit), true -> crop (cover, the default for a PV)
    property bool cover: true
    // extra zoom applied on top of the fit, so the host's lyric-size slider
    // still has an effect on a composition that is designed for the full frame
    property real contentScale: 1.0
    property int shapeRenderer: Shape.CurveRenderer

    readonly property real fitScale: (cover
        ? Math.max(width / designW, height / designH)
        : Math.min(width / designW, height / designH)) * contentScale

    // frame currently on screen, for diagnostics
    property int liveShapes: 0
    property int liveTexts: 0

    function apply(list): void {
        if (!list) {
            clear();
            return;
        }
        const L = list.layers;
        layer0.apply(L[0]);
        layer1.apply(L[1]);
        layer2.apply(L[2]);
        layer3.apply(L[3]);
        layer4.apply(L[4]);
        let s = 0, t = 0;
        for (let i = 0; i < L.length; i++) {
            s += L[i].nv;
            t += L[i].nt;
        }
        liveShapes = s;
        liveTexts = t;
    }

    function clear(): void {
        layer0.clear();
        layer1.clear();
        layer2.clear();
        layer3.clear();
        layer4.clear();
        liveShapes = 0;
        liveTexts = 0;
    }

    // Called when the host unloads the plugin: drop the recorded paths and the
    // pool items so Qt can release their scene-graph resources immediately
    // instead of waiting for the whole page to be collected.
    function release(): void {
        layer0.release();
        layer1.release();
        layer2.release();
        layer3.release();
        layer4.release();
        liveShapes = 0;
        liveTexts = 0;
    }

    Item {
        id: content
        width: stage.designW
        height: stage.designH
        anchors.centerIn: parent
        scale: stage.fitScale

        JzLayer { id: layer0; designW: stage.designW; designH: stage.designH; shapeRenderer: stage.shapeRenderer }
        JzLayer { id: layer1; designW: stage.designW; designH: stage.designH; shapeRenderer: stage.shapeRenderer }
        JzLayer { id: layer2; designW: stage.designW; designH: stage.designH; shapeRenderer: stage.shapeRenderer }
        JzLayer { id: layer3; designW: stage.designW; designH: stage.designH; shapeRenderer: stage.shapeRenderer }
        JzLayer { id: layer4; designW: stage.designW; designH: stage.designH; shapeRenderer: stage.shapeRenderer }
    }
}
