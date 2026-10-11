#!/usr/bin/env node
/* Sweep many seeds and every frame, and report anything a JIZURA pack throws.
 *
 *   node tools/scan_errors.js --seeds 60 --style noir
 *
 * The Shape backend cannot implement every browser canvas feature, so packs
 * occasionally hit something that is not there.  JIZURA swallows those failures
 * per content item (console.warn), which means a broken pack looks like a
 * missing decoration rather than an error - this scan surfaces them with a
 * stack, so a port or a JIZURA update can be checked in seconds.
 *
 * Exits non-zero when a new failure is found, so it can gate a release.
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
const SEEDS = parseInt(opt('seeds', '40'), 10);
const STYLE = opt('style', 'noir');
const BUNDLE = path.resolve(opt('bundle', path.join(ROOT, 'js', 'jizura.bundle.js')));

const seen = new Map();
const LYRIC_LINES = [
  '夜明けの色を覚えてる', 'ほどけた声が遠くで鳴った', 'ねえ、まだ間に合うかな',
  '透明なままじゃ終われない', '君の名前を風が運ぶ', 'I still remember the light',
  '握った手のひらの熱', 'もう一度だけ歌わせて', '静かな海の底で', '光の粒が落ちてくる'
];

function load() {
  const source = fs.readFileSync(BUNDLE, 'utf8').replace(/^\s*\.pragma[^\n]*\n/, '\n');
  const sandbox = {
    performance: { now: () => Number(process.hrtime.bigint() / 1000n) / 1000 },
    console: {
      log() {},
      error() {},
      warn: function () {
        const a = Array.prototype.slice.call(arguments);
        const key = String(a[0]) + ' ' + String(a[1]);
        if (!seen.has(key)) {
          const err = a.length > 2 ? a[2] : null;
          seen.set(key, err && err.stack ? String(err.stack).split('\n').slice(0, 5).join('\n') : a.slice(2).join(' '));
        }
      }
    },
    Promise, Map, Set, WeakMap, Symbol, Math, JSON, Date, RegExp, String, Number, Object, Array, Error, isNaN, parseInt, parseFloat
  };
  vm.createContext(sandbox);
  vm.runInContext(source, sandbox, { filename: 'jizura.bundle.js' });
  return sandbox;
}

function main() {
  const ctx = load();
  const JZ = ctx.JZB.JZ;
  JZ.install({ fontFamilies: [] });
  const lyrics = LYRIC_LINES.map((t, i) => '[00:' + String(i * 4 + 1).padStart(2, '0') + '.00]' + t).join('\n');

  let frames = 0, plans = 0;
  const t0 = Date.now();
  for (let seed = 1; seed <= SEEDS; seed++) {
    const project = JZ.makeProject({
      lyrics, title: 'scan', artist: 'scan', style: STYLE, seed,
      fx: { motion: 0.8, glitch: 0.8, chroma: 0.9, decor: 1.0, density: 0.8 }
    });
    const res = JZ.plan(project, null);
    if (!res.plan) { console.error('plan failed for seed ' + seed + ': ' + res.error); process.exitCode = 1; continue; }
    plans++;
    const dur = res.plan.duration;
    for (let t = 0; t < dur; t += 0.05) { JZ.frame(t); frames++; }
  }

  console.log('scanned ' + plans + ' plans / ' + frames + ' frames in ' + ((Date.now() - t0) / 1000).toFixed(1) + ' s');
  console.log('inventory: ' + JSON.stringify(JZ.inventory()));
  console.log('pruned fx: ' + JZ.prunedEffects().length);
  if (!seen.size) {
    console.log('OK - no pack reported a failure');
    return;
  }
  console.error('\n' + seen.size + ' distinct failures:');
  for (const [key, stack] of seen) console.error('\n### ' + key + '\n' + stack);
  process.exitCode = 1;
}

main();
