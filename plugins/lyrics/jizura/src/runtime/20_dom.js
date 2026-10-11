/* ============================================================================
   JIZURA for QueMusic — fragment 2/4: the browser surface JIZURA expects
   ----------------------------------------------------------------------------
   JIZURA is written for a browser: it reads `window`, creates offscreen
   canvases with `document.createElement('canvas')`, measures text with
   `measureText` and touches a few other globals.  This fragment provides that
   surface on top of the recorder, plus the ES2019 built-ins the QML JS engine
   (ES2017) is missing.  No JIZURA source is modified.

   IMPORTANT: everything JIZURA must see has to be a *top-level* declaration of
   the bundle — an IIFE would hide it.  That is why this fragment has no wrapper.
   ========================================================================== */

/* ---- ES2019 built-ins missing from the QML JS engine (Qt 6.10 = ES2017) ---- */
(function () {
'use strict';
if (typeof Object.fromEntries !== 'function') {
  Object.fromEntries = function (iter) {
    var o = {}, a = Array.from(iter);
    for (var i = 0; i < a.length; i++) o[a[i][0]] = a[i][1];
    return o;
  };
}
if (typeof Array.prototype.flat !== 'function') {
  Object.defineProperty(Array.prototype, 'flat', {
    value: function (depth) {
      var d = arguments.length ? depth | 0 : 1, out = [];
      (function walk(a, lv) {
        for (var i = 0; i < a.length; i++) {
          var v = a[i];
          if (Array.isArray(v) && lv > 0) walk(v, lv - 1); else out.push(v);
        }
      })(this, d);
      return out;
    }, writable: true, configurable: true
  });
}
if (typeof Array.prototype.flatMap !== 'function') {
  Object.defineProperty(Array.prototype, 'flatMap', {
    value: function (fn, thisArg) { return Array.prototype.map.call(this, fn, thisArg).flat(); },
    writable: true, configurable: true
  });
}
if (typeof String.prototype.matchAll !== 'function') {
  Object.defineProperty(String.prototype, 'matchAll', {
    value: function (re) {
      var rx = re.global ? re : new RegExp(re.source, re.flags + 'g'), out = [], m;
      rx.lastIndex = 0;
      while ((m = rx.exec(this)) !== null) { out.push(m); if (m[0] === '') rx.lastIndex++; }
      return out;
    }, writable: true, configurable: true
  });
}
if (typeof Promise.allSettled !== 'function') {
  Promise.allSettled = function (list) {
    return Promise.all(Array.from(list).map(function (p) {
      return Promise.resolve(p).then(function (v) { return { status: 'fulfilled', value: v }; },
                                     function (e) { return { status: 'rejected', reason: e }; });
    }));
  };
}
})();

/* ---- browser globals (declared as top-level `var` so JIZURA can see them) ---- */
var __jzWin = {
  J: JZB.J,
  devicePixelRatio: 1,
  innerWidth: 1920, innerHeight: 1080,
  addEventListener: function () {}, removeEventListener: function () {},
  requestAnimationFrame: function () { return 0; },
  cancelAnimationFrame: function () {},
  requestIdleCallback: function () { return 0; },
  setTimeout: function () { return 0; }, clearTimeout: function () {},
  matchMedia: function () { return { matches: false, addListener: function () {} }; },
  getComputedStyle: function () { return { getPropertyValue: function () { return ''; } }; }
};
var __jzDoc = {
  createElement: function (tag) {
    if (tag === 'canvas') return new JZB.Surface(300, 150);
    return { style: {}, setAttribute: function () {}, appendChild: function () {}, addEventListener: function () {},
             classList: { add: function () {}, remove: function () {}, toggle: function () {} }, children: [] };
  },
  createElementNS: function (ns, tag) { return __jzDoc.createElement(tag); },
  createTextNode: function () { return {}; },
  getElementById: function () { return null; },
  querySelector: function () { return null; },
  querySelectorAll: function () { return []; },
  addEventListener: function () {}, removeEventListener: function () {},
  documentElement: { lang: 'zh-Hans', style: {}, classList: { add: function () {} } },
  body: { appendChild: function () {}, addEventListener: function () {}, style: {} },
  head: { appendChild: function () {} },
  hidden: false,
  readyState: 'complete',
  fonts: { add: function () {}, load: function () { return Promise.resolve([]); }, ready: Promise.resolve() }
};

var window = (typeof window !== 'undefined' && window) ? window : __jzWin;
var document = (typeof document !== 'undefined' && document) ? document : __jzDoc;
var navigator = (typeof navigator !== 'undefined' && navigator) ? navigator : { userAgent: 'QtQml', language: 'zh-CN' };
var performance = (typeof performance !== 'undefined' && performance) ? performance : { now: function () { return 0; } };
var Image = (typeof Image !== 'undefined') ? Image : function () { this.width = 0; this.height = 0; this.onload = null; this.onerror = null; };
if (Image.prototype && !Image.prototype.addEventListener) Image.prototype.addEventListener = function () {};
var DOMMatrix = (typeof DOMMatrix !== 'undefined') ? DOMMatrix : undefined;
var Path2D = (typeof Path2D !== 'undefined') ? Path2D : JZB.Path2D;
var FontFace = (typeof FontFace !== 'undefined') ? FontFace : function () { throw new Error('FontFace is unavailable'); };
var AudioContext = (typeof AudioContext !== 'undefined') ? AudioContext : undefined;
var OfflineAudioContext = (typeof OfflineAudioContext !== 'undefined') ? OfflineAudioContext : undefined;
var Blob = (typeof Blob !== 'undefined') ? Blob : function () {};
var URL = (typeof URL !== 'undefined' && URL && URL.createObjectURL) ? URL
        : { createObjectURL: function () { return ''; }, revokeObjectURL: function () {} };
var indexedDB = (typeof indexedDB !== 'undefined') ? indexedDB : undefined;
var requestAnimationFrame = (typeof requestAnimationFrame !== 'undefined') ? requestAnimationFrame : function () { return 0; };
var cancelAnimationFrame = (typeof cancelAnimationFrame !== 'undefined') ? cancelAnimationFrame : function () {};

JZB.window = window;
JZB.document = document;
