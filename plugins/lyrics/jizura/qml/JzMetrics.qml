// SPDX-License-Identifier: MIT
//
// JIZURA × QueMusic — Qt font engine provider for the JIZURA runtime.
//
// The port measures glyph advances with Qt's font engine instead of
// canvas.measureText, and needs the em-box ascent/descent to centre text on a
// baseline.  Measured values are cached by the runtime, so a handful of
// FontMetrics objects is enough.

import QtQuick
import QtQml

// An Item rather than a QtObject: the Instantiator below needs a default property.
Item {
    id: metrics

    visible: false
    width: 0
    height: 0

    // families are resolved by the runtime from Qt.fontFamilies(); anything not
    // installed falls back to the platform default here.
    property var byKey: ({})
    property var pool: []
    property int used: 0

    Instantiator {
        id: instantiator
        model: 24
        delegate: FontMetrics { font.pixelSize: 100 }
        onObjectAdded: (index, object) => metrics.pool.push(object)
    }

    function reset(): void {
        byKey = ({});
        used = 0;
    }

    function slot(family, weight, italic) {
        const key = family + "|" + weight + "|" + (italic ? 1 : 0);
        let fm = byKey[key];
        if (fm)
            return fm;
        if (used >= pool.length)
            return null;
        fm = pool[used++];
        fm.font.family = family && family.length ? family : "sans-serif";
        fm.font.weight = weight;
        fm.font.italic = !!italic;
        byKey[key] = fm;
        return fm;
    }

    // em advance of one character
    function advance(family, weight, italic, ch): real {
        const fm = slot(family, weight, italic);
        if (!fm)
            return 0;
        const w = fm.advanceWidth(ch);
        return w > 0 ? w / 100 : 0;
    }

    // em ascent/descent of the text box a canvas would centre on
    function box(family, weight, italic): var {
        const fm = slot(family, weight, italic);
        if (!fm)
            return { ascent: 0.88, descent: 0.12 };
        return { ascent: fm.ascent / 100, descent: fm.descent / 100 };
    }
}
