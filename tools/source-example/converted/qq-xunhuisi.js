// QQ 音乐取址（xunhuisi 公开接口，无需登录，返回 320k m4a）
var meta = { id: "qq-xunhuisi", name: "QQ·寻回丝", version: "1.0.0", capabilities: ["music"] };

function musicUrl(t) {
    if (!t || t.platform !== 3 || !t.id) return "";
    try {
        var r = JSON.parse(http.get("https://api.xunhuisi.store/API/QQMusic/Song.php?id="
                                    + encodeURIComponent(t.id)));
        var url = r.music_url || r.url || "";
        return url.indexOf("http") === 0 ? url : "";
    } catch (e) {
        return "";
    }
}
