// SPDX-License-Identifier: MIT
//
// JIZURA × QueMusic — one compositing layer of a recorded frame.
//
// Five of these make up a frame (background+back decor, vectors under the text,
// the text, vectors over the text, front decor+HUD) because Shapes cannot be
// re-ordered at runtime.  Each layer keeps three pools that are re-pointed every
// frame: shapes (one per recorded path, clipped by the wrapper Item), gradients
// (a separate small pool, since a ShapePath cannot take gradient objects that
// are in its own pathElements) and texts (one merged glyph run per Text).

import QtQuick
import QtQuick.Shapes

Item {
    id: jzLayer          // not `layer`: Item's own `layer` grouped property would shadow it

    property int designW: 1920
    property int designH: 1080

    // Shape.GeometryRenderer is Qt's own default (CPU triangulation); CurveRenderer
    // uses the newer GPU tessellator.  Switchable because it is the hottest Qt
    // Quick code this plugin drives.
    property int shapeRenderer: Shape.GeometryRenderer

    // hard caps; a frame that needs more is trimmed (the runtime's op and text
    // budgets are usually reached first)
    property int shapeCap: 128
    property int gradCap: 8
    property int textCap: 160

    property var shapeCells: []
    property var gradCells: []
    property var textCells: []

    property int shapesUsed: 0
    property int gradsUsed: 0
    property int textsUsed: 0

    // plain shapes first, then gradient fills, then text
    Item { id: shapeHost; anchors.fill: parent }
    Item { id: gradHost; anchors.fill: parent }
    Item { id: textHost; anchors.fill: parent }

    // ------------------------------------------------------------------ shapes
    Component {
        id: shapeComp
        Item {
            id: cell
            property alias path: pathSvg.path
            property alias fill: shapePath.fillColor
            property alias stroke: shapePath.strokeColor
            property alias strokeWidth: shapePath.strokeWidth
            property alias capStyle: shapePath.capStyle
            property alias joinStyle: shapePath.joinStyle
            property alias dashStyle: shapePath.strokeStyle
            property alias dashPattern: shapePath.dashPattern

            visible: false
            clip: false
            width: jzLayer.designW
            height: jzLayer.designH

            // recorded paths are in design coordinates, so the Shape's origin is
            // pushed back to the design origin inside the clipping wrapper
            Item {
                x: -cell.x
                y: -cell.y
                width: jzLayer.designW
                height: jzLayer.designH
                Shape {
                    anchors.fill: parent
                    antialiasing: true
                    preferredRendererType: jzLayer.shapeRenderer
                    ShapePath {
                        id: shapePath
                        fillColor: "transparent"
                        strokeColor: "transparent"
                        strokeWidth: 0
                        capStyle: ShapePath.FlatCap
                        joinStyle: ShapePath.MiterJoin
                        PathSvg { id: pathSvg; path: "" }
                    }
                }
            }
        }
    }

    // --------------------------------------------------------------- gradients
    Component {
        id: gradComp
        Item {
            id: gcell
            property alias path: gSvg.path
            property alias x1: lg.x1
            property alias y1: lg.y1
            property alias x2: lg.x2
            property alias y2: lg.y2
            property alias focalX: rg.focalX
            property alias focalY: rg.focalY
            property alias focalRadius: rg.focalRadius
            property alias centerX: rg.centerX
            property alias centerY: rg.centerY
            property alias centerRadius: rg.centerRadius
            property alias s0p: gs0.position
            property alias s0c: gs0.color
            property alias s1p: gs1.position
            property alias s1c: gs1.color
            property alias s2p: gs2.position
            property alias s2c: gs2.color
            property alias s3p: gs3.position
            property alias s3c: gs3.color

            function useLinear() { gPath.fillGradient = lg; }
            function useRadial() { gPath.fillGradient = rg; }

            visible: false
            clip: false
            width: jzLayer.designW
            height: jzLayer.designH

            LinearGradient {
                id: lg
                x1: 0; y1: 0; x2: 0; y2: 1
                GradientStop { id: gs0; position: 0.0; color: "transparent" }
                GradientStop { id: gs1; position: 0.34; color: "transparent" }
                GradientStop { id: gs2; position: 0.67; color: "transparent" }
                GradientStop { id: gs3; position: 1.0; color: "transparent" }
            }
            RadialGradient {
                id: rg
                focalX: 0; focalY: 0; focalRadius: 0
                centerX: 0; centerY: 0; centerRadius: 1
                GradientStop { id: rs0; position: 0.0; color: "transparent" }
                GradientStop { id: rs1; position: 0.34; color: "transparent" }
                GradientStop { id: rs2; position: 0.67; color: "transparent" }
                GradientStop { id: rs3; position: 1.0; color: "transparent" }
            }

            Item {
                x: -gcell.x
                y: -gcell.y
                width: jzLayer.designW
                height: jzLayer.designH
                Shape {
                    anchors.fill: parent
                    antialiasing: true
                    preferredRendererType: jzLayer.shapeRenderer
                    ShapePath {
                        id: gPath
                        strokeColor: "transparent"
                        strokeWidth: 0
                        PathSvg { id: gSvg; path: "" }
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------- texts
    // The recorded matrix is a product of translate/rotate/scale, so its columns
    // stay orthogonal and it decomposes exactly into Rotation + Scale about the
    // box origin (sy = det / sx).  This is deliberate: writing into a Matrix4x4
    // *value type* held inside a list property is the one JS->QML operation in
    // this plugin that pokes engine-owned memory, and Qt's V4 has been observed
    // to corrupt its heap in exactly that kind of workload.  Rotation and Scale
    // are ordinary QObjects, so every write below is a plain property write.
    Component {
        id: textComp
        Item {
            id: tcell
            property alias txt: txt
            property alias rot: rot
            property alias scl: scl

            visible: false
            clip: false
            width: jzLayer.designW
            height: jzLayer.designH

            Text {
                id: txt
                x: 0
                y: 0
                text: ""
                color: "#ffffff"
                renderType: Text.QtRendering
                wrapMode: Text.NoWrap
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
                font.kerning: false
                transform: [
                    Rotation { id: rot; origin.x: 0; origin.y: 0 },
                    Scale { id: scl; origin.x: 0; origin.y: 0 }
                ]
            }
        }
    }

    Component.onCompleted: {
        growShapes(12);
        growGrads(4);
        growTexts(12);
    }

    // ------------------------------------------------------------ pool growth
    function growShapes(n): void {
        while (shapeCells.length < n && shapeCells.length < shapeCap) {
            const c = shapeComp.createObject(shapeHost);
            if (!c)
                break;
            shapeCells.push(c);
        }
    }

    function growGrads(n): void {
        while (gradCells.length < n && gradCells.length < gradCap) {
            const c = gradComp.createObject(gradHost);
            if (!c)
                break;
            gradCells.push(c);
        }
    }

    function growTexts(n): void {
        while (textCells.length < n && textCells.length < textCap) {
            const c = textComp.createObject(textHost);
            if (!c)
                break;
            textCells.push(c);
        }
    }

    // ---------------------------------------------------------------- helpers
    function placeCell(cell, op): void {
        const c = op.clip;
        if (c) {
            cell.x = c[0];
            cell.y = c[1];
            cell.width = c[2];
            cell.height = c[3];
            cell.clip = true;
        } else if (cell.clip || cell.width !== designW) {
            cell.x = 0;
            cell.y = 0;
            cell.width = designW;
            cell.height = designH;
            cell.clip = false;
        }
    }

    function capOf(c): int {
        if (c === "round")
            return ShapePath.RoundCap;
        if (c === "square")
            return ShapePath.SquareCap;
        return ShapePath.FlatCap;
    }

    function joinOf(j): int {
        if (j === "round")
            return ShapePath.RoundJoin;
        if (j === "bevel")
            return ShapePath.BevelJoin;
        return ShapePath.MiterJoin;
    }

    function stops(cell, st): void {
        const n = st.length;
        cell.s0p = n > 0 ? st[0][0] : 0;
        cell.s0c = n > 0 ? st[0][1] : "transparent";
        cell.s1p = n > 1 ? st[1][0] : cell.s0p;
        cell.s1c = n > 1 ? st[1][1] : cell.s0c;
        cell.s2p = n > 2 ? st[2][0] : cell.s1p;
        cell.s2c = n > 2 ? st[2][1] : cell.s1c;
        cell.s3p = n > 3 ? st[3][0] : cell.s2p;
        cell.s3c = n > 3 ? st[3][1] : cell.s2c;
    }

    // ---------------------------------------------------------------- updaters
    function apply(L): void {
        let i, cell, op;

        // ---- paths + gradient fills ----------------------------------------
        if (L.nv > shapeCells.length + gradCells.length) {
            growShapes(L.nv);
            growGrads(4);
        }
        let n = 0, g = 0;
        for (i = 0; i < L.nv; i++) {
            op = L.vec[i];
            if (op.grad) {
                if (g < gradCells.length) {
                    cell = gradCells[g++];
                    cell.path = op.svg;
                    const c = op.grad.c;
                    if (op.grad.t === 1) {
                        cell.useRadial();
                        cell.focalX = c[0];
                        cell.focalY = c[1];
                        cell.focalRadius = Math.max(0, c[2]);
                        cell.centerX = c[3];
                        cell.centerY = c[4];
                        cell.centerRadius = Math.max(0.01, c[5]);
                    } else {
                        cell.useLinear();
                        cell.x1 = c[0];
                        cell.y1 = c[1];
                        cell.x2 = c[2];
                        cell.y2 = c[3];
                    }
                    stops(cell, op.grad.s);
                    placeCell(cell, op);
                    cell.visible = true;
                    continue;
                }
                // out of gradient slots: fall through to a flat mid colour
            }
            if (n >= shapeCells.length) {
                growShapes(n + 8);
                if (n >= shapeCells.length)
                    continue;
            }
            cell = shapeCells[n++];
            cell.path = op.svg;
            if (op.grad) {
                cell.fill = op.fill ? op.fill : "transparent";
                cell.stroke = "transparent";
                cell.strokeWidth = 0;
            } else {
                cell.fill = op.fill ? op.fill : "transparent";
                cell.stroke = op.stroke ? op.stroke : "transparent";
                cell.strokeWidth = op.sw;
                cell.capStyle = capOf(op.cap);
                cell.joinStyle = joinOf(op.join);
                if (op.dash && op.dash.length) {
                    cell.dashStyle = ShapePath.DashLine;
                    cell.dashPattern = op.dash;
                } else if (cell.dashStyle !== ShapePath.SolidLine) {
                    cell.dashStyle = ShapePath.SolidLine;
                }
            }
            placeCell(cell, op);
            cell.visible = true;
        }
        for (i = n; i < shapesUsed; i++)
            shapeCells[i].visible = false;
        for (i = g; i < gradsUsed; i++)
            gradCells[i].visible = false;
        shapesUsed = n;
        gradsUsed = g;

        // ---- texts ---------------------------------------------------------
        if (L.nt > textCells.length)
            growTexts(L.nt);
        const nt = Math.min(L.nt, textCells.length);
        for (i = 0; i < nt; i++) {
            op = L.txt[i];
            cell = textCells[i];
            placeCell(cell, op);
            const t = cell.txt;
            t.text = op.t;
            t.color = op.col ? op.col : "#ffffff";
            t.font.family = (op.fam && op.fam.length) ? op.fam : "sans-serif";
            t.font.weight = op.weight;
            // Quantised on purpose: every distinct (family, weight, pixelSize,
            // letterSpacing) makes Qt build another font engine, and JIZURA
            // animates size and tracking continuously during an entrance.
            t.font.pixelSize = op.px > 1 ? Math.round(op.px) : 1;
            t.font.italic = op.italic;
            t.font.letterSpacing = op.ls ? Math.round(op.ls * 4) / 4 : 0;
            t.x = op.x - cell.x;
            t.y = op.y - cell.y;
            t.width = Math.max(1, op.w);
            t.height = Math.max(1, op.h);
            const m = op.m;
            const sx = Math.hypot(m[0], m[1]);
            cell.scl.xScale = sx;
            cell.scl.yScale = sx > 1e-6 ? (m[0] * m[3] - m[1] * m[2]) / sx : Math.hypot(m[2], m[3]);
            cell.rot.angle = Math.atan2(m[1], m[0]) * 180 / Math.PI;
            cell.visible = true;
        }
        for (i = nt; i < textsUsed; i++)
            textCells[i].visible = false;
        textsUsed = nt;
    }

    // Hide everything but keep the pool alive; safe to call at any time because
    // it never touches a cell that has not been created.
    function clear(): void {
        hideAll();
    }

    // Drop the recording and give the pool back its base size.  Only use this
    // while the layer is alive - Qt reclaims the items itself on destruction.
    function release(): void {
        hideAll();
        for (let i = 0; i < shapeCells.length; i++)
            shapeCells[i].path = "";
        for (let i = 0; i < gradCells.length; i++)
            gradCells[i].path = "";
        for (let i = 0; i < textCells.length; i++)
            textCells[i].txt.text = "";
    }

    function hideAll(): void {
        let i;
        for (i = 0; i < shapesUsed && i < shapeCells.length; i++)
            shapeCells[i].visible = false;
        for (i = 0; i < gradsUsed && i < gradCells.length; i++)
            gradCells[i].visible = false;
        for (i = 0; i < textsUsed && i < textCells.length; i++)
            textCells[i].visible = false;
        shapesUsed = 0;
        gradsUsed = 0;
        textsUsed = 0;
    }

    // Build-time diagnostics
    readonly property int poolSize: shapeCells.length + gradCells.length + textCells.length
}
