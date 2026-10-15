/* ============================================================================
   JIZURA for QueMusic — fragment 4/4: renderer bridge + public API
   ----------------------------------------------------------------------------
   Loaded last.  It
     * wires Qt's font engine into JIZURA's metrics/glyph layer,
     * reroutes the renderer's drawing phases into the recorder's layers,
     * removes the effects the Shape backend cannot express, and
     * exposes the small API the QML stage calls once per frame.

   Everything here is a *runtime* override of JIZURA globals.  No JIZURA source
   file is edited, so updating JIZURA means re-running tools/build_bundle.py.
   ========================================================================== */
(function () {
'use strict';
var J = JZB.J;
var R = J.Renderer;

JZB.VERSION = '1.0.0';

/* ------------------------------------------------------------ capabilities
   Surfaced to the plugin UI and documented in README.md, so it is always clear
   what the Shape backend can and cannot do. */
JZB.caps = {
  vector: true,
  text: true,
  gradients: true,
  textRunMerging: true,
  pathCoalescing: true,
  rectClips: true,
  polygonClips: false,          // clipFn polygons degrade to their bounding box
  glyphPieces: false,           // J.glyphs yields one piece per glyph
  patterns: false,              // ctx.createPattern -> null (solid colour fallback)
  blur: false,                  // ctx.filter is recorded but not applied
  shadows: 'offset',            // approximated with an offset copy, no blur
  blendModes: 'tint-only',      // multiply/screen/overlay -> source-over
  frameEffects: false,          // slice / block / mosaic / zoom / invert dropped
  imageData: false,             // getImageData/putImageData are no-ops
  transitions: true
};

/* --------------------------------------------------------- effects policy
   Effects that read the finished frame back (scratch) or that need real pixel
   maths cannot be drawn with Shapes, so they are removed from the pick order
   and the planner never selects them.  `chroma`, `shake` and `flash` are pure
   geometry / colour overlays and stay. */
var KEEP_BUILTIN_FX = { chroma: 1, shake: 1, flash: 1 };
JZB.pruned = [];
JZB.pruneEffects = function () {
  var i;
  if (!J.FXE || !J.FXE_ORDER) return;
  var keep = [];
  for (i = 0; i < J.FXE_ORDER.length; i++) {
    var k = J.FXE_ORDER[i], d = J.FXE[k];
    if (!d) continue;
    if ((d.builtin && !KEEP_BUILTIN_FX[k]) || d.scratch) {
      if (JZB.pruned.indexOf('fx.' + k) < 0) JZB.pruned.push('fx.' + k);
      continue;
    }
    keep.push(k);
  }
  J.FXE_ORDER.length = 0;
  for (i = 0; i < keep.length; i++) J.FXE_ORDER.push(keep[i]);
};

/* ---------------------------------------------------- layer routing patches
   JIZURA draws background graphic -> back decor -> lyric -> front decor -> HUD.
   The recorder needs to know which phase is running and JIZURA has no hook for
   it, so two renderer entry points are wrapped: their bodies are untouched,
   only the layer cursor is moved around them. */
function setLayer(ctx, l) { if (ctx && ctx.__layer !== undefined) ctx.__layer = l; }

var origMakeEnv = R.prototype.makeEnv;
R.prototype.makeEnv = function (ctx, plan, cut, sc, o) {
  var env = origMakeEnv.apply(this, arguments);
  var c = env.ctx;
  if (c && c.__layer !== undefined) {
    if (env.pass && env.pass !== 'main') { c.__flat = true; c.__layer = 0; }   // chromatic ghost pass
    else { c.__flat = false; c.__layer = JZB.LAYER_BACK; }
  }
  return env;
};

R.prototype.drawCut = function (env) {
  var ctx = env.ctx;
  var cut = env.cut, decor = cut.decor || [];
  var flat = ctx && ctx.__flat;
  if (ctx) ctx.__sawText = false;
  if (env.layer !== 'front') {
    for (var i = 0; i < decor.length; i++) {
      var D = J.DECOR[decor[i].id];
      if (!D || D.layer !== 'back') continue;
      setLayer(ctx, flat ? 0 : JZB.LAYER_BACK);
      try { D.draw(env, null, decor[i]); } catch (e) { JZB.warn('decor.' + decor[i].id, e); }
    }
  }
  if (env.layer === 'back') return null;
  var bb = null;
  setLayer(ctx, flat ? 0 : JZB.LAYER_UNDER);
  try { bb = (J.LAYOUTS[cut.layout] || J.LAYOUTS.center).render(env); }
  catch (e) { JZB.warn('layout.' + cut.layout, e); }
  for (var j = 0; j < decor.length; j++) {
    var E2 = J.DECOR[decor[j].id];
    if (!E2 || E2.layer !== 'front') continue;
    setLayer(ctx, flat ? 0 : JZB.LAYER_FRONT);
    try { E2.draw(env, bb, decor[j]); } catch (e) { JZB.warn('decor.' + decor[j].id, e); }
  }
  return bb;
};

var origHUD = J.drawHUD;
J.drawHUD = function (env, plan) {
  var ctx = env.ctx, saved = ctx && ctx.__layer;
  setLayer(ctx, ctx && ctx.__flat ? 0 : JZB.LAYER_FRONT);
  try { origHUD(env, plan); } catch (e) { JZB.warn('hud', e); }
  setLayer(ctx, saved);
};

var origMorph = R.prototype.drawMorph;
R.prototype.drawMorph = function (ctx, L, e, allowFilter) {
  var saved = ctx.__layer;
  setLayer(ctx, ctx.__flat ? 0 : JZB.LAYER_OVER);
  try { origMorph.call(this, ctx, L, e, allowFilter); } catch (err) { JZB.warn('morph', err); }
  setLayer(ctx, saved);
};

var origPost = R.prototype.post;
R.prototype.post = function (ctx, plan, t, tq, step, sc, scale, opt, allowFilter) {
  var saved = ctx.__layer;
  setLayer(ctx, JZB.LAYER_FRONT);
  try { origPost.apply(this, arguments); } catch (e) { JZB.warn('post', e); }
  setLayer(ctx, saved);
};

/* transition / effect entry points take the context first: move the cursor so
   their output lands above everything already drawn */
function wrapLayerFirst(fn) {
  return function (ctx) {
    var saved = ctx && ctx.__layer;
    setLayer(ctx, JZB.LAYER_FRONT);
    try { return fn.apply(this, arguments); } finally { setLayer(ctx, saved); }
  };
}
JZB.wrapEffectLayers = function () {
  if (JZB.__layersWrapped) return;
  JZB.__layersWrapped = true;
  var k;
  for (k in J.TRANS) if (J.TRANS[k] && J.TRANS[k].draw) J.TRANS[k].draw = wrapLayerFirst(J.TRANS[k].draw);
  for (k in J.FXE) if (J.FXE[k] && J.FXE[k].draw) J.FXE[k].draw = wrapLayerFirst(J.FXE[k].draw);
};

/* ------------------------------------------------------------ frame batching
   frame() is re-entrant: transitions render the previous cut into an offscreen
   surface (another Frame) and blit it.  Only the root call clears the op pool
   and finalises; nested calls just reset their own surface. */
var origFrame = R.prototype.frame;
R.prototype.frame = function (ctx, plan, t, opt) {
  var root = !!ctx.__isRoot;
  if (root) {
    JZB.resetOpPool();
    JZB.rootStamp++;
  }
  ctx.__beginFrame(plan.W, plan.H);
  var out = null;
  try { out = origFrame.call(this, ctx, plan, t, opt); }
  catch (e) { JZB.warn('frame', e); }
  if (root) {
    JZB.finalize(ctx.__frame);
    JZB.lastFrame = ctx.__frame;
  }
  return out;
};

/* --------------------------------------------------------------- diagnostics */
JZB.errors = [];
JZB.stacks = [];
JZB.warn = function (what, e) {
  if (JZB.errors.length < 30) {
    JZB.errors.push(what + ': ' + (e && e.message ? e.message : e));
    if (e && e.stack && JZB.stacks.length < 6) JZB.stacks.push(what + '\n' + String(e.stack));
  }
};
/* JIZURA's renderer reports content failures through console.warn (its own
   try/catch around every background graphic, layout, decoration and effect).
   Those messages carry the exception, so keep their stacks too - a pack that
   throws on a canvas feature the Shape backend does not implement is otherwise
   invisible. */
(function () {
  try {
    if (typeof console === 'undefined' || !console.warn) return;
    var origWarn = console.warn, origError = console.error;
    function capture(args) {
      for (var i = 1; i < args.length; i++) {
        var v = args[i];
        if (v && v.stack && JZB.stacks.length < 6) {
          JZB.stacks.push(String(args[0]) + ' ' + String(args[1] || '') + '\n' + String(v.stack));
          break;
        }
      }
    }
    console.warn = function () { capture(arguments); return origWarn.apply(console, arguments); };
    console.error = function () { capture(arguments); return origError.apply(console, arguments); };
  } catch (e) { /* read-only console */ }
})();

/* ============================== public API ================================ */
var ASPECTS = [['16:9', 16 / 9], ['21:9', 21 / 9], ['4:3', 4 / 3], ['1:1', 1], ['3:4', 3 / 4], ['4:5', 4 / 5], ['9:16', 9 / 16]];

function now() { return (typeof performance !== 'undefined' && performance.now) ? performance.now() : 0; }

var JZ = {
  version: JZB.VERSION,
  caps: JZB.caps,
  errors: function () { return JZB.errors.slice(); },
  stacks: function () { return JZB.stacks.slice(); },
  prunedEffects: function () { return JZB.pruned.slice(); },
  setBudget: function (n) { JZB.opBudget = n > 100 ? n : 100; return JZB.opBudget; },
  budget: function () { return JZB.opBudget; },
  setTextBudget: function (n) { JZB.textBudget = n > 40 ? n : 40; return JZB.textBudget; },

  /* 接入 C++ 计算内核 (LyricPvCore, 见 cpp/lyricpvcore)。
     传 null/不传 -> 全部留在 JS, PV 仍然可用 (外部插件安装场景)。 */
  setCore: function (core) {
    JZB.core = core || null;
    if (JZB.core && JZB.core.resetCaches) JZB.core.resetCaches();
    return !!JZB.core;
  },
  hasCore: function () { return !!JZB.core; },

  /* called once from the QML stage; host = { fontFamilies, metrics, stacks, core } */
  install: function (host) {
    if (host && host.core) JZ.setCore(host.core);
    if (host && host.fontFamilies && host.fontFamilies.length) {
      JZB.fonts.configure({ available: Array.prototype.slice.call(host.fontFamilies) });
    }
    if (host && host.stacks) JZB.fonts.configure({ stacks: host.stacks });
    if (host && host.metrics) J.metrics.setProvider(host.metrics);
    JZB.pruneEffects();
    JZB.wrapEffectLayers();
    return JZ;
  },

  /* the aspect whose JIZURA design size best matches a stage of w x h */
  aspectFor: function (w, h) {
    var want = Math.log((w || 16) / (h || 9)), best = ASPECTS[0][0], bd = 1e9;
    for (var i = 0; i < ASPECTS.length; i++) {
      var d = Math.abs(Math.log(ASPECTS[i][1]) - want);
      if (d < bd) { bd = d; best = ASPECTS[i][0]; }
    }
    return best;
  },

  /* [{time: ms, text}] -> the LRC string JIZURA's parser expects */
  lyricsToLrc: function (lines) {
    var out = [];
    for (var i = 0; i < lines.length; i++) {
      var l = lines[i];
      if (!l || l.text == null) continue;
      var tx = String(l.text);
      if (!tx) continue;
      var total = Math.max(0, +l.time || 0) / 1000;
      var m = Math.floor(total / 60), s = total - m * 60;
      out.push('[' + (m < 10 ? '0' : '') + m + ':' + (s < 10 ? '0' : '') + s.toFixed(2) + ']' + tx);
    }
    return out.join('\n');
  },

  /* build a JIZURA project from plugin settings + the song's lyrics */
  makeProject: function (opt) {
    opt = opt || {};
    var p = J.defaultProject();
    p.lyrics = opt.lyrics || '';
    p.title = opt.title || '';
    p.artist = opt.artist || '';
    p.style = opt.style || 'noir';
    p.seed = opt.seed == null ? 20260922 : opt.seed >>> 0;
    p.aspect = opt.aspect || '16:9';
    p.res = opt.res || 1080;
    p.fps = opt.fps || 60;
    p.lang = opt.lang || 'auto';
    p.unify = !!opt.unify;
    p.typeset = !!opt.typeset;
    p.extra = opt.extra !== false;
    p.wa = opt.wa !== false;
    p.typo = opt.typo !== false;
    p.kinetic = opt.kinetic !== false;
    p.horror = !!opt.horror;
    p.centerFree = !!opt.centerFree;
    p.keyBg = 'off';
    p.fx = Object.assign(p.fx, opt.fx || {});
    p.fx.hud = 'off';                      // the host draws its own chrome
    if (opt.colors) {
      p.colors = Object.assign({ enabled: false }, opt.colors);
      p.colors.enabled = !!opt.colors.enabled;
    }
    if (opt.groups) {
      for (var g in opt.groups) {
        if (!p.enabled[g]) continue;
        for (var k in p.enabled[g]) p.enabled[g][k] = !!opt.groups[g];
      }
    }
    if (opt.disabled) {                    // per-entry switch-off, applied last
      for (var g2 in opt.disabled) {
        if (!p.enabled[g2]) continue;
        for (var k2 in opt.disabled[g2]) if (opt.disabled[g2][k2]) p.enabled[g2][k2] = false;
      }
    }
    JZB.project = p;
    return p;
  },

  /* run the planner: { plan, W, H, ms, error } */
  plan: function (project, audio) {
    var t0 = now(), plan = null, err = null;
    try { plan = J.plan(project, audio || null); } catch (e) { err = e; JZB.warn('plan', e); }
    if (!plan) return { plan: null, error: err ? String(err.message || err) : 'plan failed', ms: now() - t0 };
    JZB.plan = plan;
    JZB.project = project;
    if (!JZB.renderer) JZB.renderer = new R();
    JZB.renderer.filterOK = false;         // ctx.filter is not applied by the Shape backend
    JZB.rootSurface = new JZB.Surface(plan.W, plan.H);
    JZB.rootCtx = JZB.rootSurface.getContext('2d');
    JZB.rootCtx.__isRoot = true;
    return { plan: plan, W: plan.W, H: plan.H, ms: now() - t0 };
  },

  hasPlan: function () { return !!JZB.plan; },
  /* The host destroys the plugin item whenever the immersive page is hidden.
     The bundle itself lives on (it is a shared QML library script), so the plan,
     the surfaces and every cache have to be dropped explicitly - otherwise a
     long session with many entries accumulates one plan and one set of caches
     per visit. */
  dispose: function () {
    // 内核缓存里只有几何与颜色字符串 (不持有 plan/QML 对象), 但 plan 都没了,
    // 留着也没意义, 顺手清掉
    if (JZB.core && JZB.core.resetCaches) JZB.core.resetCaches();
    // The metrics provider is the QML JzMetrics object and is about to be
    // destroyed: drop the reference *first*, or a later lookup would call
    // through a dangling QObject.
    J.metrics.setProvider(null);
    JZB.plan = null;
    JZB.lastFrame = null;
    JZB.project = null;
    JZB.rootCtx = null;
    JZB.rootSurface = null;
    JZB.renderer = null;
    JZB.errors.length = 0;
    JZB.stacks.length = 0;
    JZB.allFrames.length = 0;
    JZB.layoutCache.clear();
    JZB.opBudget = 1600;
    JZB.textBudget = 300;
    JZ.setBackgrounds(true);
    JZ.setDecorations(true);
    J.metrics.clear();
    JZB.resetOpPool();
  },
  design: function () {
    return JZB.plan ? { W: JZB.plan.W, H: JZB.plan.H, fps: JZB.plan.fps, duration: JZB.plan.duration } : null;
  },
  currentPlan: function () { return JZB.plan; },
  /* the cut playing at t (seconds), for diagnostics */
  cutAt: function (t) { return JZB.plan ? J.cutAt(JZB.plan, t) : null; },
  describeCut: function (t) {
    var c = J.cutAt(JZB.plan, t);
    if (!c) return 'gap';
    var d = [];
    for (var i = 0; i < (c.decor || []).length; i++) d.push(c.decor[i].id);
    return c.layout + (c.trans ? ' trans=' + c.trans : '') + ' bg=' + (c.bg || 'none')
         + ' enter=' + c.enter + ' exit=' + c.exit + (d.length ? ' decor=' + d.join('+') : '');
  },

  /* record one frame at t (seconds); returns the display list */
  frame: function (t, opt) {
    if (!JZB.plan || !JZB.rootCtx) return null;
    JZB.errors.length = 0;
    var o = opt || {}, ctx = JZB.rootCtx;
    ctx.__layer = JZB.LAYER_BACK;
    ctx.__flat = false;
    ctx.__sawText = false;
    JZB.renderer.frame(ctx, JZB.plan, t, {
      scale: 1, fast: !!o.fast, noPost: !!o.noPost, noGhost: !!o.noGhost,
      noTrans: !!o.noTrans, noHud: true
    });
    return ctx.__frame;
  },

  /* The biggest display lists alive right now.  An offscreen surface whose op
     count keeps climbing across frames is the one bug an immediate-mode
     recorder must not have, and it is invisible from the outside - this is how
     tools/scan_errors.js and a soak test catch it. */
  frameStats: function (top) {
    var a = JZB.allFrames.slice();
    a.sort(function (x, y) { return y.n - x.n; });
    var out = [];
    for (var i = 0; i < Math.min(top || 8, a.length); i++) {
      out.push({ n: a[i].n, nt: a[i].nt, W: a[i].W, H: a[i].H, root: !!a[i].__isRoot, stamp: a[i].stamp });
    }
    return out;
  },

  stats: function () {    var f = JZB.lastFrame, nv = 0, nt = 0, layers = [];
    if (f) {
      for (var i = 0; i < f.layers.length; i++) {
        nv += f.layers[i].nv; nt += f.layers[i].nt;
        layers.push(f.layers[i].nv + 'v/' + f.layers[i].nt + 't');
      }
    }
    return { paths: nv, texts: nt, layers: layers, ops: JZB.opCount ? JZB.opCount() : 0 };
  },

  /* entry counts, for the settings UI */
  inventory: function () {
    var g = J.GROUP_KEYS || [], out = {};
    for (var i = 0; i < g.length; i++) out[g[i]] = J.order(g[i]).length;
    out.style = J.STYLE_ORDER ? J.STYLE_ORDER.length : 0;
    return out;
  },

  /* style catalogue for the settings UI */
  styles: function () {
    var out = [];
    for (var i = 0; i < (J.STYLE_ORDER || []).length; i++) {
      var k = J.STYLE_ORDER[i], s = J.STYLES[k];
      if (s) out.push({ key: k, name: s.name || k, desc: s.desc || '' });
    }
    return out;
  },

  /* a plain beat grid, so a known BPM still gets beat-synced accents */
  beatGrid: function (bpm, offset, duration) {
    var out = [];
    if (!(bpm > 0)) return out;
    var period = 60 / bpm, t = offset || 0;
    while (t < (duration || 600)) { out.push(+t.toFixed(4)); t += period; }
    return out;
  }
};

JZB.pruneEffects();
JZB.wrapEffectLayers();

/* ------------------------------------------------------------ layout caching
   JIZURA re-lays its text out on every frame because the layout functions build
   a fresh item object each time.  Outside of an entrance (where the size
   changes continuously) the result is identical frame to frame, so memoising
   J.layoutText on the item's parameters removes the single hottest loop of the
   whole engine.  Nothing mutates the returned rows, so sharing them is safe. */
JZB.layoutCache = new Map();
(function () {
  var origLayoutText = J.layoutText;
  J.layoutText = function (it) {
    var key = it.text + '\u0000' + it.font + '\u0000' + it.size + '\u0000' + (it.track || 0)
            + '\u0000' + (it.lead || 1.3) + '\u0000' + (it.sx || 1) + '\u0000' + (it.sy || 1)
            + '\u0000' + (it.vertical ? 1 : 0) + '\u0000' + (it.align || 'c') + '\u0000' + (J.TYPESET ? 1 : 0);
    var v = JZB.layoutCache.get(key);
    if (v === undefined) {
      v = origLayoutText(it);
      if (JZB.layoutCache.size > 1500) JZB.layoutCache.clear();
      JZB.layoutCache.set(key, v);
    }
    return v;
  };
  var origMeasure = J.measure;
  J.measure = function (it) {
    var l = J.layoutText(it);
    return { w: l.W * (it.sx || 1), h: l.H * (it.sy || 1), lay: l };
  };
})();

/* ------------------------------------------------------- content degradation
   The last two quality steps drop whole content groups at runtime instead of
   re-planning: JIZURA's background graphics are designed for offline rendering
   and a few of them are the single most expensive thing in a frame.  Swapping
   draw() for a no-op keeps the arrangement (and the timing) intact. */
JZB.bgOn = true;
JZB.decorOn = true;
function overrideGroup(reg, on, flag) {
  for (var k in reg) {
    var d = reg[k];
    if (!d || typeof d.draw !== 'function') continue;
    if (!on) {
      if (!d.__origDraw) { d.__origDraw = d.draw; d.draw = noop; }
    } else if (d.__origDraw) {
      d.draw = d.__origDraw;
      d.__origDraw = null;
    }
  }
}
function noop() { }

JZ.setBackgrounds = function (on) {
  on = !!on;
  if (on === JZB.bgOn) return;
  JZB.bgOn = on;
  overrideGroup(J.BG, on);
};
JZ.setDecorations = function (on) {
  on = !!on;
  if (on === JZB.decorOn) return;
  JZB.decorOn = on;
  overrideGroup(J.DECOR, on);
};

/* ===================== C++ 计算内核 (LyricPvCore) 接管 =====================
   cpp/lyricpvcore 把"每帧都要做、与画面语义无关"的重活搬到了 C++。实测这一块占
   一帧的 ~40%: 路径 -> SVG 字符串 ~25%, 颜色归一化 ~16% (1280x720 / q0 画质,
   record+apply 每帧 ~11.4ms)。JS 的瓶颈在于循环里反复拼接字符串、数值格式化要过
   V4 的 String 转换、颜色解析要过正则 + parseInt。

   接法有三个约束:
     1. 只在 JZB.core 存在时启用。宿主没链接这个模块 (例如把 jizura 目录当外部
        插件拷到别的播放器) 时, 全部退回下面的 JS 实现, PV 照常工作;
     2. 跨语言边界要尽量少: 每帧只调一次 pathsToSvg (整帧的向量 op 一次交出去),
        而不是每条路径一次;
     3. 一旦某次调用抛异常 (参数类型不被该 Qt 版本支持等), 立刻永久退回 JS,
        并把 JZB.core 置空, 避免每帧都在 try/catch 里付代价。

   normalizeColor 的接管不在这里: 它是 recorder 里的局部函数, 见 10_recorder.js。 */

/* 暂存数组跨帧复用: 每帧新建 cmds/offs/mats/pending 四个数组要走 V4 的分配 +
   逐次扩容 (cmds 一帧约 1,200 个数), 复用后这部分基本归零。数组只在本次同步调用
   期间被读走, 下一次 finalize 之前不会有人引用它。 */
var finCmds = [], finOffs = [0], finMats = [], finOps = [];

/* 整帧的向量 op -> SVG, 一次跨界。
   每条 op 的几何是 o.d (扁平指令流), 位移是 o.shift (可选), 二者合起来就是一个
   仿射矩阵 —— 正好对上 C++ 的 pathsToSvg(commands, offsets, matrices) 签名。
   全部 op 都没有位移时连 matrices 都不用传 (C++ 侧按单位阵处理), 省一次数组转换。 */
(function () {
  var origFinalize = JZB.finalize;
  JZB.finalize = function (frame) {
    var core = JZB.core;
    if (!core) { origFinalize(frame); return; }
    try {
      var cmds = finCmds, offs = finOffs, mats = finMats, pend = finOps;
      cmds.length = 0; mats.length = 0; pend.length = 0;
      offs.length = 0; offs.push(0);
      var needMats = false;
      for (var li = 0; li < frame.layers.length; li++) {
        var L = frame.layers[li];
        for (var i = 0; i < L.nv; i++) {
          var o = L.vec[i];
          if (o.svg) continue;                  // 与原实现一致: 已经有 svg 的不重算
          var d = o.d;
          if (!d || !d.length) { o.svg = ''; continue; }
          for (var k = 0; k < d.length; k++) cmds.push(d[k]);
          offs.push(cmds.length);
          if (o.shift) {
            needMats = true;
            mats.push(1, 0, 0, 1, o.shift[0], o.shift[1]);
          } else {
            mats.push(1, 0, 0, 1, 0, 0);
          }
          pend.push(o);
        }
      }
      if (!pend.length) return;
      var out = core.pathsToSvg(cmds, offs, needMats ? mats : []);
      for (var j = 0; j < pend.length; j++) pend[j].svg = out[j] || '';
    } catch (e) {
      JZB.core = null;                          // 永久退回 JS
      origFinalize(frame);
    }
  };
})();

/* 颜色插值/亮度: JIZURA 的绘制代码里调用极密 (每帧数千次), 而两者都要先把
   颜色字符串解析成 RGB —— 原实现每次都要 split/map/join 建临时数组。 */
(function () {
  var origMix = J.mix, origLum = J.lum;
  J.mix = function (h1, h2, t) {
    var core = JZB.core;
    if (core && typeof h1 === 'string' && typeof h2 === 'string') {
      try { return core.mixColors(h1, h2, t); } catch (e) { JZB.core = null; }
    }
    return origMix(h1, h2, t);
  };
  J.lum = function (h) {
    var core = JZB.core;
    if (core && typeof h === 'string') {
      try { return core.luminance(h); } catch (e) { JZB.core = null; }
    }
    return origLum(h);
  };
})();

/* the QML stage reads the API from `JZ` in the bundle's namespace */
JZB.JZ = JZ;
})();
