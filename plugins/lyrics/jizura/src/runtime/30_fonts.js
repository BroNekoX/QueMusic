/* ============================================================================
   JIZURA for QueMusic — fragment 3/4a: fonts, metrics, glyphs
   ----------------------------------------------------------------------------
   JIZURA's own src/02_fonts.js is a *browser* font layer: it injects Google
   Fonts stylesheets, loads FontFace objects, and measures advances with
   `canvas.measureText`.  Two things make it unusable inside QML:
     * it uses `async` / `await`, which the Qt 6 QML JS engine cannot even
       parse (the whole file would fail to load), and
     * `J.glyphs` rasterises glyphs with getImageData to split them into pieces,
       which the Shape backend cannot consume.

   This fragment re-implements the same *API surface* on top of Qt's own font
   engine, so every other JIZURA file keeps working untouched:
     J.FONTS          the catalogue (same keys, same kinds)
     J.fontCSS        key + px -> CSS-ish font string (parsed by the recorder)
     J.metrics.adv    key + char -> advance in em      (QML FontMetrics)
     J.metrics.clear  drop caches (called by J.setLang)
     J.glyphs         piece decomposition, degraded to "one piece per glyph"
   ========================================================================== */
(function () {
'use strict';
var J = JZB.J;

/* ---------------------------------------------------------------- catalogue
   `kind` drives J.varFontCSS (mincho -> serif) and nothing else.
   `fb` is only read by J.varFontCSS as a last-resort family.               */
function F(label, family, weight, kind, fb) { return { label: label, family: family, weight: weight, kind: kind, fb: fb }; }
var SANS_FB = 'sans-serif', SERIF_FB = 'serif', MONO_FB = 'monospace';

J.FONTS = {
  gothic_black: F('Gothic Black', 'sans', 900, 'gothic', SANS_FB),
  gothic_bold: F('Gothic Bold', 'sans', 700, 'gothic', SANS_FB),
  gothic_med: F('Gothic Medium', 'sans', 500, 'gothic', SANS_FB),
  gothic_light: F('Gothic Light', 'sans', 300, 'gothic', SANS_FB),
  dela: F('Display Black', 'display', 400, 'display', SANS_FB),
  zenkaku: F('Zenkaku Black', 'sans', 900, 'gothic', SANS_FB),
  mincho_black: F('Mincho Black', 'serif', 900, 'mincho', SERIF_FB),
  mincho_bold: F('Mincho Bold', 'serif', 700, 'mincho', SERIF_FB),
  mincho: F('Mincho Medium', 'serif', 500, 'mincho', SERIF_FB),
  mincho_light: F('Mincho Light', 'serif', 300, 'mincho', SERIF_FB),
  tokumin: F('Tokumin', 'serif', 800, 'mincho', SERIF_FB),
  round: F('Rounded', 'round', 800, 'round', SANS_FB),
  pop: F('Pop', 'display', 400, 'display', SANS_FB),
  dot: F('Dot', 'pixel', 400, 'pixel', MONO_FB),
  brush: F('Brush', 'brush', 400, 'brush', SERIF_FB),
  mono: F('Mono', 'mono', 500, 'mono', MONO_FB),
  reggae: F('Reggae', 'display', 400, 'display', SANS_FB),
  rampart: F('Rampart', 'display', 400, 'display', SANS_FB),
  potta: F('Potta', 'brush', 400, 'brush', SANS_FB),
  kiwi: F('Kiwi', 'round', 500, 'round', SANS_FB),
  klee: F('Klee', 'hand', 600, 'hand', SERIF_FB),
  shippori: F('Shippori', 'serif', 800, 'mincho', SERIF_FB),
  sansui: F('Sans UI', 'sans', 500, 'gothic', SANS_FB)
};

/* role each catalogue key is drawn with (resolved against the installed
   families below).  Mirrors the original family/weight pairs.              */
var KEY_ROLE = {
  gothic_black: ['sans', 900], gothic_bold: ['sans', 700], gothic_med: ['sans', 500], gothic_light: ['sans', 300],
  dela: ['display', 400], zenkaku: ['sans', 900], sansui: ['sans', 500],
  mincho_black: ['serif', 900], mincho_bold: ['serif', 700], mincho: ['serif', 500], mincho_light: ['serif', 300],
  tokumin: ['serif', 800], shippori: ['serif', 800],
  round: ['round', 800], pop: ['display', 400], kiwi: ['round', 500],
  dot: ['pixel', 400], brush: ['brush', 400], potta: ['brush', 400], klee: ['hand', 600],
  mono: ['mono', 500], reggae: ['display', 400], rampart: ['display', 400]
};

/* ------------------------------------------------------- installed families
   Candidate stacks, most-wanted first.  Every entry is a family name that
   either ships with Windows / macOS / common Linux desktops or can be dropped
   into <plugin>/fonts/ by the user; the QML side injects the list of families
   that actually exist, and resolve() picks the first match.                */
var STACKS = {
  ja: {
    sans: ['Noto Sans JP', 'Noto Sans CJK JP', 'Yu Gothic UI', 'Yu Gothic', 'Meiryo UI', 'Meiryo', 'MS UI Gothic', 'Microsoft YaHei UI', 'Microsoft YaHei', 'Segoe UI', 'Arial'],
    serif: ['Noto Serif JP', 'Noto Serif CJK JP', 'Yu Mincho', 'YuMincho', 'MS PMincho', 'MS Mincho', 'SimSun', 'Times New Roman'],
    round: ['M PLUS Rounded 1c', 'Yu Gothic UI', 'Meiryo UI', 'Microsoft YaHei UI', 'Segoe UI'],
    display: ['Dela Gothic One', 'Yu Gothic UI', 'Meiryo UI', 'Microsoft YaHei UI', 'Segoe UI'],
    brush: ['Yuji Syuku', 'Yu Mincho', 'SimSun', 'MS Mincho'],
    hand: ['Klee One', 'Yu Mincho', 'SimSun'],
    pixel: ['DotGothic16', 'MS Gothic', 'SimSun', 'Consolas'],
    mono: ['IBM Plex Mono', 'Consolas', 'Cascadia Mono', 'MS Gothic', 'Courier New']
  },
  'zh-Hans': {
    sans: ['Microsoft YaHei UI', 'Microsoft YaHei', 'Noto Sans SC', 'Noto Sans CJK SC', 'SimHei', 'Segoe UI', 'Arial'],
    serif: ['SimSun', 'NSimSun', 'Noto Serif SC', 'Noto Serif CJK SC', 'FangSong', 'KaiTi', 'Times New Roman'],
    round: ['Microsoft YaHei UI', 'Microsoft YaHei', 'Noto Sans SC', 'Segoe UI'],
    display: ['Microsoft YaHei UI', 'Microsoft YaHei', 'Noto Sans SC', 'SimHei'],
    brush: ['KaiTi', 'STKaiti', 'DFKai-SB', 'SimSun', 'Noto Serif SC'],
    hand: ['KaiTi', 'STKaiti', 'SimSun'],
    pixel: ['SimSun', 'NSimSun', 'MS Gothic', 'Consolas'],
    mono: ['Consolas', 'Cascadia Mono', 'Microsoft YaHei UI', 'Courier New']
  },
  'zh-Hant': {
    sans: ['Microsoft JhengHei UI', 'Microsoft JhengHei', 'Noto Sans TC', 'Noto Sans CJK TC', 'PingFang TC', 'Segoe UI'],
    serif: ['PMingLiU', 'MingLiU', 'Noto Serif TC', 'Noto Serif CJK TC', 'DFKai-SB', 'Times New Roman'],
    round: ['Microsoft JhengHei UI', 'Microsoft JhengHei', 'Noto Sans TC', 'Segoe UI'],
    display: ['Microsoft JhengHei UI', 'Microsoft JhengHei', 'Noto Sans TC'],
    brush: ['DFKai-SB', 'MingLiU', 'PMingLiU', 'Noto Serif TC'],
    hand: ['DFKai-SB', 'PMingLiU'],
    pixel: ['MingLiU', 'PMingLiU', 'Consolas'],
    mono: ['Consolas', 'Cascadia Mono', 'Microsoft JhengHei UI', 'Courier New']
  },
  ko: {
    sans: ['Malgun Gothic', 'Noto Sans KR', 'Noto Sans CJK KR', 'Apple SD Gothic Neo', 'Segoe UI'],
    serif: ['Batang', 'Noto Serif KR', 'Noto Serif CJK KR', 'Gulim', 'Times New Roman'],
    round: ['Malgun Gothic', 'Noto Sans KR', 'Segoe UI'],
    display: ['Malgun Gothic', 'Noto Sans KR'],
    brush: ['Batang', 'Gungsuh', 'Noto Serif KR'],
    hand: ['Gungsuh', 'Batang'],
    pixel: ['Gulim', 'Dotum', 'Consolas'],
    mono: ['Consolas', 'Cascadia Mono', 'GulimChe', 'Courier New']
  },
  en: {
    sans: ['Segoe UI', 'Arial', 'Helvetica Neue', 'Noto Sans', 'DejaVu Sans'],
    serif: ['Times New Roman', 'Georgia', 'Noto Serif', 'DejaVu Serif'],
    round: ['Segoe UI', 'Arial Rounded MT Bold', 'Arial'],
    display: ['Impact', 'Arial Black', 'Segoe UI'],
    brush: ['Segoe Script', 'Georgia', 'Times New Roman'],
    hand: ['Segoe Script', 'Georgia'],
    pixel: ['Consolas', 'Courier New'],
    mono: ['Consolas', 'Cascadia Mono', 'Courier New']
  }
};

var AVAILABLE = null;             // injected by QML: Qt.fontFamilies()
var STACK = STACKS;               // overridable from the plugin's fonts.json
var resolveCache = new Map();

function defaultStack() { return STACKS.zh_Hans || STACKS['zh-Hans']; }

var fonts = {
  configure: function (opt) {
    if (opt && opt.available && opt.available.length) AVAILABLE = opt.available;
    if (opt && opt.stacks) STACK = opt.stacks;
    resolveCache.clear();
  },
  available: function () { return AVAILABLE; },
  stacks: function () { return STACK; },
  /* first installed family of a candidate list; '' lets Qt pick the default */
  resolve: function (fams) {
    var key = fams.join('|');
    var v = resolveCache.get(key);
    if (v !== undefined) return v;
    v = '';
    if (AVAILABLE && AVAILABLE.length) {
      var set = null;
      if (!fonts.__set || fonts.__n !== AVAILABLE.length) {
        set = {}; for (var i = 0; i < AVAILABLE.length; i++) set[AVAILABLE[i]] = 1;
        fonts.__set = set; fonts.__n = AVAILABLE.length;
      } else set = fonts.__set;
      for (var k = 0; k < fams.length; k++) { if (fams[k] && set[fams[k]]) { v = fams[k]; break; } }
    } else {
      v = fams[0] || '';
    }
    resolveCache.set(key, v);
    return v;
  }
};

/* JIZURA hands `ctx.font` around as a CSS string; the recorder parses it back.
   Producing it here lets the plugin swap in families that actually exist. */
var fontCSS = function (key, px) {
  var r = KEY_ROLE[key] || ['sans', 400];
  var st = STACK[J.lang] || STACK['zh-Hans'] || defaultStack();
  var list = st[r[0]] || st.sans || [];
  var out = r[1] + ' ' + px.toFixed(2) + 'px ';
  for (var i = 0; i < list.length; i++) out += (i ? ',' : '') + '"' + list[i] + '"';
  return out;
};
J.fontCSS = fontCSS;
/* JIZURA's own varFontCSS (src/03_text.js) interpolates the weight; both
   weights must resolve to the same family, which the stacks guarantee. */
var cssCache = new Map();
JZB.fonts = fonts;
JZB.fonts.parse = function (css) {
  var v = cssCache.get(css);
  if (v) return v;
  var weight = 400, px = 16, italic = false, fams = [];
  var s = String(css || '');
  var wm = /^\s*(normal|bold|bolder|lighter|\d{3})\s+/.exec(s);
  if (wm) {
    var w = wm[1];
    weight = w === 'bold' ? 700 : w === 'normal' ? 400 : w === 'bolder' ? 700 : w === 'lighter' ? 300 : +w;
    s = s.slice(wm[0].length);
  }
  if (/^\s*italic\s+/.test(s)) { italic = true; s = s.replace(/^\s*italic\s+/, ''); }
  var pm = /([\d.]+)px/.exec(s);
  if (pm) { px = parseFloat(pm[1]) || 16; s = s.slice(pm.index + pm[0].length); }
  fams = s.split(',').map(function (x) { return x.trim().replace(/^["']|["']$/g, ''); }).filter(function (x) { return x && x !== 'sans-serif' && x !== 'serif' && x !== 'monospace'; });
  v = { weight: weight, px: px, italic: italic, fams: fams, fam: fonts.resolve(fams) };
  if (cssCache.size > 512) cssCache.clear();
  cssCache.set(css, v);
  return v;
};

/* ------------------------------------------------------------- metrics
   Advances are measured by Qt (see qml/JzMetrics.qml) and cached per
   (family, weight, char) in em units, exactly like the original
   J.metrics.adv cached canvas measureText results.                        */
function advFallback(ch) {
  var c = ch.codePointAt(0);
  if (c === 0x20 || c === 0x3000) return 1.0;
  if (c >= 0x1100 && c <= 0x115f) return 1.0;
  if (c >= 0x2e80 && c <= 0xa4cf) return 1.0;
  if (c >= 0xac00 && c <= 0xd7a3) return 1.0;
  if (c >= 0xf900 && c <= 0xfaff) return 1.0;
  if (c >= 0xfe30 && c <= 0xfe6f) return 1.0;
  if (c >= 0xff00 && c <= 0xff60) return 1.0;
  if (c >= 0xffe0 && c <= 0xffe6) return 1.0;
  if (c >= 0x30 && c <= 0x39) return 0.56;
  if (c >= 0x41 && c <= 0x5a) return 0.67;
  if (c >= 0x61 && c <= 0x7a) return 0.53;
  if (c < 0x80) return 0.33;
  return 0.6;
}

var advCache = new Map();
var boxCache = new Map();
var fontByKey = {};                   // font registry key -> parsed face (fontCSS is not cheap)
var provider = null;                  // set by JzMetrics.qml: { advance(), box() }

J.metrics = {
  m: advCache,
  clear: function () {
    advCache.clear(); boxCache.clear(); cssCache.clear(); resolveCache.clear();
    // Rebinding, NOT `for (var k in fontByKey) delete fontByKey[k]`: deleting the
    // properties of the very object being enumerated corrupts V4's
    // OwnPropertyKeyIterator, and the process then dies at an arbitrary later
    // property lookup (observed as an access violation inside
    // QV4::Value::sameValueZero, reached from QV4::OwnPropertyKeyIterator).
    // Rebinding is also cheaper - one allocation instead of N deletes.
    fontByKey = {};
    J.metrics.m = advCache;
  },
  setProvider: function (p) { provider = p; J.metrics.clear(); },
  /* advance in em, the unit every JIZURA layout works in */
  adv: function (fontKey, ch) {
    var F = fontByKey[fontKey];
    if (!F) F = fontByKey[fontKey] = JZB.fonts.parse(fontCSS(fontKey, 100));
    return JZB.metrics.advanceEm(F.fam, F.weight, F.italic, ch);
  }
};

JZB.metrics = {
  /** advance of one char in em */
  advanceEm: function (fam, weight, italic, ch) {
    var k = fam + '|' + weight + '|' + (italic ? 1 : 0) + '|' + ch;
    var v = advCache.get(k);
    if (v === undefined) {
      v = provider ? provider.advance(fam, weight, italic, ch) : advFallback(ch);
      if (!(v > 0)) v = advFallback(ch);
      advCache.set(k, v);
    }
    return v;
  },
  /** advance of one char in px */
  advance: function (fam, weight, italic, px, ch) {
    return JZB.metrics.advanceEm(fam, weight, italic, ch) * px;
  },
  /** ascent/descent of the em box in px, so text can be centred on a point */
  box: function (fam, weight, italic, px) {
    var k = fam + '|' + weight + '|' + (italic ? 1 : 0);
    var v = boxCache.get(k);
    if (!v) {
      v = provider ? provider.box(fam, weight, italic) : { ascent: 0.88, descent: 0.12 };
      if (!(v.ascent > 0)) v = { ascent: 0.88, descent: 0.12 };
      boxCache.set(k, v);
    }
    return { ascent: v.ascent * px, descent: v.descent * px };
  },
  /** { w, ascent, descent } in px for a whole string (canvas measureText shape) */
  glyph: function (fam, weight, italic, px, text) {
    var w = 0, s = String(text || '');
    for (var i = 0; i < s.length;) {
      var cp = s.codePointAt(i), ch = String.fromCodePoint(cp);
      i += ch.length;
      w += JZB.metrics.advanceEm(fam, weight, italic, ch) * px;
    }
    var b = JZB.metrics.box(fam, weight, italic, px);
    return { w: w, ascent: b.ascent, descent: b.descent, width: w };
  }
};

/* --------------------------------------------------------------- glyphs
   The real J.glyphs rasterises each glyph and floods it into connected
   components so pieces can fly apart.  Shapes cannot rasterise, so this
   returns "one piece" — J.drawItem then draws the whole glyph, which is
   exactly the fallback the original already has for missing glyphs.
   (JZ.caps.pieces reports this to the UI.)                              */
J.glyphs = {
  map: new Map(), tint: new Map(),
  clear: function () { this.map.clear(); this.tint.clear(); },
  get: function () { return { pieces: [] }; },
  sprite: function () { return null; }
};
J.fragments = function () { return []; };

/* ------------------------------------------------- browser-only leftovers
   Kept as no-ops so nothing that merely *mentions* them can throw.        */
J.addUserFont = function () {};
J.loadFontFile = function () { return Promise.resolve(null); };
J.restoreUserFonts = function () { return Promise.resolve([]); };
J.missingUserFonts = function () { return []; };
J.ensureFonts = function () { return Promise.resolve(); };
J.fontsOfPlan = function (plan) {
  var set = { mono: 1 };
  if (plan) {
    var f = plan.style && plan.style.fonts || {};
    for (var r in f) for (var i = 0; i < f[r].length; i++) set[f[r][i]] = 1;
  }
  return Object.keys(set).filter(function (k) { return !!J.FONTS[k]; });
};
J.SAFE_FONT_KEY = /^user_[A-Za-z0-9_-]{1,80}$/;
J.safeFamily = function (s) { return String(s || '').replace(/[^\w\- ]/g, '_').slice(0, 80); };
})();
