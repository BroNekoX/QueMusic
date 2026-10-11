#!/usr/bin/env node
/* Structural / visual smoke test for the generated JIZURA bundle, without Qt.
 *
 *   node tools/svg_preview.js --tier recommended --frames 6 --out preview/
 *
 * It loads js/jizura.bundle.js in a bare VM (no DOM: the bundle brings its own
 * shim), plans a sample song, records frames and writes them as SVG.  Handy to
 * check a JIZURA upgrade, a new pack, or a change to the recorder - and to see
 * the display list without starting QueMusic.
 *
 * Font advances come from the shim's built-in approximations here (the QML
 * stage uses Qt's real FontMetrics), so glyph spacing differs slightly from the
 * app.  Geometry, layering, colours and counts are authoritative.
 */
'use strict';
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const ROOT = path.dirname(__dirname);
const args = process.argv.slice(2);
function opt(name, def) {
  const i = args.indexOf('--' + name);
  return i >= 0 && args[i + 1] && args[i + 1][0] !== '-' ? args[i + 1] : def;
}
const BUNDLE = path.resolve(opt('bundle', path.join(ROOT, 'js', 'jizura.bundle.js')));
const OUT = path.resolve(opt('out', path.join(ROOT, 'preview')));
const FRAMES = parseInt(opt('frames', '1'), 10);
const START = parseFloat(opt('t', '1.0'));
const STEP = parseFloat(opt('step', '1.4'), 10);

const LYRICS = [
  '[00:01.00]夜明けの色を覚えてる',
  '[00:05.40]ほどけた声が遠くで鳴った',
  '[00:09.80]ねえ、まだ間に合うかな',
  '[00:14.20]透明なままじゃ終われない',
  '[00:18.60]君の名前を風が運ぶ',
  '[00:23.00]I still remember the light',
  '[00:27.40]握った手のひらの熱',
  '[00:31.80]もう一度だけ歌わせて'
].join('\n');

function load() {
  // `.pragma library` is a QML directive, not JavaScript: drop it for the VM.
  const source = fs.readFileSync(BUNDLE, 'utf8').replace(/^\s*\.pragma\s+\w+\s*\n/, '\n');
  const sandbox = {
    performance: { now: () => Number(process.hrtime.bigint() / 1000n) / 1000 },
    console: { log: () => {}, warn: () => {}, error: () => {} },
    Promise, Map, Set, WeakMap, Symbol, Math, JSON, Date, RegExp, String, Number, Object, Array, Error, isNaN, parseInt, parseFloat
  };
  sandbox.self = sandbox;
  vm.createContext(sandbox);
  vm.runInContext(source, sandbox, { filename: 'jizura.bundle.js' });
  return sandbox;
}

function hex(c) {
  if (!c) return { color: 'none', alpha: 1 };
  if (typeof c === 'object' && c.__grad) c = c.__solid;        // gradients degrade to their mid colour here
  if (typeof c !== 'string' || c[0] !== '#') return { color: c || 'none', alpha: 1 };
  const o = parseInt(c.slice(1, 3), 16) / 255;
  return { color: '#' + c.slice(3), alpha: o };
}
function esc(s) {
  return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

function toSvg(ctx, frame, W, H, scale) {
  const out = [];
  out.push('<svg xmlns="http://www.w3.org/2000/svg" width="' + (W * scale) + '" height="' + (H * scale) + '" viewBox="0 0 ' + W + ' ' + H + '">');
  out.push('<rect width="' + W + '" height="' + H + '" fill="#101012"/>');
  const clips = [];
  const grads = [];
  const clipId = new Map();
  const gradId = new Map();
  const gradRef = (gr) => {
    const k = gr.t + ':' + gr.c.join(',') + ':' + gr.s.map(s => s[0] + s[1]).join(';');
    if (!gradId.has(k)) {
      gradId.set(k, 'g' + gradId.size);
      const stops = gr.s.map(s => {
        const c = hex(s[1]);
        return '<stop offset="' + s[0] + '" stop-color="' + c.color + '" stop-opacity="' + c.alpha.toFixed(3) + '"/>';
      }).join('');
      const body = gr.t === 1
        ? '<radialGradient id="' + gradId.get(k) + '" gradientUnits="userSpaceOnUse" fx="' + gr.c[0] + '" fy="' + gr.c[1] + '" fr="' + gr.c[2] +
          '" cx="' + gr.c[3] + '" cy="' + gr.c[4] + '" r="' + Math.max(0.01, gr.c[5]) + '">' + stops + '</radialGradient>'
        : '<linearGradient id="' + gradId.get(k) + '" gradientUnits="userSpaceOnUse" x1="' + gr.c[0] + '" y1="' + gr.c[1] +
          '" x2="' + gr.c[2] + '" y2="' + gr.c[3] + '">' + stops + '</linearGradient>';
      grads.push(body);
    }
    return gradId.get(k);
  };
  const clipRef = (c) => {
    if (!c) return null;
    const k = c.join(',');
    if (!clipId.has(k)) {
      clipId.set(k, 'c' + clipId.size);
      clips.push('<clipPath id="' + clipId.get(k) + '"><rect x="' + c[0] + '" y="' + c[1] + '" width="' + c[2] + '" height="' + c[3] + '"/></clipPath>');
    }
    return clipId.get(k);
  };
  for (let li = 0; li < frame.layers.length; li++) {
    const L = frame.layers[li];
    out.push('<g data-layer="' + li + '">');
    for (let i = 0; i < L.nv; i++) {
      const o = L.vec[i];
      if (!o.svg && !o.d) continue;
      const d = o.svg || ctx.JZB.emitSvg(o.d);
      const f = hex(o.fill);
      const s = hex(o.stroke);
      let attrs = 'd="' + d + '"';
      attrs += ' fill="' + (o.grad ? 'url(#' + gradRef(o.grad) + ')' : (o.fill ? f.color : 'none')) + '"';
      if (o.fill && !o.grad && f.alpha < 1) attrs += ' fill-opacity="' + f.alpha.toFixed(3) + '"';
      if (o.stroke) {
        attrs += ' stroke="' + s.color + '" stroke-width="' + Math.max(0.3, o.sw || 1) + '"';
        if (s.alpha < 1) attrs += ' stroke-opacity="' + s.alpha.toFixed(3) + '"';
        if (o.cap && o.cap !== 'butt') attrs += ' stroke-linecap="' + o.cap + '"';
        if (o.join && o.join !== 'miter') attrs += ' stroke-linejoin="' + o.join + '"';
      }
      const cid = clipRef(o.clip);
      if (cid) { out.push('<g clip-path="url(#' + cid + ')"><path ' + attrs + '/></g>'); }
      else out.push('<path ' + attrs + '/>');
    }
    for (let j = 0; j < L.nt; j++) {
      const o = L.txt[j];
      if (!o.t) continue;
      const box = ctx.JZB.metrics.box(o.fam, o.weight, o.italic, o.px);
      const m = o.m || [1, 0, 0, 1];
      const col = hex(typeof o.col === 'string' ? o.col : null);
      const tr = 'matrix(' + m[0] + ',' + m[1] + ',' + m[2] + ',' + m[3] + ',' + o.x + ',' + o.y + ')';
      const cid = clipRef(o.clip);
      const el = '<text transform="' + tr + '" x="0" y="' + box.ascent.toFixed(2) + '" font-family="' + esc(o.fam || 'sans-serif') +
        '" font-size="' + o.px.toFixed(2) + '" font-weight="' + o.weight + '" fill="' + col.color + '"' +
        (col.alpha < 1 ? ' fill-opacity="' + col.alpha.toFixed(3) + '"' : '') + '>' + esc(o.t) + '</text>';
      out.push(cid ? '<g clip-path="url(#' + cid + ')">' + el + '</g>' : el);
    }
    out.push('</g>');
  }
  if (clips.length || grads.length) out.unshift('<defs>' + grads.join('') + clips.join('') + '</defs>');
  out.push('</svg>');
  return out.join('\n');
}

function main() {
  if (!fs.existsSync(BUNDLE)) { console.error('bundle not found: ' + BUNDLE); process.exit(2); }
  const ctx = load();
  const JZ = (ctx.JZB && ctx.JZB.JZ) || ctx.JZ;
  if (!JZ) { console.error('bundle did not expose JZ (JZB=' + typeof ctx.JZB + ')'); process.exit(3); }

  JZ.install({ fontFamilies: [] });
  const project = JZ.makeProject({
    lyrics: LYRICS, title: 'JIZURA × QueMusic', artist: 'smoke test',
    style: opt('style', 'noir'), aspect: opt('aspect', '16:9'), seed: 20260922,
    fx: { motion: 0.7, glitch: 0.55, chroma: 0.7, decor: 0.5, density: 0.55, texture: 0.6, koma: 12 }
  });
  const res = JZ.plan(project, null);
  if (!res.plan) { console.error('plan failed: ' + res.error); console.error(JZ.errors().join('\n')); process.exit(4); }
  const d = JZ.design();
  console.log('plan      : ' + d.W + 'x' + d.H + '  ' + d.duration.toFixed(2) + 's  ' +
    res.plan.cuts.length + ' cuts, ' + res.plan.events.length + ' events  (' + res.ms.toFixed(1) + ' ms)');
  console.log('pruned fx : ' + (JZ.prunedEffects() || []).join(', '));
  const J = ctx.JZB.J;
  console.log('packs     : layouts=' + J.LAYOUT_ORDER.length + ' enter=' + J.ENTER_ORDER.length +
    ' hold=' + J.HOLD_ORDER.length + ' exit=' + J.EXIT_ORDER.length +
    ' decor=' + J.DECOR_ORDER.length + ' treat=' + J.TREAT_ORDER.length +
    ' bg=' + J.BG_ORDER.length + ' cam=' + J.CAMERA_ORDER.length +
    ' fx=' + J.FXE_ORDER.length + ' trans=' + J.TRANS_ORDER.length +
    ' styles=' + J.STYLE_ORDER.length);

  const usedLayouts = new Set(), usedEnter = new Set(), usedDecor = new Set();
  for (const c of res.plan.cuts) {
    usedLayouts.add(c.layout); usedEnter.add(c.enter); usedEnter.add(c.exit);
    for (const dd of c.decor || []) usedDecor.add(dd.id);
  }
  console.log('used      : layouts=' + [...usedLayouts].join(','));
  console.log('            motion=' + [...usedEnter].join(','));
  console.log('            decor=' + [...usedDecor].join(','));

  fs.mkdirSync(OUT, { recursive: true });
  const SWEEP = parseInt(opt('sweep', '0'), 10);
  if (SWEEP > 0) {
    // dense sweep over the whole song: the real worst case the QML stage must draw
    const dur = res.plan.duration;
    let wPaths = 0, wTexts = 0, sPaths = 0, sTexts = 0, wMs = 0, sMs = 0, worstT = 0, worstLayout = '';
    for (let i = 0; i < SWEEP; i++) {
      const t = dur * (i + 0.5) / SWEEP;
      const t0 = Date.now();
      const frame = JZ.frame(t);
      const ms = Date.now() - t0;
      if (!frame) continue;
      const st = JZ.stats();
      sPaths += st.paths; sTexts += st.texts; sMs += ms;
      if (st.texts > wTexts) { wTexts = st.texts; worstT = t; const wc = J.cutAt(res.plan, t); worstLayout = wc ? wc.layout : ''; }
      wPaths = Math.max(wPaths, st.paths);
      wMs = Math.max(wMs, ms);
    }
    console.log('\nsweep ' + SWEEP + ' frames over ' + dur.toFixed(1) + 's:');
    console.log('  paths : avg ' + (sPaths / SWEEP).toFixed(1) + '  max ' + wPaths);
    console.log('  texts : avg ' + (sTexts / SWEEP).toFixed(1) + '  max ' + wTexts + (worstLayout ? '  (' + worstLayout + ' @ ' + worstT.toFixed(1) + 's)' : ''));
    console.log('  record: avg ' + (sMs / SWEEP).toFixed(2) + ' ms  max ' + wMs + ' ms (Node, no rasterising)');
  }
  let worst = 0, sum = 0, maxPaths = 0, maxTexts = 0;
  for (let f = 0; f < FRAMES; f++) {
    const t = START + f * STEP;
    const t0 = Date.now();
    const frame = JZ.frame(t);
    const ms = Date.now() - t0;
    if (!frame) { console.error('frame ' + t + ' returned null'); break; }
    const st = JZ.stats();
    maxPaths = Math.max(maxPaths, st.paths); maxTexts = Math.max(maxTexts, st.texts);
    worst = Math.max(worst, ms); sum += ms;
    const cut = J.cutAt(res.plan, t);
    const svg = toSvg(ctx, frame, d.W, d.H, parseFloat(opt('scale', '0.5')));
    const file = path.join(OUT, 'frame_' + String(f).padStart(3, '0') + '_t' + t.toFixed(2) + '.svg');
    fs.writeFileSync(file, svg);
    console.log('frame t=' + t.toFixed(2).padStart(6) + '  ' + String(ms).padStart(4) + ' ms  ' +
      String(st.paths).padStart(4) + ' paths ' + String(st.texts).padStart(4) + ' texts  layers ' + st.layers.join(' ') +
      (cut ? '   [' + cut.layout + ' / ' + cut.enter + ' / ' + cut.exit + ' / ' + (cut.decor || []).map(x => x.id).join('+') + ']' : ''));
  }
  const errs = JZ.errors();
  if (errs.length) { console.log('\nerrors/warnings (' + errs.length + '):'); errs.slice(0, 20).forEach(e => console.log('  ' + e)); }
  console.log('\nwrote SVG to ' + OUT);
  console.log('average frame: ' + (sum / Math.max(1, FRAMES)).toFixed(1) + ' ms (worst ' + worst + ' ms) in Node, no GPU; ' +
    'the QML stage does the rasterising.');
}

main();
