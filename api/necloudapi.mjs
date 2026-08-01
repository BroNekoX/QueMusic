// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
// necloudapi.mjs — 网易云音乐 API 模块
// 走 music.163.com/api/ 老口，GET，无加密

// 模块级 sendMessage 引用，由 neteaseHandler 注入
let _sendMessage = null;

export function neteaseHandler(message) {
    _sendMessage = message.sendMessage || WorkerScript.sendMessage;
    switch(message.action) {
    case "searchSongs":        searchSongs(message.keyword, message.type, message.page, message.pageSize); break;
    case "getPlaylistMenu":    getPlaylistMenu(); break;
    case "getMenuInfo":        getMenuInfo(message.id); break;
    case "getMusicPlaylists":  getMusicPlaylists(message.tagid, message.page, message.pageSize); break;
    case "getPlaylistSongs":   getPlaylistSongs(message.listid, message.page, message.pageSize); break;
    case "getRecommendSongs":  getRecommendSongs(message.page, message.pageSize); break;
    case "getHotPlaylistMenu": getHotPlaylistMenu(); break;
    case "getHotPlaylists":    getHotPlaylists(message.page); break;
    case "getNewSongs":        getNewSongs(message.type, message.page, message.pageSize); break;
    case "getMusicToplist":    getMusicToplist(); break;
    case "getMusicInfo":       getMusicInfo(message.id); break;
    case "getLyricInfo":       getLyricInfo(message.id); break;
    default: console.log("neteaseHandler: 未知操作", message.action);
    }
}

function makeGet(url, callback) {
    try {
        var xhr = new XMLHttpRequest();
        xhr.timeout = 3000;
        console.log("正在连接冈毅服务器：", url);
        xhr.responseType = "text";
        xhr.open("GET", url, true);
        xhr.setRequestHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
        xhr.setRequestHeader("Referer", "https://music.163.com/");
        xhr.onload = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                if (xhr.status === 200) {
                    console.log(xhr.responseText.substring(0, 100));
                    var json = JSON.parse(xhr.responseText);
                    callback(null, json);
                } else {
                    callback("HTTP " + xhr.status);
                }
            }
        };
        xhr.send();
    } catch(e) {
        callback(e.message);
    }
}

// 搜索 type:0=单曲 1=歌单 2=专辑（外部传的 0/1/2 跟网易云 web口 type 1/1000/10 对应）
function searchSongs(keyword, type, page, pageSize) {
    var ntype = (type === 1) ? 1000 : (type === 2) ? 10 : 1; // 默认单曲
    var offset = (page - 1) * pageSize;

    var url = "http://music.163.com/api/search/get/web?csrf_token="
            + "&s=" + encodeURIComponent(keyword)
            + "&type=" + ntype
            + "&offset=" + offset
            + "&total=true&limit=" + pageSize;
    makeGet(url, function(err, json) {
        if (err) { console.log("searchSongs err:", err); return; }
        var info = [];
        if (ntype === 1 && json.result && json.result.songs) {
            json.result.songs.forEach(function(s) {
                info.push({
                    title: s.name,
                    artist: s.artists[0].name || "Artist",
                    cover: s.album && s.album.artist ? s.album.artist.img1v1Url : "",
                    hash: String(s.id),
                    hashhq: String(s.id),
                    hashsq: String(s.id),
                    paytype: s.fee === 1 ? 1 : s.fee === 8 ? 0 : 3,
                    duration: Math.floor(s.duration / 1000) || 0,
                    album: s.album ? s.album.name : ""
                });
            });
        } else if (ntype === 1000 && json.result && json.result.playlists) {
            json.result.playlists.forEach(function(p) {
                info.push({
                    title: p.name,
                    artist: p.creator ? p.creator.nickname : "",
                    cover: p.coverImgUrl || "",
                    hash: String(p.id),
                    duration: p.trackCount,
                    album: p.description || "",
                    playcount: p.playCount || 0
                });
            });
        } else if (ntype === 10 && json.result && json.result.albums) {
            json.result.albums.forEach(function(a) {
                info.push({
                    title: a.name,
                    artist: a.artist ? a.artist.name : "",
                    cover: a.picUrl || "",
                    hash: String(a.id),
                    duration: 0,
                    album: a.name
                });
            });
        }
        _sendMessage({ type: "searchSongs", data: info });
    });
}

// 歌单分类  /playlist/catlist
function getPlaylistMenu() {
    makeGet("http://music.163.com/api/playlist/catlist", function(err, json) {
        // 网易云返回 categories + sub，需要拍平给 UI
        // 简易版：把 sub 每项当 menu，带 category 名
        var info = [];
        if (json.categories && json.sub) {
            json.sub.forEach(function(s) {
                info.push({
                    title: s.name,
                    id: s.id,
                    category: json.categories[s.category].name
                });
            });
        }
        _sendMessage({ type: "getPlaylistMenu", data: { info: info } });
    });
}

// 分类 tag 信息（网易云 catlist 里 id 就是 tag，直接透）
function getMenuInfo(id) {
    _sendMessage({
        type: "getMenuInfo",
        data: { special_tag_id: id, id: id, name: "" }
    });
}

// 分类下歌单  /top/playlist?cat= + limit/offset
function getMusicPlaylists(tagid, page, pageSize) {
    var offset = (page - 1) * pageSize;
    var url = "http://music.163.com/api/playlist/list?cat=" + encodeURIComponent(tagid)
            + "&order=hot&offset=" + offset + "&limit=" + pageSize;
    makeGet(url, function(err, json) {
        var info = [];
        if (json.playlists) {
            json.playlists.forEach(function(p) {
                info.push({
                    title: p.name,
                    artist: p.creator ? p.creator.nickname : "",
                    cover: p.coverImgUrl || "",
                    hash: String(p.id),
                    duration: p.trackCount,
                    album: p.description || ""
                });
            });
        }
        _sendMessage({ type: "getMusicPlaylists", data: { info: info } });
    });
}

// 歌单内歌曲  /playlist/track/all
function getPlaylistSongs(listid, page, pageSize) {
    var url = "http://music.163.com/api/v6/playlist/detail?id=" + listid + "&limit=" + pageSize + "&offset=" + ((page-1)*pageSize);
    makeGet(url, function(err, json) {
        if (err) { console.log("getPlaylistSongs err:", err); return; }
        var info = [];
        if (json.playlist && json.playlist.tracks) {
            json.playlist.tracks.forEach(function(s) {
                info.push({
                    title: s.name,
                    artist: s.ar && s.ar[0] ? s.ar[0].name : (s.artists && s.artists[0] ? s.artists[0].name : ""),
                    cover: s.al && s.al.picUrl ? s.al.picUrl : (s.album && s.album.picUrl ? s.album.picUrl : ""),
                    hash: String(s.id),
                    hashhq: String(s.id),
                    hashsq: String(s.id),
                    paytype: s.fee === 1 ? 1 : s.fee === 8 ? 0 : 3,
                    duration: Math.floor(s.duration / 1000) || 0,
                    album: s.al ? s.al.name : (s.album ? s.album.name : "")
                });
            });
        }


        _sendMessage({ type: "getPlaylistSongs", data: { info: info } });
    });
}

// 推荐歌曲  用 /personalized/newsong 或 rank/newsong
function getRecommendSongs(page, pageSize) {
    var url = "http://music.163.com/api/personalized/newsong?limit=" + pageSize + "&offset=" + ((page-1)*pageSize);
    makeGet(url, function(err, json) {
        var info = [];
        if (json.result) {
            json.result.forEach(function(s) {
                var song = s.song || s;
                info.push({
                    title: song.name,
                    artist: song.artists && song.artists[0] ? song.artists[0].name : "",
                    cover: song.album && song.album.picUrl ? song.album.picUrl : "",
                    hash: String(song.id) || "",
                    duration: song.duration || 0,
                    album: song.album ? song.album.name : "",
                    hashhq: String(song.id) || "",
                    hashsq: String(song.id) || "",
                    paytype: song.pay_type || 1,
                    duration: song.duration || 0
                });
            });
        }
        _sendMessage({ type: "getRecommendSongs", data: { info: info } });
    });
}

// 热门歌单分类  /playlist/hot
function getHotPlaylistMenu() {
    makeGet("http://music.163.com/api/playlist/hot", function(err, json) {
        var info = [];
        if (json.tags) {
            json.tags.forEach(function(t) {
                info.push({ title: t.name, id: t.id });
            });
        }
        _sendMessage({ type: "getHotPlaylistMenu", data: { info: info } });
    });
}

// 热门歌单  /top/playlist?order=new
function getHotPlaylists(page) {
    var url = "https://music.163.com/api/playlist/list?cat='全部'&limit=20&offset=" + ((page-1)*20);
    makeGet(url, function(err, json) {
        var info = [];
        if (json.playlists) {
            json.playlists.forEach(function(p) {
                info.push({
                    title: p.name || "Unknown",
                    artist: p.creator.nickname || "Unknown",
                    cover: p.coverImgUrl || "",
                    hash: String(p.id),
                    duration: p.trackCount || 0,
                    album: p.description || "",
                    playcount: p.playCount || 0
                });
            });
        }
        _sendMessage({ type: "getHotPlaylists", data: { info: info } });
    });
}

// 新歌  /top/song?type= （7华语 96欧美 8日本 16韩国，外部传 1/2/3 要 map）
function getNewSongs(type, page, pageSize) {
    var ntype = 0
    switch(type) {
        case 1: ntype = 7; break;
        case 2: ntype = 96; break;
        case 3: ntype = 8; break;
        case 4: ntype = 16; break;
        case 5: ntype = 8; break;
    }
    var url = "http://music.163.com/api/v1/discovery/new/songs?areaId=" + ntype + "&limit=" + pageSize + "&offset=" + ((page-1)*pageSize);
    makeGet(url, function(err, json) {
        var info = [];
        if (json.data) {
            json.data.forEach(function(s) {
                info.push({
                    title: s.name,
                    artist: s.artists[0].name || "Artist",
                    cover: s.album.picUrl || s.album.artist.img1v1Url || "",
                    hash: String(s.id),
                    hashhq: String(s.id),
                    hashsq: String(s.id),
                    paytype: 0,
                    duration: Math.floor(s.duration / 1000) || 0,
                    album: s.album ? s.album.name : ""
                });
            });
        }
        _sendMessage({ type: "getNewSongs", data: { info: info } });
    });
}

// 排行榜列表  /toplist
function getMusicToplist() {
    makeGet("http://music.163.com/api/toplist", function(err, json) {
        var info = [];
        if (json.list) {
            json.list.forEach(function(t) {
                info.push({
                    title: t.name,
                    artist: t.updateFrequency || "",
                    cover: t.coverImgUrl || "",
                    hash: String(t.id),
                    duration: 0,
                    album: t.description || ""
                });
            });
        }
        _sendMessage({ type: "getMusicToplist", data: { info: info } });
        // 注意：点了某个榜单后，用 getPlaylistSongs(listid) 拉，因为榜单本质是个歌单
    });
}

// 获取歌曲播放信息
function getMusicInfo(id) {
    // 先获取歌曲详情（封面、歌手等）
    var detailUrl = "http://music.163.com/api/song/detail/?ids=[" + id + "]";
    makeGet(detailUrl, function(err, json) {
        if (err) { console.log("getMusicInfo detail err:", err); return; }
        var s = json.songs && json.songs[0];
        var playUrl = "http://music.163.com/song/media/outer/url?id=" + id + ".mp3";
        _sendMessage({
            type: "getMusicInfo",
            data: {
                backup_url: playUrl,
                url: playUrl,
                songName: s.name || "",
                author_name: s && s.ar ? s.ar[0].name : (s && s.artists ? s.artists[0].name : ""),
                singer_img: s.artists.picUrl || "",
                album_img: s && s.al ? s.al.picUrl : (s && s.album ? s.album.picUrl : ""),
                timeLength: Math.floor(s.duration || 0) / 1000 || 0,
                fileName: (s ? s.name : "unknown") + ".mp3",
                hash: String(id),
                type: 0   // 默认为播放
            },
            source: 1
        });
    });
}

// 歌词 /lyric?id= ，返回 { lrc: { lyric: "..." } }，标准 [mm:ss.xx] 格式
function getLyricInfo(id) {
    var url = "http://music.163.com/api/song/lyric/v1?id=" + id + "&cp=false&tv=0&lv=0&rv=0&kv=0&yv=0&ytv=0&yrv=0";
    makeGet(url, function(err, json) {
        if (err) { console.log("getLyricInfo err:", err); return; }
        var lrc = "";
        if (json.lrc && json.lrc.lyric) {
            lrc = json.lrc.lyric;
        }

        var lyricList = lyricCout(lrc);
        var lyricTransList = [];

        if (json.tlyric && json.tlyric.lyric) {
            var trc = json.tlyric.lyric;
            var lyricTrans = lyricCout(trc);
            lyricList.forEach(function(t,index) {
                var text = "";
                for (var i = index; i >= 0; i--) {//(var i = index; i >= 0; i--)
                    if(lyricTrans[i]) {
                        if (lyricTrans[i].time == t.time) {
                            text = lyricTrans[i].text;
                            break;
                        }
                    }
                }
                console.log("歌词翻译行text：",text)
                lyricTransList.push(text);
            });
        }

        _sendMessage({ type: "getLyricInfo", data: { info: lyricList, translate: lyricTransList } });
    });
}

// 歌词转list
function lyricCout(lyrics) {
    var lines = lyrics.split("[");
    console.log("获取歌词行",lines);
    var result = [];
    for (var i = 1; i < lines.length; i++) {  // 跳过第一个空串
        var part = lines[i];                   // 例如 "02:13.45] 歌词内容"
        // 找到 ']' 的位置
        var closeBracket = part.indexOf(']');
        if (closeBracket === -1) continue;     // 格式异常，跳过

        // 提取时间字符串和歌词文本
        var timeStr = part.substring(0, closeBracket);
        var text = part.substring(closeBracket + 1).trim();
        if (text === "") continue;             // 跳过空歌词

        // 解析时间字符串，支持多种格式：[mm:ss.xx] [mm:ss.xxx] [mm:ss:xx]
        var match = timeStr.match(/^(\d{2}):(\d{2})[.:](\d{2,3})$/);
        if (!match) continue;

        var min = parseInt(match[1], 10);
        var sec = parseInt(match[2], 10);
        var msStr = match[3];
        var ms = msStr.length === 2 ? parseInt(msStr + '0', 10) : parseInt(msStr, 10);
        var time = min * 60000 + sec * 1000 + ms;

        result.push({ time: time, text: text, info: null });
    }
    // 按时间排序
    result.sort((a, b) => a.time - b.time);
    return result;
}