/* ============================================================================
   JIZURA for QueMusic — fragment 1/4: the Canvas2D -> display-list recorder
   ----------------------------------------------------------------------------
   JIZURA draws every frame with the Canvas2D API.  A QML renderer cannot replay
   raw Canvas2D (it is an immediate-mode raster API), but JIZURA's content is
   almost entirely *vector*: rects, polylines, circles, arcs, bezier blobs and
   text.  So instead of rasterising we RECORD the Canvas2D call stream into a
   flat display list and let QML draw it with Shape/ShapePath + Text.

   Nothing in this file knows about QML: it is plain JS so it also runs in Node
   (tools/svg_preview.js) to produce reference renders.

   Two design choices carry the performance:
     * path data is baked into design space as it is built, so clipping,
       merging and layer sorting never need a matrix again;
     * consecutive glyphs that advance by exactly one font advance are merged
       into a single Text item, so a lyric line is usually ONE item instead of
       twenty.
   ========================================================================== */
var JZB = (typeof JZB !== 'undefined' && JZB) || {};
(function () {
'use strict';

var TAU = Math.PI * 2;

/* ---- 2x3 affine matrices: [a, b, c, d, e, f] (canvas order) ---- */
function mul(m, n) {
  return [m[0] * n[0] + m[2] * n[1], m[1] * n[0] + m[3] * n[1],
          m[0] * n[2] + m[2] * n[3], m[1] * n[2] + m[3] * n[3],
          m[0] * n[4] + m[2] * n[5] + m[4], m[1] * n[4] + m[3] * n[5] + m[5]];
}
function ap(m, x, y) { return [m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5]]; }
function det(m) { return m[0] * m[3] - m[1] * m[2]; }
function uni(m) { var d = Math.abs(det(m)); return d > 1e-9 ? Math.sqrt(d) : 1e-6; }

/* ---- path command codes, stored in one flat number array ---- */
var M_ = 0, L_ = 1, Q_ = 2, C_ = 3, Z_ = 4;
var ARGS = [2, 2, 4, 6, 0];

/* ---- colour: everything becomes #AARRGGBB so QML parses it directly ----
   The alpha in the key is a *continuous* value (every fade multiplies it), so
   this cache must be bounded: left alone it grows by thousands of entries per
   second and is a genuine memory leak in a long playback session. */
var COLOR_CACHE = new Map();
var COLOR_CACHE_MAX = 4096;
/* 单槽备忘。normColor 是纯函数, 而绘制循环里连续遇到的往往是同一个颜色 (同一批
   图元共用 fillStyle/strokeStyle), 所以"上一次的 (颜色, alpha)"命中率很高。
   命中时连缓存键字符串都不用拼, 也不跨语言边界。只存字符串/数字, 不持有对象,
   与 JZW 那类弱表问题无关。 */
var ncC = null, ncA = -2, ncV = null;

function normColor(c, a) {
  if (a == null) a = 1;
  if (a <= 0.002) return null;
  if (c && c.__grad) return c;
  if (c === ncC && a === ncA) return ncV;
  /* C++ 内核 (LyricPvCore::normalizeColor) 接管热路径: 这里省掉的是"键字符串拼接 +
     正则/parseInt 解析 + 十六进制格式化"。语义与下面的 JS 分支完全一致 (含
     #rgb/#rgba/#rrggbb/#rrggbbaa/rgb()/rgba()/transparent 与 alpha 累乘),
     所以可以按字符串比对做回归。传空串表示"解析为透明", 对应 JS 的 null。 */
  if (JZB.core && typeof c === 'string') {
    try {
      var r0 = JZB.core.normalizeColor(c, a);
      ncC = c; ncA = a; ncV = r0 === '' ? null : r0;
      return ncV;
    } catch (e) { JZB.core = null; }        // 参数类型不被支持 -> 永久退回 JS
  }
  var key = c + '|' + (a >= 0.999 ? 1 : a.toFixed(2));
  var v = COLOR_CACHE.get(key);
  if (v !== undefined) { ncC = c; ncA = a; ncV = v; return v; }
  var r = 0, g = 0, b = 0;
  if (typeof c === 'string') {
    var s = c.trim();
    if (s.charCodeAt(0) === 35) {
      var h = s.slice(1);
      if (h.length === 3) h = h[0] + h[0] + h[1] + h[1] + h[2] + h[2];
      if (h.length === 4) h = h[0] + h[0] + h[1] + h[1] + h[2] + h[2] + h[3] + h[3];
      if (h.length >= 6) {
        r = parseInt(h.slice(0, 2), 16) || 0; g = parseInt(h.slice(2, 4), 16) || 0; b = parseInt(h.slice(4, 6), 16) || 0;
        if (h.length >= 8) a *= (parseInt(h.slice(6, 8), 16) || 0) / 255;
      }
    } else {
      var m = s.match(/rgba?\(([^)]+)\)/i);
      if (m) {
        var p = m[1].split(',');
        r = parseFloat(p[0]) || 0; g = parseFloat(p[1]) || 0; b = parseFloat(p[2]) || 0;
        if (p.length > 3) a *= parseFloat(p[3]);
      } else if (s === 'transparent') { a = 0; }
    }
  }
  if (a <= 0.002) { COLOR_CACHE.set(key, null); ncC = c; ncA = a; ncV = null; return null; }
  v = '#' + ('0' + Math.round(Math.max(0, Math.min(1, a)) * 255).toString(16)).slice(-2) +
      ('000000' + (((r & 255) << 16) | ((g & 255) << 8) | (b & 255)).toString(16)).slice(-6);
  if (COLOR_CACHE.size >= COLOR_CACHE_MAX) COLOR_CACHE.clear();
  COLOR_CACHE.set(key, v);
  ncC = c; ncA = a; ncV = v;
  return v;
}
function mulColorAlpha(c, a) {
  if (!c || a >= 0.999 || typeof c !== 'string' || c.charCodeAt(0) !== 35 || c.length < 9) return c;
  var o = Math.round(Math.max(0, Math.min(1, parseInt(c.slice(1, 3), 16) / 255 * a)) * 255);
  return '#' + ('0' + o.toString(16)).slice(-2) + c.slice(3);
}

/* ---- offscreen surface (document.createElement('canvas')) ---- */
function Surface(w, h) {
  this.width = Math.max(1, w | 0);
  this.height = Math.max(1, h | 0);
  this.__frame = null;
  this.__ctx = null;
}
Surface.prototype.getContext = function () {
  if (!this.__ctx) { this.__frame = new Frame(this.width, this.height); this.__ctx = new Ctx(this, this.__frame); }
  return this.__ctx;
};

/* ---- a recorded frame: JZB.LAYERS layers, each with a vector and a text list ---- */
function Frame(w, h) {
  this.W = w; this.H = h;
  this.gen = 0;                       // bumped by every emitted op; guards glyph-run merging
  this.n = 0;                         // total ops, for the per-frame budget
  this.nt = 0;                        // text ops, for the text budget
  this.stamp = -1;                    // root frame this surface was last written in
  this.opTop = 0;                     // recycle cursor for this surface's own ops
  this.pool = [];
  this.layers = [];
  if (JZB.trackFrames)
    JZB.allFrames.push(this);         // diagnostics only; see JZB.trackFrames
  for (var i = 0; i < JZB.LAYERS; i++) this.layers.push({ vec: [], txt: [], nv: 0, nt: 0 });
}
Frame.prototype.reset = function (w, h) {
  this.W = w; this.H = h; this.gen = 0; this.n = 0; this.nt = 0;
  this.wipe();
};
/* Drop everything recorded so far but keep the surface size.  Offscreen
   canvases are cached and reused across frames (pre-rendered tiles, low-res
   compositing buffers, glyph strips) and every one of them starts its frame
   with clearRect(0, 0, w, h) - without this the operations would pile up
   forever and be replayed on every blit. */
Frame.prototype.wipe = function () {
  this.gen++; this.n = 0; this.nt = 0; this.opTop = 0;
  for (var i = 0; i < this.layers.length; i++) {
    var L = this.layers[i];
    L.nv = 0; L.nt = 0; L.vec.length = 0; L.txt.length = 0;
  }
};

/* ---- op pool: recording the root frame allocates almost nothing after warm-up.
   Offscreen surfaces are a different matter: JIZURA caches pre-rendered canvases
   (tiles, low-res buffers, glyph strips) for many frames, so the operations they
   hold must never be recycled into a later root frame.  Non-root contexts
   therefore allocate permanent ops; only the root frame draws from the pool. ---- */
var opPool = [], opTop = 0;
function makeOp() {
  return { k: 0, d: null, svg: '', fill: null, stroke: null, sw: 0, cap: 'butt', join: 'miter', dash: null,
    blur: 0, t: '', x: 0, y: 0, w: 0, h: 0, fam: '', weight: 400, px: 0, italic: false, col: null, m: null, clip: null,
    shift: null, mergeOk: true, grad: null, ls: 0 };
}
function newOp() {
  var o = opPool[opTop];
  if (!o) o = opPool[opTop] = makeOp();
  opTop++;
  resetOp(o);
  return o;
}
function resetOp(o) {
  o.k = 0; o.d = null; o.svg = ''; o.fill = null; o.stroke = null; o.sw = 0; o.cap = 'butt'; o.join = 'miter'; o.dash = null;
  o.blur = 0; o.t = ''; o.x = 0; o.y = 0; o.w = 0; o.h = 0; o.fam = ''; o.weight = 400; o.px = 0; o.italic = false;
  o.col = null; o.m = null; o.clip = null; o.shift = null; o.mergeOk = true; o.grad = null; o.ls = 0;
  return o;
}

/* QML Shapes have no per-op blend mode: modes that only tint fall back to
   source-over, modes that need real pixel maths (or would clobber the frame)
   drop the op. */
function gcoPolicy(g) {
  switch (g) {
    case 'source-over': case '': case null: case undefined: return 1;
    case 'multiply': case 'screen': case 'overlay': case 'soft-light': case 'hard-light': return 0.9;
    default: return 0;
  }
}
function filterBlur(f) {
  if (!f || f === 'none') return 0;
  var m = /blur\(\s*([\d.]+)px\s*\)/.exec(f);
  return m ? parseFloat(m[1]) || 0 : 0;
}

/* ============================ the recorder context ======================== */
function Ctx(surface, frame) {
  this.canvas = surface;
  this.__frame = frame;
  this.__layer = 0;
  this.__flat = false;              // ghost passes: everything into layer 0
  this.__sawText = false;           // promotes layer 1 -> 3 after the first text op
  this.__run = null;                // active glyph run
  this.__stats = { paths: 0, texts: 0, merges: 0, dropped: 0, clipped: 0 };
  this.__st = {
    m: [1, 0, 0, 1, 0, 0], fill: '#000000', stroke: '#000000', lw: 1, lj: 'miter', lc: 'butt',
    alpha: 1, gco: 'source-over', filter: '', font: '10px sans-serif', fontP: null,
    align: 'start', baseline: 'alphabetic', dash: null, shadow: null, clip: null
  };
  this.__stack = [];
  this.__path = [];
  this.__sealed = false;
  this.__cx = 0; this.__cy = 0; this.__sx = 0; this.__sy = 0;
}
Ctx.prototype.__beginFrame = function (w, h) {
  this.__frame.reset(w, h);
  this.__frame.stamp = JZB.rootStamp;
  this.__st.clip = null;
  this.__run = null;
};
/* Called before recording into a non-root surface: an offscreen canvas that a
   pack repaints is starting a new picture, so whatever it holds from an earlier
   root frame is stale.  Surfaces that are only *read* (blitted) are untouched. */
Ctx.prototype.__touch = function () {
  if (this.__isRoot) return;
  var f = this.__frame;
  if (f.stamp !== JZB.rootStamp) {
    f.wipe();
    f.stamp = JZB.rootStamp;
    this.__run = null;
  }
};
/* Operation allocation.  The root frame draws from the shared pool, which is
   reset once per root frame.  An offscreen surface recycles its OWN ops instead:
   a surface that a pack repaints every frame has just been wiped, so reusing its
   op objects is safe, and a surface that is only painted once is never written
   again.  Either way a nested frame allocates nothing per frame, which keeps the
   V4 garbage collector out of the hot path entirely. */
Ctx.prototype.__mk = function () {
  if (this.__isRoot) return newOp();
  var f = this.__frame;
  var o = f.pool[f.opTop];
  if (!o) o = f.pool[f.opTop] = makeOp();
  f.opTop++;
  resetOp(o);
  return o;
};
/* Glyph-run merging is guarded by frame.gen, not by state changes: JIZURA
   wraps every glyph in save/restore and re-sets fillStyle/globalAlpha per
   glyph, so invalidating on state changes would disable merging entirely.
   Every op that is *not* part of the run bumps the generation instead, and the
   merge test re-validates colour/font/transform/clip on every glyph. */
Ctx.prototype.__breakRun = function () { };
Ctx.prototype.__bump = function () { this.__frame.gen++; };
Ctx.prototype.__push = function (code, pts) {
  if (this.__sealed) { this.__path = this.__path.slice(); this.__sealed = false; }
  var p = this.__path;
  p.push(code);
  for (var i = 0; i < pts.length; i++) p.push(pts[i]);
};
Ctx.prototype.__pt = function (x, y) { return ap(this.__st.m, x, y); };

/* ---- 标量直推: 把"变换 + push"融合成一次调用, 不再为每个点分配数组 ----
   原来 moveTo/lineTo 每次经 __pt -> ap 分配一个 [x, y]; quadraticCurveTo 要
   2 个 __pt 再加一次 concat (第 3 个数组), bezierCurveTo 要 3 个 __pt 加两次
   concat (共 5 个)。形变效果每帧要发数千条命令, 这里就是每帧上万次短命数组分配
   —— 全落在 V4 的分配器与 GC 上, 也是效果包耗时的主要来源。
   下面几个变体的取值顺序与算式和原路径逐位一致 (m[0]*x + m[2]*y + m[4] 等),
   所以显示列表、进而 SVG 与画面都完全不变。 */
Ctx.prototype.__push2 = function (code, x, y) {
  if (this.__sealed) { this.__path = this.__path.slice(); this.__sealed = false; }
  var m = this.__st.m;
  this.__path.push(code, m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5]);
};
Ctx.prototype.__push4 = function (code, a, b, x, y) {
  if (this.__sealed) { this.__path = this.__path.slice(); this.__sealed = false; }
  var m = this.__st.m;
  this.__path.push(code,
    m[0] * a + m[2] * b + m[4], m[1] * a + m[3] * b + m[5],
    m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5]);
};
Ctx.prototype.__push6 = function (code, a, b, c, d, x, y) {
  if (this.__sealed) { this.__path = this.__path.slice(); this.__sealed = false; }
  var m = this.__st.m;
  this.__path.push(code,
    m[0] * a + m[2] * b + m[4], m[1] * a + m[3] * b + m[5],
    m[0] * c + m[2] * d + m[4], m[1] * c + m[3] * d + m[5],
    m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5]);
};
Ctx.prototype.__pushCode = function (code) {
  if (this.__sealed) { this.__path = this.__path.slice(); this.__sealed = false; }
  this.__path.push(code);
};

/* ---------- state ---------- */
Ctx.prototype.save = function () {
  var s = this.__st;
  this.__stack.push({ m: s.m.slice(), fill: s.fill, stroke: s.stroke, lw: s.lw, lj: s.lj, lc: s.lc,
    alpha: s.alpha, gco: s.gco, filter: s.filter, font: s.font, fontP: s.fontP, align: s.align,
    baseline: s.baseline, dash: s.dash, shadow: s.shadow, clip: s.clip });
  this.__breakRun();
};
Ctx.prototype.restore = function () {
  var v = this.__stack.pop();
  if (v) this.__st = v;
  this.__breakRun();
};

/* ---------- transforms ---------- */
Ctx.prototype.setTransform = function (a, b, c, d, e, f) {
  if (typeof a === 'object' && a) { f = a.f; e = a.e; d = a.d; c = a.c; b = a.b; a = a.a; }
  this.__st.m = [a, b, c, d, e || 0, f || 0];
  this.__breakRun();
};
Ctx.prototype.resetTransform = function () { this.__st.m = [1, 0, 0, 1, 0, 0]; this.__breakRun(); };
Ctx.prototype.getTransform = function () {
  var m = this.__st.m;
  return { a: m[0], b: m[1], c: m[2], d: m[3], e: m[4], f: m[5] };
};
Ctx.prototype.transform = function (a, b, c, d, e, f) { this.__st.m = mul(this.__st.m, [a, b, c, d, e, f]); this.__breakRun(); };
Ctx.prototype.translate = function (x, y) { this.__st.m = mul(this.__st.m, [1, 0, 0, 1, x, y]); this.__breakRun(); };
Ctx.prototype.scale = function (x, y) { this.__st.m = mul(this.__st.m, [x, 0, 0, y, 0, 0]); this.__breakRun(); };
Ctx.prototype.rotate = function (r) { var c = Math.cos(r), s = Math.sin(r); this.__st.m = mul(this.__st.m, [c, s, -s, c, 0, 0]); this.__breakRun(); };

/* ---------- path building (baked into design space) ---------- */
Ctx.prototype.beginPath = function () {
  this.__path = []; this.__sealed = false;
  this.__cx = this.__cy = this.__sx = this.__sy = 0;
  this.__breakRun();
};
Ctx.prototype.moveTo = function (x, y) {
  this.__push2(M_, x, y);
  this.__cx = this.__sx = x; this.__cy = this.__sy = y;
};
Ctx.prototype.lineTo = function (x, y) {
  this.__push2(L_, x, y); this.__cx = x; this.__cy = y;
};
Ctx.prototype.quadraticCurveTo = function (a, b, x, y) {
  this.__push4(Q_, a, b, x, y); this.__cx = x; this.__cy = y;
};
Ctx.prototype.bezierCurveTo = function (a, b, c, d, x, y) {
  this.__push6(C_, a, b, c, d, x, y); this.__cx = x; this.__cy = y;
};
Ctx.prototype.closePath = function () { this.__pushCode(Z_); this.__cx = this.__sx; this.__cy = this.__sy; };
Ctx.prototype.rect = function (x, y, w, h) {
  if (w === 0 || h === 0) return;
  if (this.__sealed) { this.__path = this.__path.slice(); this.__sealed = false; }
  var m = this.__st.m, p = this.__path;
  p.push(M_, m[0] * x + m[2] * y + m[4], m[1] * x + m[3] * y + m[5],
            L_, m[0] * (x + w) + m[2] * y + m[4], m[1] * (x + w) + m[3] * y + m[5],
            L_, m[0] * (x + w) + m[2] * (y + h) + m[4], m[1] * (x + w) + m[3] * (y + h) + m[5],
            L_, m[0] * x + m[2] * (y + h) + m[4], m[1] * x + m[3] * (y + h) + m[5],
            Z_);
  this.__cx = this.__sx = x; this.__cy = this.__sy = y;
  this.__breakRun();
};
Ctx.prototype.roundRect = function (x, y, w, h, r) {
  if (typeof r === 'object' && r) r = r[0];
  r = Math.min(Math.abs(r || 0), Math.abs(w) / 2, Math.abs(h) / 2);
  this.beginPath();
  this.moveTo(x + r, y);
  this.arcTo(x + w, y, x + w, y + h, r);
  this.arcTo(x + w, y + h, x, y + h, r);
  this.arcTo(x, y + h, x, y, r);
  this.arcTo(x, y, x + w, y, r);
  this.closePath();
};
/* arcs become cubic beziers: affine-exact and far fewer points than a polyline */
Ctx.prototype.arc = function (cx, cy, r, a0, a1, ccw) {
  if (!(r > 0)) return;
  var span = a1 - a0;
  if (ccw) { if (span >= 0) span -= TAU; if (span < -TAU) span = -TAU; }
  else { if (span <= 0) span += TAU; if (span > TAU) span = TAU; }
  var n = Math.max(1, Math.ceil(Math.abs(span) / (TAU / 4)));
  var step = span / n, k = 4 / 3 * Math.tan(step / 4);
  var x0 = cx + r * Math.cos(a0), y0 = cy + r * Math.sin(a0);
  if (!this.__path.length) this.moveTo(x0, y0); else this.lineTo(x0, y0);
  for (var i = 0; i < n; i++) {
    var s = a0 + i * step, e = s + step;
    var cs = Math.cos(s), sn = Math.sin(s), ce = Math.cos(e), se = Math.sin(e);
    this.bezierCurveTo(cx + r * (cs - k * sn), cy + r * (sn + k * cs),
                       cx + r * (ce + k * se), cy + r * (se - k * ce),
                       cx + r * ce, cy + r * se);
  }
};
Ctx.prototype.ellipse = function (cx, cy, rx, ry, rot, a0, a1, ccw) {
  this.save(); this.translate(cx, cy); if (rot) this.rotate(rot); this.scale(rx, ry);
  this.arc(0, 0, 1, a0, a1, ccw); this.restore();
};
/* canvas arcTo: the arc of radius r tangent to P0->P1 and P1->P2 */
Ctx.prototype.arcTo = function (x1, y1, x2, y2, r) {
  if (!(r > 0)) { this.lineTo(x1, y1); return; }
  var x0 = this.__cx, y0 = this.__cy;
  var d1x = x0 - x1, d1y = y0 - y1, d2x = x2 - x1, d2y = y2 - y1;
  var l1 = Math.hypot(d1x, d1y), l2 = Math.hypot(d2x, d2y);
  if (l1 < 1e-6 || l2 < 1e-6) { this.lineTo(x1, y1); return; }
  var a1 = Math.atan2(d1y, d1x), a2 = Math.atan2(d2y, d2x), da = a2 - a1;
  while (da > Math.PI) da -= TAU;
  while (da < -Math.PI) da += TAU;
  var tan = Math.tan(Math.abs(da) / 2);
  if (tan < 1e-9) { this.lineTo(x1, y1); return; }
  var d = r / tan;
  if (d > l1) { r = r * l1 / d; d = l1; }
  var tx = x1 + d1x / l1 * d, ty = y1 + d1y / l1 * d;
  this.lineTo(tx, ty);
  if (Math.abs(da) < 1e-6) return;
  var h = Math.sqrt(Math.max(0, r * r + d * d));
  var bisx = d1x / l1 + d2x / l2, bisy = d1y / l1 + d2y / l2, bl = Math.hypot(bisx, bisy) || 1;
  var cx = x1 + bisx / bl * h, cy = y1 + bisy / bl * h;
  this.arc(cx, cy, r, Math.atan2(ty - cy, tx - cx), Math.atan2(y2 - cy, x2 - cx), da < 0);
};
Ctx.prototype.clip = function (path) {
  this.__touch();
  var d = this.__bake(path), rs = [];
  if (d.length) {
    var saved = this.__path; this.__path = d; rs = this.__pathRects(); this.__path = saved;
  }
  this.__st.clip = !rs.length ? null : rs.length > 4 ? [bboxOf(rs)] : rs;
  this.__stats.clipped++;
  this.__bump();
};
Ctx.prototype.__pathRects = function () {
  var p = this.__path, rects = [], i = 0;
  var minx = 0, miny = 0, maxx = 0, maxy = 0, any = false;
  while (i < p.length) {
    var c = p[i], n = ARGS[c]; i++;
    for (var j = 0; j < n; j += 2) {
      var x = p[i + j], y = p[i + j + 1];
      if (!any) { minx = maxx = x; miny = maxy = y; any = true; }
      else { if (x < minx) minx = x; if (x > maxx) maxx = x; if (y < miny) miny = y; if (y > maxy) maxy = y; }
    }
    i += n;
    if (any && (c === Z_ || (i < p.length && p[i] === M_))) {
      rects.push({ x: minx, y: miny, w: maxx - minx, h: maxy - miny }); any = false;
    }
  }
  if (any) rects.push({ x: minx, y: miny, w: maxx - minx, h: maxy - miny });
  return rects;
};
function bboxOf(rs) {
  var x0 = 1e18, y0 = 1e18, x1 = -1e18, y1 = -1e18;
  for (var i = 0; i < rs.length; i++) {
    var r = rs[i];
    if (r.x < x0) x0 = r.x; if (r.y < y0) y0 = r.y;
    if (r.x + r.w > x1) x1 = r.x + r.w; if (r.y + r.h > y1) y1 = r.y + r.h;
  }
  return { x: x0, y: y0, w: x1 - x0, h: y1 - y0 };
}

/* ---------- painting ---------- */
/* Path2D is used once in the packs; paths are stored raw (user space) and the
   CTM is applied when the path is painted, which is what the canvas does. */
function Path2D() { this.__d = []; }
(function () {
  function raw(self, code, args) { self.__d.push(code); for (var i = 0; i < args.length; i++) self.__d.push(args[i]); }
  Path2D.prototype.moveTo = function (x, y) { raw(this, M_, [x, y]); };
  Path2D.prototype.lineTo = function (x, y) { raw(this, L_, [x, y]); };
  Path2D.prototype.quadraticCurveTo = function (a, b, x, y) { raw(this, Q_, [a, b, x, y]); };
  Path2D.prototype.bezierCurveTo = function (a, b, c, d, x, y) { raw(this, C_, [a, b, c, d, x, y]); };
  Path2D.prototype.closePath = function () { raw(this, Z_, []); };
  Path2D.prototype.rect = function (x, y, w, h) {
    raw(this, M_, [x, y]); raw(this, L_, [x + w, y]); raw(this, L_, [x + w, y + h]); raw(this, L_, [x, y + h]); raw(this, Z_, []);
  };
  Path2D.prototype.arc = function (cx, cy, r, a0, a1, ccw) {
    if (!(r > 0)) return;
    var span = a1 - a0;
    if (ccw) { if (span >= 0) span -= Math.PI * 2; } else { if (span <= 0) span += Math.PI * 2; }
    var n = Math.max(8, Math.ceil(Math.abs(span) / (Math.PI / 12)));
    for (var i = 0; i <= n; i++) {
      var a = a0 + span * i / n, x = cx + r * Math.cos(a), y = cy + r * Math.sin(a);
      if (i === 0 && !this.__d.length) raw(this, M_, [x, y]); else if (i === 0) raw(this, L_, [x, y]); else raw(this, L_, [x, y]);
    }
  };
  Path2D.prototype.ellipse = function (cx, cy, rx, ry, rot, a0, a1, ccw) {
    var span = a1 - a0;
    if (ccw) { if (span >= 0) span -= Math.PI * 2; } else { if (span <= 0) span += Math.PI * 2; }
    var n = Math.max(8, Math.ceil(Math.abs(span) / (Math.PI / 12))), c = Math.cos(rot || 0), s = Math.sin(rot || 0);
    for (var i = 0; i <= n; i++) {
      var a = a0 + span * i / n, px = rx * Math.cos(a), py = ry * Math.sin(a);
      raw(this, i === 0 ? M_ : L_, [cx + px * c - py * s, cy + px * s + py * c]);
    }
  };
  Path2D.prototype.roundRect = function (x, y, w, h, r) {
    if (typeof r === 'object' && r) r = r[0];
    r = Math.min(Math.abs(r || 0), Math.abs(w) / 2, Math.abs(h) / 2);
    raw(this, M_, [x + r, y]);
    raw(this, L_, [x + w - r, y]); this.arc(x + w - r, y + r, r, -Math.PI / 2, 0, false);
    raw(this, L_, [x + w, y + h - r]); this.arc(x + w - r, y + h - r, r, 0, Math.PI / 2, false);
    raw(this, L_, [x + r, y + h]); this.arc(x + r, y + h - r, r, Math.PI / 2, Math.PI, false);
    raw(this, L_, [x, y + r]); this.arc(x + r, y + r, r, Math.PI, Math.PI * 1.5, false);
    raw(this, Z_, []);
  };
  Path2D.prototype.addPath = function (other) { if (other && other.__d) for (var i = 0; i < other.__d.length; i++) this.__d.push(other.__d[i]); };
})();
Ctx.prototype.__bake = function (arg) {
  if (arg && arg.__d) return transformPath(arg.__d, this.__st.m);
  return this.__path;
};

Ctx.prototype.fillRect = function (x, y, w, h) {
  this.__path = []; this.__sealed = false;
  this.rect(x, y, w, h);
  this.fill();
};
Ctx.prototype.strokeRect = function (x, y, w, h) {
  this.__path = []; this.__sealed = false;
  this.rect(x, y, w, h);
  this.stroke();
};
/* Only a full-canvas clear is meaningful for a display list; a partial clear
   would need real pixels, so it is dropped. */
Ctx.prototype.clearRect = function (x, y, w, h) {
  var f = this.__frame;
  if (x <= 0.001 && y <= 0.001 && w >= f.W - 0.001 && h >= f.H - 0.001) { f.wipe(); return; }
  this.__stats.dropped++;
};
Ctx.prototype.fill = function (path) { this.__emitPath(this.__st.fill, null, null, path); };
Ctx.prototype.stroke = function (path) { this.__emitPath(null, this.__st.stroke, this.__st.lw, path); };
Ctx.prototype.fillText = function (t, x, y) { this.__emitText(t, x, y, true); };
Ctx.prototype.strokeText = function (t, x, y) { this.__emitText(t, x, y, false); };

/* which layer the next *vector* op lands in */
Ctx.prototype.__targetLayer = function () {
  if (this.__flat) return 0;
  var L = this.__layer | 0;
  if (L < 0) L = 0;
  if (L > JZB.LAYERS - 1) L = JZB.LAYERS - 1;
  if (L === JZB.LAYER_UNDER && this.__sawText) return JZB.LAYER_OVER;   // drawn after the lyric -> on top of it
  return L;
};
/* text always lands in its own layer, except in a ghost pass (below all) and
   in the front phase (HUD text goes with the front decorations) */
Ctx.prototype.__textLayer = function () {
  if (this.__flat) return 0;
  return (this.__layer | 0) >= JZB.LAYER_FRONT ? JZB.LAYER_FRONT : JZB.LAYER_TEXT;
};

/* A canvas gradient lives in the user space that was current when it was
   created; the path is baked into design space, so the gradient stops and
   coordinates have to be baked too. */
function bakeGradient(g, m, alpha) {
  var k = uni(m), stops = [], i;
  for (i = 0; i < g.stops.length; i++) {
    var c = normColor(g.stops[i][1], alpha);
    if (c) stops.push([g.stops[i][0], c]);
  }
  if (!stops.length) return null;
  if (g.type === 'rad') {
    var f = ap(m, g.coords[0], g.coords[1]), ctr = ap(m, g.coords[3], g.coords[4]);
    return { t: 1, c: [f[0], f[1], (g.coords[2] || 0) * k, ctr[0], ctr[1], (g.coords[5] || 0) * k],
             s: stops, solid: g.__solid };
  }
  var a = ap(m, g.coords[0], g.coords[1]), b = ap(m, g.coords[2], g.coords[3]);
  return { t: 0, c: [a[0], a[1], b[0], b[1]], s: stops, solid: g.__solid };
}

Ctx.prototype.__emitPath = function (fill, stroke, lw, pathArg) {
  this.__touch();
  var s = this.__st, pol = gcoPolicy(s.gco);
  this.__breakRun();
  var usePath = this.__bake(pathArg);
  if (pol <= 0 || !usePath.length) { this.__stats.dropped++; return; }
  var grad = null;
  if (fill && fill.__grad) {
    grad = bakeGradient(fill, s.m, s.alpha * pol);
    if (!grad) return;
    fill = grad.solid;                                   // solid fallback + coalescing key
  }
  var f = fill == null ? null : normColor(fill, s.alpha * pol);
  var stc = stroke == null ? null : normColor(stroke, s.alpha * pol);
  if (!f && !stc) return;
  this.__sealed = true;                       // freeze: __push clones before the next mutation
  var d = usePath;
  var L = this.__frame.layers[this.__targetLayer()];
  var sw = stc ? Math.max(0.05, (lw == null ? s.lw : lw) * uni(s.m)) : 0;
  var blur = filterBlur(s.filter);
  var clip = this.__clipRect();
  var last = L.vec[L.nv - 1];

  /* 1. fill() + stroke() on the very same path -> a single ShapePath */
  if (last && last.mergeOk && last.d === d) {
    if (stc && !last.stroke) {
      last.stroke = stc; last.sw = sw; last.cap = s.lc; last.join = s.lj; last.dash = s.dash;
      this.__stats.paths++;
      return;
    }
    if (f && !last.fill) { last.fill = f; last.grad = grad; this.__stats.paths++; return; }
  }

  /* 2. fold this shape into the previous one when the style is identical: JIZURA
        draws a grid as 16 separate strokes and a dot ring as 36 separate fills,
        and one ShapePath with many subpaths is far cheaper than 36. */
  if (!grad && last && last.mergeOk && !last.grad && last.clip === null && clip === null &&
      last.blur === blur && last.sw === sw && last.cap === s.lc && last.join === s.lj &&
      last.fill === f && last.stroke === stc && sameDash(last.dash, s.dash) &&
      (!!last.fill) === (!!f) && (!!last.stroke) === (!!stc) && last.d.length + d.length < 24000) {
    for (var q = 0; q < d.length; q++) last.d.push(d[q]);   // in place: no array per coalesce
    this.__stats.coalesced = (this.__stats.coalesced || 0) + 1;
    this.__stats.paths++;
    return;
  }

  var sh = s.shadow, base = null;
  if (sh && stc == null && f) base = normColor(sh.color || 'rgba(0,0,0,0.6)', s.alpha * pol * 0.5);
  if (base) {
    var so = this.__mk();
    so.k = 0; so.d = d; so.fill = base; so.svg = '';
    so.sw = 0; so.cap = s.lc; so.join = s.lj; so.dash = null; so.blur = blur;
    so.shift = [sh.dx || 0, sh.dy || 0];                // approximated shadow: one offset copy underneath
    so.mergeOk = false;
    L.vec[L.nv++] = so;
    this.__stats.paths++;
  }
  var o = this.__mk();
  o.k = 0; o.d = d; o.fill = f; o.stroke = stc; o.grad = grad; o.svg = '';
  o.sw = sw; o.cap = s.lc; o.join = s.lj; o.dash = s.dash; o.blur = blur;
  o.clip = clip;
  L.vec[L.nv++] = o;
  this.__bump();
  this.__frame.n++;
  this.__stats.paths++;
};
function sameDash(a, b) {
  if (a === b) return true;
  if (!a || !b || a.length !== b.length) return false;
  for (var i = 0; i < a.length; i++) if (a[i] !== b[i]) return false;
  return true;
}

/* ---------- text ----------
   JIZURA draws text one glyph at a time, each glyph centred on its own pen
   position (canvas textAlign).  Recording that literally would need one QML
   Text item per glyph — 20+ per lyric line, times three chromatic passes.

   Instead, consecutive glyphs that advance by exactly their own font advances
   (plus a constant tracking, which QML expresses with font.letterSpacing) are
   merged into a single Text item.  The merged item is anchored on the first
   glyph's *box left edge*, which is `anchor - advance/2` for every canvas
   alignment, so the merge works for left-, centre- and right-aligned items. */
Ctx.prototype.__emitText = function (t, x, y, isFill) {
  this.__touch();
  var s = this.__st, pol = gcoPolicy(s.gco);
  if (pol <= 0 || !t) { this.__stats.dropped++; this.__breakRun(); return; }
  if (this.__frame.nt >= JZB.textBudget) {
    this.__stats.budgeted = (this.__stats.budgeted || 0) + 1;
    this.__breakRun();
    return;
  }
  if (s.align === 'start') s.align = 'left';
  var F = s.fontP || (s.fontP = JZB.fonts.parse(s.font));
  var fam = F.fam, px = F.px;
  var m = s.m, lin = [m[0], m[1], m[2], m[3], 0, 0];
  var tadv = JZB.metrics.advance(fam, F.weight, F.italic, px, t);
  var met = JZB.metrics.box(fam, F.weight, F.italic, px);
  var asc = met.ascent, desc = met.descent, h = asc + desc;
  var col = isFill ? (s.fill && s.fill.__grad ? s.fill.__solid : normColor(s.fill, s.alpha * pol))
                   : normColor(s.stroke, s.alpha * pol);
  if (!col) { this.__breakRun(); return; }
  var blur = filterBlur(s.filter);
  var baseOff = s.baseline === 'middle' ? -h / 2
              : (s.baseline === 'top' || s.baseline === 'hanging') ? 0
              : (s.baseline === 'bottom' || s.baseline === 'ideographic') ? -h
              : -asc;                                    // alphabetic (default)
  if (!this.__flat && this.__layer === 1) this.__layer = 3;
  this.__sawText = true;
  var L = this.__frame.layers[this.__textLayer()];
  var c = ap(m, x, y);

  /* ---- try to extend the glyph run ---- */
  var clip = this.__clipRect();
  var r = this.__run;
  if (isFill && !s.shadow && blur <= 0 && !s.dash && r &&
      r.gen === this.__frame.gen && sameClip(r.clip, clip) &&
      r.col === col && r.fam === fam && r.weight === F.weight && r.px === px && r.italic === F.italic &&
      r.baseline === s.baseline && r.lin[0] === lin[0] && r.lin[1] === lin[1] &&
      r.lin[2] === lin[2] && r.lin[3] === lin[3] && r.t.length + t.length <= 300 && r.n < 400) {
    var ux = Math.hypot(lin[0], lin[1]) || 1;
    var dx = c[0] - r.cx, dy = c[1] - r.cy;
    var along = (dx * lin[0] + dy * lin[1]) / ux;
    var perp = (dy * lin[0] - dx * lin[1]) / ux;
    var ls = along - (r.aLast + tadv) / 2;                 // implied tracking, in px
    if (Math.abs(perp) < 0.1 && Math.abs(ls) <= 0.4 * px && (r.n === 1 || Math.abs(ls - r.ls) <= 0.06)) {
      if (r.n === 1) r.ls = ls;
      r.t += t; r.w += ls + tadv; r.aLast = tadv; r.n++;
      r.cx = c[0]; r.cy = c[1];
      r.op.t = r.t; r.op.w = r.w; r.op.ls = r.ls;
      this.__stats.merges++;
      return;                                             // op.x / op.y already anchored on glyph 0
    }
  }
  this.__run = null;

  var ox = s.align === 'left' ? 0 : (s.align === 'right' || s.align === 'end') ? -tadv : -tadv / 2;
  var off = ap(lin, ox, baseOff);
  var bx = c[0] + off[0], by = c[1] + off[1];
  var o = this.__mk();
  o.k = 1; o.t = t; o.col = col; o.fam = fam; o.weight = F.weight; o.px = px; o.italic = F.italic;
  o.w = tadv; o.h = h; o.m = lin; o.ls = 0;
  o.x = bx; o.y = by; o.blur = blur;
  o.clip = clip;
  var sh = s.shadow;
  if (sh && isFill) {
    var sc = normColor(sh.color || 'rgba(0,0,0,0.6)', s.alpha * pol * 0.5);
    if (sc) {
      var so = this.__mk();
      so.k = 1; so.t = t; so.col = sc; so.fam = fam; so.weight = F.weight; so.px = px; so.italic = F.italic;
      so.w = tadv; so.h = h; so.m = lin; so.clip = clip; so.blur = blur; so.ls = 0;
      so.x = bx + (sh.dx || 0); so.y = by + (sh.dy || 0);
      L.txt[L.nt++] = so; this.__stats.texts++;
    }
  }
  L.txt[L.nt++] = o;
  this.__stats.texts++;
  this.__bump();
  this.__frame.n++;
  this.__frame.nt++;
  if (isFill && !s.shadow && blur <= 0 && !s.dash) {
    var b0 = ap(m, x - tadv / 2, y + baseOff);             // glyph 0's box-left corner
    this.__run = { op: o, t: t, w: tadv, aLast: tadv, n: 1, ls: 0, col: col, fam: fam, weight: F.weight,
      px: px, italic: F.italic, lin: lin, cx: c[0], cy: c[1], bx: b0[0], by: b0[1], baseline: s.baseline,
      clip: clip, gen: this.__frame.gen };
  } else {
    this.__run = null;
  }
};
function sameClip(a, b) {
  if (a === b) return true;
  if (!a || !b) return false;
  return a[0] === b[0] && a[1] === b[1] && a[2] === b[2] && a[3] === b[3];
}
Ctx.prototype.__clipRect = function () {
  var cl = this.__st.clip;
  if (!cl || !cl.length) return null;
  var r = cl[0];
  return [r.x, r.y, r.w, r.h];
};

/* ---------- offscreen surface blit (transitions composite two frames) ----------
   A blit replays the source frame's operations with a new transform, so a
   motion-blur transition that taps six times over two frames copies the whole
   frame twelve times.  JZB.opBudget bounds that: once a frame has more ops than
   the budget, further blits are dropped, which degrades a transition to fewer
   taps instead of stalling the UI. */
Ctx.prototype.drawImage = function (src) {
  this.__touch();
  var s = this.__st, pol = gcoPolicy(s.gco);
  this.__breakRun();
  if (pol <= 0) { this.__stats.dropped++; return; }
  if (!src || !src.__frame) { this.__stats.dropped++; return; }
  var sf = src.__frame;
  if (this.__frame.n + sf.n > JZB.opBudget) { this.__stats.budgeted = (this.__stats.budgeted || 0) + 1; return; }
  var m = s.m, a = s.alpha * pol;
  var args = Array.prototype.slice.call(arguments, 1);
  var dx = 0, dy = 0, dw = sf.W, dh = sf.H, sx = 0, sy = 0, sw = sf.W, shh = sf.H;
  if (args.length === 2) { dx = args[0]; dy = args[1]; }
  else if (args.length === 4) { dx = args[0]; dy = args[1]; dw = args[2]; dh = args[3]; }
  else if (args.length === 8) { sx = args[0]; sy = args[1]; sw = args[2]; shh = args[3]; dx = args[4]; dy = args[5]; dw = args[6]; dh = args[7]; }
  else if (args.length >= 9) { dx = args[4]; dy = args[5]; dw = args[6]; dh = args[7]; }
  var kx = sw ? dw / sw : 1, ky = shh ? dh / shh : 1;
  var M2 = mul(m, [kx, 0, 0, ky, dx - sx * kx, dy - sy * ky]);
  var base = this.__targetLayer();
  for (var li = 0; li < sf.layers.length; li++) {
    var srcL = sf.layers[li], dstL = this.__frame.layers[Math.min(JZB.LAYERS - 1, base + li)];
    for (var i = 0; i < srcL.nv; i++) {
      var so = srcL.vec[i], no = this.__mk();
      no.k = 0; no.d = transformPath(so.d, M2); no.svg = '';
      no.fill = mulColorAlpha(so.fill, a); no.stroke = mulColorAlpha(so.stroke, a);
      no.sw = so.sw * uni(M2); no.cap = so.cap; no.join = so.join; no.dash = so.dash; no.blur = so.blur;
      no.clip = so.clip; no.mergeOk = false;
      no.shift = so.shift ? [so.shift[0] * kx, so.shift[1] * ky] : null;
      dstL.vec[dstL.nv++] = no;
      this.__frame.n++;
    }
    for (var j = 0; j < srcL.nt; j++) {
      if (this.__frame.nt >= JZB.textBudget) { this.__stats.budgeted = (this.__stats.budgeted || 0) + 1; break; }
      var st2 = srcL.txt[j], nt = this.__mk();
      var lm = [st2.m[0], st2.m[1], st2.m[2], st2.m[3], 0, 0];
      nt.k = 1; nt.t = st2.t; nt.col = mulColorAlpha(st2.col, a); nt.fam = st2.fam; nt.weight = st2.weight;
      nt.px = st2.px * uni(M2); nt.italic = st2.italic;
      nt.m = [M2[0] * lm[0] + M2[2] * lm[1], M2[1] * lm[0] + M2[3] * lm[1],
              M2[0] * lm[2] + M2[2] * lm[3], M2[1] * lm[2] + M2[3] * lm[3], 0, 0];
      var p2 = ap(M2, st2.x, st2.y);
      nt.x = p2[0]; nt.y = p2[1];
      nt.w = st2.w * Math.abs(kx) * uni(M2); nt.h = st2.h * Math.abs(ky) * uni(M2);
      nt.clip = st2.clip; nt.blur = st2.blur;
      dstL.txt[dstL.nt++] = nt;
      this.__frame.n++;
      this.__frame.nt++;
    }
  }
  this.__bump();
};
function transformPath(d, m) {
  var out = new Array(d.length), i = 0;
  while (i < d.length) {
    var c = d[i], n = ARGS[c]; out[i] = c; i++;
    for (var j = 0; j < n; j += 2) { var q = ap(m, d[i + j], d[i + j + 1]); out[i + j] = q[0]; out[i + j + 1] = q[1]; }
    i += n;
  }
  return out;
}

/* ---------- gradients & patterns ---------- */
function Gradient(type) { this.__grad = true; this.type = type; this.coords = []; this.stops = []; this.__solid = '#888888'; }
Gradient.prototype.addColorStop = function (o, c) {
  this.stops.push([Math.max(0, Math.min(1, o)), c]);
  var s = normColor(c, 1);
  if (s) this.__solid = s;
  return this;
};
Ctx.prototype.createLinearGradient = function (x0, y0, x1, y1) { var g = new Gradient('lin'); g.coords = [x0, y0, x1, y1]; return g; };
Ctx.prototype.createRadialGradient = function (x0, y0, r0, x1, y1, r1) { var g = new Gradient('rad'); g.coords = [x0, y0, r0, x1, y1, r1]; return g; };
Ctx.prototype.createConicGradient = function () { return new Gradient('conic'); };
Ctx.prototype.createPattern = function () { return null; };      // texture fills are unsupported by the Shape backend
Ctx.prototype.createImageData = function (w, h) { return { width: w, height: h, data: [] }; };
Ctx.prototype.getImageData = function (x, y, w, h) { return { width: w, height: h, data: [] }; };
Ctx.prototype.putImageData = function () {};
Ctx.prototype.measureText = function (t) {
  var s = this.__st, F = s.fontP || (s.fontP = JZB.fonts.parse(s.font));
  return JZB.metrics.glyph(F.fam, F.weight, F.italic, F.px, t);
};
Ctx.prototype.setLineDash = function (d) { this.__st.dash = d && d.length ? d.slice(0, 6) : null; this.__breakRun(); };
Ctx.prototype.getLineDash = function () { return this.__st.dash || []; };
Ctx.prototype.isPointInPath = function () { return false; };
Ctx.prototype.isPointInStroke = function () { return false; };

/* ---------- canvas style properties ---------- */
var PROPS = {
  fillStyle: null, strokeStyle: null, lineWidth: null, lineJoin: null, lineCap: null, globalAlpha: null,
  globalCompositeOperation: null, filter: null, font: null, textAlign: null, textBaseline: null,
  shadowColor: null, shadowBlur: null, shadowOffsetX: null, shadowOffsetY: null,
  miterLimit: null, lineDashOffset: null, imageSmoothingEnabled: null
};
Object.keys(PROPS).forEach(function (p) {
  Object.defineProperty(Ctx.prototype, p, {
    get: function () {
      var s = this.__st;
      switch (p) {
        case 'shadowColor': return s.shadow ? s.shadow.color : 'rgba(0,0,0,0)';
        case 'shadowBlur': return s.shadow ? s.shadow.blur : 0;
        case 'shadowOffsetX': return s.shadow ? s.shadow.dx : 0;
        case 'shadowOffsetY': return s.shadow ? s.shadow.dy : 0;
        case 'miterLimit': return 10;
        case 'lineDashOffset': return 0;
        case 'imageSmoothingEnabled': return true;
        case 'fillStyle': return s.fill; case 'strokeStyle': return s.stroke;
        case 'lineWidth': return s.lw; case 'lineJoin': return s.lj; case 'lineCap': return s.lc;
        case 'globalAlpha': return s.alpha; case 'globalCompositeOperation': return s.gco;
        case 'filter': return s.filter; case 'font': return s.font;
        case 'textAlign': return s.align; case 'textBaseline': return s.baseline;
      }
      return null;
    },
    set: function (v) {
      var s = this.__st;
      switch (p) {
        case 'shadowColor': case 'shadowBlur': case 'shadowOffsetX': case 'shadowOffsetY':
          if (!s.shadow) s.shadow = { color: 'rgba(0,0,0,0)', blur: 0, dx: 0, dy: 0 };
          if (p === 'shadowColor') { s.shadow.color = v; if (normColor(v, 1) == null) s.shadow = null; }
          else if (p === 'shadowBlur') s.shadow.blur = +v || 0;
          else if (p === 'shadowOffsetX') s.shadow.dx = +v || 0;
          else s.shadow.dy = +v || 0;
          this.__breakRun();
          return;
        case 'miterLimit': case 'lineDashOffset': case 'imageSmoothingEnabled': return;
        case 'font': if (v !== s.font) { s.font = v; s.fontP = null; } this.__breakRun(); return;
        case 'fillStyle': s.fill = v; this.__breakRun(); return;
        case 'strokeStyle': s.stroke = v; this.__breakRun(); return;
        case 'lineWidth': s.lw = +v || 0; this.__breakRun(); return;
        case 'globalAlpha': s.alpha = v; this.__breakRun(); return;
        case 'globalCompositeOperation': s.gco = v; this.__breakRun(); return;
        case 'filter': s.filter = v || ''; this.__breakRun(); return;
        case 'textAlign': s.align = v; this.__breakRun(); return;
        case 'textBaseline': s.baseline = v; this.__breakRun(); return;
        case 'lineJoin': s.lj = v; return;
        case 'lineCap': s.lc = v; return;
      }
    }
  });
});

/* ============================ frame finalisation ===========================
   Paths stay as number arrays while recording (cheap to transform and merge);
   they become SVG path strings once, after the frame is complete. */
function r2(v) { var x = Math.round(v * 100) / 100; return x === (x | 0) ? String(x | 0) : String(x); }
function emitSvg(d) {
  var s = '', i = 0, n = d.length;
  while (i < n) {
    var c = d[i]; i++;
    if (c === M_) { s += 'M' + r2(d[i]) + ' ' + r2(d[i + 1]); i += 2; }
    else if (c === L_) { s += 'L' + r2(d[i]) + ' ' + r2(d[i + 1]); i += 2; }
    else if (c === Q_) { s += 'Q' + r2(d[i]) + ' ' + r2(d[i + 1]) + ' ' + r2(d[i + 2]) + ' ' + r2(d[i + 3]); i += 4; }
    else if (c === C_) { s += 'C' + r2(d[i]) + ' ' + r2(d[i + 1]) + ' ' + r2(d[i + 2]) + ' ' + r2(d[i + 3]) + ' ' + r2(d[i + 4]) + ' ' + r2(d[i + 5]); i += 6; }
    else s += 'Z';
  }
  return s;
}
JZB.finalize = function (frame) {
  for (var li = 0; li < frame.layers.length; li++) {
    var L = frame.layers[li];
    for (var i = 0; i < L.nv; i++) {
      var o = L.vec[i];
      if (!o.svg) o.svg = emitSvg(o.shift ? transformPath(o.d, [1, 0, 0, 1, o.shift[0], o.shift[1]]) : o.d);
    }
  }
};

JZB.Ctx = Ctx;
JZB.Surface = Surface;
JZB.Frame = Frame;
JZB.Path2D = Path2D;
JZB.emitSvg = emitSvg;
JZB.transformPath = transformPath;
JZB.normColor = normColor;
JZB.mulLeft = mul;
JZB.resetOpPool = function () { opTop = 0; };
JZB.opCount = function () { return opTop; };
})();
