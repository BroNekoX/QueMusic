/* ============================================================================
   JIZURA for QueMusic — fragment 0/4: prelude
   ----------------------------------------------------------------------------
   The whole plugin runtime is generated into ONE QML JavaScript library file by
   tools/build_bundle.py, in this order:

       00_prelude.js      <- this file
       10_recorder.js     the Canvas2D -> display-list recorder
       20_dom.js          window / document / ES polyfills
       <JIZURA src/*.js>  unmodified, in JIZURA's own load order
       <packs>            optionally, more expression packs
       40_bridge.js       metrics, glyphs, renderer patches, the public API

   Bundling into a single script is deliberate: a QML library script shares one
   scope, so JIZURA's `window.J` singleton, its top-level `var` declarations and
   our shim all see each other without touching a single original file.
   ========================================================================== */
var JZB = {};

/* Layer order of one rendered frame.  JIZURA draws background graphic -> back
   decorations -> lyric text -> front decorations -> HUD; the recorder routes
   each operation into the matching layer so the QML side can keep that stacking
   with a fixed number of containers (Shapes cannot be re-ordered at runtime). */
JZB.LAYERS = 5;
JZB.LAYER_BACK = 0;    /* background graphic, back decor, chromatic ghost passes */
JZB.LAYER_UNDER = 1;   /* vectors drawn before the lyric text lands            */
JZB.LAYER_TEXT = 2;    /* lyric text                                           */
JZB.LAYER_OVER = 3;    /* vectors the same layout draws on top of the text     */
JZB.LAYER_FRONT = 4;   /* front decor, HUD, post effects                       */

/* the shared JIZURA singleton; 20_dom hands it to the browser shim as window.J */
JZB.J = {};

/* Upper bound on the operations one recorded frame may contain.  Compositing
   effects (transitions, chromatic splits) replay whole frames through
   drawImage, so without a ceiling a single motion-blur transition can multiply
   the frame cost by an order of magnitude.  The QML stage lowers this when it
   detects that frames no longer fit. */
JZB.opBudget = 1600;

/* And a ceiling on text items, which is the other thing that can explode: some
   layouts repeat a phrase over the whole frame (a "scatter" cut can ask for
   thousands of glyphs).  The QML stage has a fixed Text pool anyway, so
   recording past it would be wasted work. */
JZB.textBudget = 300;

/* Root-frame counter.  A pack may repaint an offscreen canvas every frame
   without ever calling clearRect (it just fills the whole area opaquely, which
   is how a canvas "clears" itself in practice).  Without help the recorder would
   append to that surface's display list forever and every blit would replay the
   whole history - the single worst thing an immediate-mode recording can do.
   The first *write* to a surface in a new root frame wipes it instead; a surface
   that is only painted once and then cached is never written again, so its
   contents survive. */
JZB.rootStamp = 0;

/* Every Frame ever created, so tools can find one that keeps growing.
   OFF by default and that matters: the bundle is a library script that lives as
   long as the engine, while a plugin instance is created and destroyed every
   time the immersive page opens.  A registry that is not cleared therefore keeps
   every display list from every past instance alive for the whole session - an
   unbounded JS heap, which is exactly the kind of pressure that ends in a
   corrupted V4 heap.  tools/scan_errors.js turns it on; the plugin does not. */
JZB.trackFrames = false;
JZB.allFrames = [];
