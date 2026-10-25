// QueMusic 音源插件示例：复制到
//   %APPDATA%/BroNekoX/QueMusic/sources/  (Windows)
//   ~/.config/BroNekoX/QueMusic/sources/  (Linux)
// 重启后出现在音源下拉框里。
var meta = {
    id: "example",
    name: "示例音源",
    version: 1,
    capabilities: ["search", "url", "lyrics"],
};

// q = { keyword, type, page, pageSize }
function search(q) {
    var res = http.getJson("https://api.example.com/search?kw=" + encodeURIComponent(q.keyword)
        + "&page=" + q.page);
    return (res.songs || []).map(function (s) {
        return {
            title: s.name,
            artist: s.singer,
            cover: s.pic,
            hash: String(s.id),
            duration: s.duration || 0,
            album: s.album || "",
            paytype: 0,
        };
    });
}

// t = { hash, id, type }，返回播放地址字符串
function musicUrl(t) {
    var res = http.getJson("https://api.example.com/url?id=" + t.hash);
    return res.url || "";
}

// t = { hash, duration }，行内 words 为逐字时间（相对行首，毫秒）
function lyrics(t) {
    var res = http.getJson("https://api.example.com/lyric?id=" + t.hash);
    return {
        info: (res.lines || []).map(function (l) {
            return { time: l.time, text: l.text, info: l.words || null };
        }),
        translate: res.translate || [],
    };
}
