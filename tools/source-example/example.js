// 示例音源插件：放到 插件目录（设置 -> 插件 -> 音乐源 -> 打开插件目录）
// 返回播放地址字符串即可；拿不到就 return ""，宿主会继续试下一个音源。
var meta = {
    id: "example-source",
    name: "示例音源",
    version: "1.0.0",
    capabilities: ["music"]
};

// t: { id, hash, platform, quality, name, artist }
// platform: 0 酷狗 / 1 网易云 / 2 B站 / 3 QQ / 4 Spotify / 5 Apple
// quality:  0 低 / 1 标准 / 2 高 / 3 无损
function musicUrl(t) {
    if (!t || !t.id)
        return "";
    // 这里换成真实的取址请求，例如：
    // var r = JSON.parse(http.get("https://api.example.com/url?id=" + t.id + "&q=" + t.quality));
    // return r.url || "";
    return "";
}
