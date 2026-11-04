// QueMusic 音源插件示例：复制到
//   %APPDATA%/BroNekoX/QueMusic/sources/  (Windows)
//   ~/.config/BroNekoX/QueMusic/sources/  (Linux)
// 重启后出现在音源下拉框里。
//
// meta.id 决定插件在配置里的稳定 source id：改了 id 会被当成另一个音源。
// 所有函数都是可选的；缺哪个函数就只缺那项能力，其它能力不受影响。
var meta = {
    id: "example",
    name: "示例音源",
    version: 1,
    author: "you",
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

// 可选：本地歌词搜词 / B 站无字幕回退会走这里；不写则宿主自动回退到 search()
// q = { keyword, type, page, pageSize }
function searchLyrics(q) {
    return search(q);
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
