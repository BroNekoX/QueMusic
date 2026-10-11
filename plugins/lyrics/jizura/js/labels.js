.pragma library
//
// Chinese labels for the JIZURA style catalogue.
//
// JIZURA's own `J.STYLES[key].name` / `.desc` are Japanese, and the registry is
// part of the engine, so the translation lives here instead of being patched
// into the bundle.  Keys are the style keys; [name, description].
//
var STYLE = {
    noir:       ["黑色・色差",   "黑底白字，青色与琥珀色的色差重影"],
    crimson:    ["绯红信号",     "深红底，黑白双栏，数据损坏感"],
    caution:    ["警戒线",       "黄底，红蓝点缀，仪表盘界面"],
    magenta:    ["波普洋红",     "荧光粉配白，粗圆黑体，引线标注"],
    paper:      ["纸与墨",       "纸张质感，靛蓝与洋红，宋体残影"],
    hud:        ["暗色 HUD",     "炭灰底，细线边框，橙色点缀，日食"],
    mint:       ["薄荷终端",     "黑配青绿与柠檬绿，标签贴纸，扫描缝"],
    specimen:   ["标本",         "墨色底，宋体，词典注释与引线"],
    transit:    ["交通标识",     "橄榄绿配黄，箭头与路牌，网点"],
    blueprint:  ["蓝图",         "亮蓝配白与黑，几何拼贴，斜带"],
    rouge:      ["胭脂渐变",     "亮灰底，红色渐变，胶囊形"],
    mono:       ["单色 RGB",     "灰色空间，白色宋体，强烈 RGB 分离，坐标圆"],
    sakura:     ["樱",           "淡樱色，深梅紫，夜樱，圆体与宋体"],
    ocean:      ["深海",         "深蓝深海，青色辉光，气泡与细黑体"],
    sunset:     ["晚霞渐变",     "橙到紫的渐变，粗宋体，逆光"],
    forest:     ["森林手帖",     "苔绿与米白，树皮棕，铅笔手写"],
    vapor:      ["蒸汽波",       "淡紫与粉彩，宋体，VHS 晕染"],
    newsprint:  ["报纸",         "灰色新闻纸，墨与红，标题宋体，CMY 套印错位与网点"],
    synth80:    ["八十年代",     "黑底霓虹洋红与青，立体字，扫描线"],
    kraft:      ["牛皮纸",       "牛皮纸与米白，朱红与靛蓝印章，纸胶带"],
    candy:      ["糖果",         "薄荷、草莓、柠檬、葡萄粉彩，圆润弹跳的字"],
    acid:       ["酸性",         "黑配酸性绿与洋红，粗糙字体与故障画面"],
    sumi:       ["墨与朱",       "和纸米白，毛笔墨字，朱红落款"],
    gold:       ["金夜",         "漆黑与金箔，象牙白宋体，闪烁光点"]
};

function styleName(key) {
    var e = STYLE[key];
    return e ? e[0] : String(key);
}

// the style keys in the engine's own order, so the settings panel can list them
// without holding a reference to the plugin instance (which may be destroyed
// while the panel is still alive)
function styleKeys() {
    var out = [];
    for (var k in STYLE)
        out.push(k);
    return out;
}

function styleDesc(key) {
    var e = STYLE[key];
    return e ? e[1] : "";
}

// true when every key the engine offers has a translation
function missingKeys(keys) {
    var out = [];
    for (var i = 0; i < keys.length; i++)
        if (!STYLE[keys[i]])
            out.push(keys[i]);
    return out;
}
