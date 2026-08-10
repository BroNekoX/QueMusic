// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
// kugouapi.js
import { inflate, inflateRaw } from './pako.mjs';

// 模块级 sendMessage 引用，由 kugouHandler 注入
let _sendMessage = null;
let _cookie = ''; // 登录态 Cookie（来自 AccountManager）

export function kugouHandler(message) {
    _sendMessage = message.sendMessage || WorkerScript.sendMessage;
    _cookie = message.cookie || '';
    switch(message.action) {
        case "searchSongs":        searchSongs(message.keyword, message.type, message.page, message.pageSize); break;
        case "getPlaylistMenu":    getPlaylistMenu(message.type); break;
        case "getMenuInfo":        getMenuInfo(message.id); break;
        case "getMusicPlaylists":  getMusicPlaylists(message.tagid, message.page, message.pageSize); break;
        case "getPlaylistSongs":   getPlaylistSongs(message.listid, message.page, message.pageSize); break;
        case "getRecommendSongs":  getRecommendSongs(message.page, message.pageSize); break;
        case "getHotPlaylistMenu": getHotPlaylistMenu(message.type); break;
        case "getHotPlaylists":    getHotPlaylists(message.page); break;
        case "getNewSongs":        getNewSongs(message.type, message.page, message.pageSize); break;
        case "getAllToplist":      getAllToplist(message.type); break;
        case "getMusicToplist":    getMusicToplist(message.dateTime); break;
        case "getMusicInfo":       getMusicInfo(message.id,message.type); break;
        case "getLyricInfo":       getLyricInfo(message.id, message.duration); break;
        default: console.log("未知操作:", message.action);
    }
}


// 同步 XHR 封装，返回回调
function makeRequest(url, callback) {
    try {
        var xhr = new XMLHttpRequest();
        xhr.timeout = 3000;
        xhr.open("GET", url, true);
        xhr.responseType = "text";
        console.log("url: ",url)
        // 登录后携带用户自己的 Cookie（合规：用自己账号的授权身份访问）
        if (_cookie) {
            console.log("Cookie:",_cookie)
            xhr.setRequestHeader("Cookie", _cookie);
        }
        xhr.onload = function() {
            if (xhr.status === 200) {
                try {
                    var getstr = xhr.responseText || xhr.response;
                    var jsonString = getstr.replace("<!--KG_TAG_RES_START-->","").replace("<!--KG_TAG_RES_END-->","");
                    var clearJsonStr = jsonString.trim();

                    var responselist = JSON.parse(clearJsonStr);
                    callback(null, responselist);
                } catch (e) {
                    callback("Json Script Error: " + e.message);
                }
            } else {
                callback("HTTP " + xhr.status);
            }
        };
        xhr.send();
    } catch(e) {
        callback("XHR Error: " + e.message);
    }
}

//  搜索歌曲
function searchSongs(keyword, type, page, pageSize) {
    var url = "";
    switch(type) {
        case 0: // 歌曲
            url = "http://mobilecdn.kugou.com/api/v3/search/song?format=json&pagesize=" + pageSize + "&page=" + page + "&keyword=" + encodeURIComponent(keyword);
            break;
        case 1: // 歌单
            url = "http://mobilecdnbj.kugou.com/api/v3/search/special?version=9108&highlight=em&filter=0&sver=2&with_res_tag=1&pagesize=" + pageSize + "&page=" + page + "&keyword=" + keyword;
            break;
        case 2: // 专辑
            url = "http://msearch.kugou.com/api/v3/search/album?version=9108&iscorrection=1&highlight=em&plat=0&sver=2&with_res_tag=1&pagesize=" + pageSize + "&page=" + page + "&keyword=" + keyword;
            break;
        case 3: // 歌词
            url = "http://mobileservice.kugou.com/api/v3/lyric/search?version=9108&highlight=1&plat=0&area_code=1&with_res_tag=1&pagesize=" + pageSize + "&page=" + page + "&keyword=" + keyword;
            break;
    }

    makeRequest(url, function(err, json) {
        if (err) { console.log("searchSongs error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (type === 0 && data.info) {
            // 歌曲搜索
            data.info.forEach(function(s) {
                info.push({
                    title: s.songname || "",
                    artist: s.singername || "",
                    cover: s.trans_param.union_cover || s.cover || "",
                    hash: s.hash || "",
                    hashhq: s.pay_type_320 !== 3 ? (s["320hash"] || s.trans_param.ogg_320_hash) : s.hash || "",
                    hashsq: s.pay_type_sq === 0 ? s.sqhash : s.hash || "",
                    paytype: s.pay_type || 1,
                    duration: s.duration || 0,
                    album: s.album_name || ""
                });
            });
        } else if (type === 1 && data.info) {
            // 歌单搜索
            data.info.forEach(function(s) {
                info.push({
                    title: s.specialname || "",
                    artist: s.nickname || "",
                    cover: s.imgurl || s.cover || "",
                    hash: String(s.specialid) || "",
                    duration: s.songcount || 0,
                    album: s.intro || "",
                    playcount: s.playcount || 0
                });
            });
        } else if (type === 2 && data.info) {
            // 专辑搜索
            data.info.forEach(function(a) {
                info.push({
                    title: a.albumname || "",
                    artist: a.singername || "",
                    cover: a.imgUrl || "",
                    hash: String(a.albumid) || "",
                    duration: 0,
                    album: a.albumname || ""
                });
            });
        }
        _sendMessage({ type: "searchSongs", data: { info: info } });
    });
}

// 获取歌单分类列表
function getPlaylistMenu(type) {
    var url = "http://mobilecdnbj.kugou.com/api/v3/tag/list?pid=0&apiver=2&plat=0";
    makeRequest(url, function(err, json) {
        if (err) { console.log("getPlaylistMenu error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (data.info) {
            data.info.forEach(function(tag) {
                info.push({
                    title: tag.tagname || "",
                    id: tag.tagid || "",
                    category: tag.parentname || ""
                });
            });
        }
        WorkerScript.sendMessage({ type: "getPlaylistMenu", data: { info: info } });
    });
}

// 获取分类信息
function getMenuInfo(id) {
    var url = "http://mobilecdnbj.kugou.com/api/v3/tag/info?apiver=2&id=" + id;
    makeRequest(url, function(err, json) {
        if (err) { console.log("getMenuInfo error:", err); return; }
        var data = json.data || json;
        WorkerScript.sendMessage({
            type: "getMenuInfo",
            data: data
        });
    });
}

// 获取分类下的歌单列表
function getMusicPlaylists(tagid, page, pageSize) {
    var url = "http://mobilecdnbj.kugou.com/api/v3/tag/specialList?plat=0&ugc=1&sort=2&pagesize=" + pageSize + "&page=" + page + "&tagid=" + tagid;
    makeRequest(url, function(err, json) {
        if (err) { console.log("getMusicPlaylists error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (data.info) {
            data.info.forEach(function(s) {
                info.push({
                    title: s.specialname || "",
                    artist: s.username || "",
                    cover: s.imgurl.replace("{size}","64") || "",
                    hash: s.specialid || "",
                    duration: s.playcount,
                    album: s.intro || ""
                });
            });
        }
        WorkerScript.sendMessage({ type: "getMusicPlaylists", data: { info: info } });
    });
}

// 获取歌单内的歌曲
function getPlaylistSongs(listid, page, pageSize) {
    var url = "http://mobilecdnbj.kugou.com/api/v3/special/song?plat=0&version=9108&with_res_tag=1&pagesize=" + pageSize + "&page=" + page + "&specialid=" + listid;
    makeRequest(url, function(err, json) {
        if (err) { console.log("getPlaylistSongs error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (data.info) {
            data.info.forEach(function(s) {
                var fileStr = s.filename.split("-");
                info.push({
                    title: fileStr[1].trim() || s.filename || "",
                    artist: fileStr[0].trim() || "",
                    cover: s.trans_param.union_cover || s.imgurl || "",
                    hash: s.hash || "",
                    hashhq: s.pay_type_320 === 0 ? (s["320hash"] || s.hash) : s.hash || "",
                    hashsq: s.pay_type_sq === 0 ? s.sqhash : s.hash || "",
                    paytype: s.pay_type || 0,
                    duration: s.duration || 0,
                    album: s.album_name || ""
                });
            });
        }
        WorkerScript.sendMessage({ type: "getPlaylistSongs", data: { info: info } });
    });
}

// 推荐歌曲（新歌榜）
function getRecommendSongs(page, pageSize) {
    var url = "http://mobilecdnbj.kugou.com/api/v3/rank/newsong?format=json&version=9108&plat=0&with_cover=1&type=1&area_code=1&with_res_tag=1&pagesize=" + pageSize + "&page=" + page;
    makeRequest(url, function(err, json) {
        if (err) { console.log("getRecommendSongs error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (data.info) {
            data.info.forEach(function(s) {
                info.push({
                    title: s.songname || "",
                    artist: s.singername || "",
                    cover: s.trans_param.union_cover || s.cover || "",
                    hash: s.hash || "",
                    hashhq: s.pay_type_320 !== 3 ? (s["320hash"] || s.trans_param.ogg_320_hash) : s.hash || "",
                    hashsq: s.pay_type_sq === 0 ? s.sqhash : s.hash || "",
                    paytype: s.pay_type || 1,
                    duration: s.duration || 0,
                    album: s.album_name || ""
                });
            });
        }
        WorkerScript.sendMessage({ type: "getRecommendSongs", data: { info: info } });
    });
}

// 热门歌单分类
function getHotPlaylistMenu(type) {
    var url = "http://mobilecdnbj.kugou.com/api/v3/tag/recommend?apiver=2&plat=0&showtype=" + type;
    makeRequest(url, function(err, json) {
        if (err) { console.log("getHotPlaylistMenu error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (data.info) {
            data.info.forEach(function(tag) {
                info.push({
                    title: tag.name || "",
                    id: tag.id || "",
                    tagid: tag.special_tag_id || "",
                    cover: tag.bannerurl || ""
                });
            });
        }
        WorkerScript.sendMessage({ type: "getHotPlaylistMenu", data: { info: info } });
    });
}

// 热门歌单
function getHotPlaylists(page) {
    var url = "http://mobilecdnbj.kugou.com/api/v5/special/recommend?recommend_expire=0&sign=52186982747e1404d426fa3f2a1e8ee4&plat=0&uid=0&version=9108&area_code=1&appid=1005&mid=286974383886022203545511837994020015101&_t=1545746286&page=" + page;
    makeRequest(url, function(err, json) {
        if (err) { console.log("getHotPlaylists error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (data.list) {
            data.list.forEach(function(s) {
                info.push({
                    title: s.specialname || "",
                    artist: s.nickname || "",
                    cover: s.imgurl || "",
                    hash: String(s.specialid) || "",
                    duration: s.songcount,
                    album: s.intro || "",
                    playcount: s.playcount || ""
                });
            });
        }
        WorkerScript.sendMessage({ type: "getHotPlaylists", data: { info: info } });
    });
}

// 新歌（type:1华语 2欧美 3日韩）
function getNewSongs(type, page, pageSize) {
    var url = "http://mobilecdnbj.kugou.com/api/v3/rank/newsong?format=json&version=9108&plat=0&with_cover=1&area_code=1&with_res_tag=1&pagesize=" + pageSize + "&page=" + page + "&type=" + type;
    makeRequest(url, function(err, json) {
        if (err) { console.log("getNewSongs error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (data.info) {
            data.info.forEach(function(s) {
                info.push({
                    title: s.songname || "",
                    artist: s.authors[1] ? (s.authors[0].author_name + "," + s.authors[1].author_name) : s.authors[0].author_name || "",
                    cover: s.trans_param.union_cover || s.cover || "",
                    hash: s.hash || "",
                    hashhq: s.pay_type_320 !== 3 ? (s["320hash"] || s.trans_param.ogg_320_hash) : s.hash || "",
                    hashsq: s.pay_type_sq === 0 ? s.sqhash : s.hash || "",
                    paytype: s.pay_type || 1,
                    duration: s.duration || 0,
                    album: s.album_name || ""
                });
            });
        }
        WorkerScript.sendMessage({ type: "getNewSongs", data: { info: info } });
    });
}

// 获取所有排行榜
function getAllToplist(type) {
    // 直接返回一个固定榜单列表
    var info = [
        { title: "酷狗飙升榜", hash: "6666", cover: "", artist: "", duration: 0, album: "" },
        { title: "酷狗TOP500", hash: "8888", cover: "", artist: "", duration: 0, album: "" }
    ];
    WorkerScript.sendMessage({ type: "getAllToplist", data: { info: info } });
}

// 获取排行榜歌曲
function getMusicToplist(dateTime) {
    var url = "http://mobilecdnbj.kugou.com/api/v3/rank/song?version=9108&ranktype=2&plat=0&pagesize=100&area_code=1&page=1&volid=" + dateTime + "&rankid=6666&with_res_tag=1";
    makeRequest(url, function(err, json) {
        if (err) { console.log("getMusicToplist error:", err); return; }
        var info = [];
        var data = json.data || json;
        if (data.info) {
            data.info.forEach(function(s) {
                info.push({
                    title: s.filename || s.songname || "",
                    artist: s.singername || "",
                    cover: s.imgUrl || "",
                    hash: s.hash || "",
                    duration: s.duration || 0,
                    album: s.album_name || ""
                });
            });
        }
        WorkerScript.sendMessage({ type: "getMusicToplist", data: { info: info } });
    });
}

// 获取歌曲播放信息
function getMusicInfo(hash,type) {
    var url = "https://m.kugou.com/app/i/getSongInfo.php?cmd=playInfo&hash=" + hash;

    makeRequest(url, function(err, json) {
        if (err) { console.log("getMusicInfo error:", err); return; }
        var data = json.data || json;
        WorkerScript.sendMessage({
            type: "getMusicInfo",
            data: {
                backup_url: data.backup_url || data.playUrl || "",
                url: data.url || data.backup_url || "",
                songName: data.songName || data.fileName || "Unknown",
                author_name: data.author_name || data.singername || "Unknown",
                singer_img: data.imgUrl || data.album_img || "",
                album_img: data.trans_param.union_cover || data.album_img || "",
                timeLength: data.timeLength || data.duration || 0,
                fileName: data.fileName + "." + data.extName || data.songName + ".mp3" || "unknown.mp3",
                hash: hash,
                type: type //0.播放 1.下载 2.收藏
            },
            source: 0
        });
    });
}

// 获取歌词信息
function getLyricInfo(hash, duration) {
    // 1搜索歌词候选
    var searchUrl = "http://lyrics.kugou.com/search?ver=1&man=yes&client=pc&duration=" + duration + "&hash=" + hash;
    makeRequest(searchUrl, function(err, json) {
        if (err || !json || !json.candidates || json.candidates.length === 0) {
            WorkerScript.sendMessage({ type: "getLyricInfo", data: "" }); // 无歌词
            return;
        }
        var candidate = json.candidates[0];
        var id = candidate.id;
        console.log("id:",id,"  accessKey:",candidate.accesskey)
        var accesskey = candidate.accesskey;
        // 2下载歌词
        var downloadUrl = "http://lyrics.kugou.com/download?ver=1&client=pc&id=" + id + "&accesskey=" + accesskey + "&fmt=krc&charset=utf8";
        makeRequest(downloadUrl, function(err2, json2) {
            if (!json2 || !json2.content) {
                WorkerScript.sendMessage({ type: "getLyricInfo", data: "" });
                return;
            }
            //console.log("lyricBase:",json2.content)
            var base64 = json2.content;
            console.log("开始解码");
            var lrc = Qt.atob(base64); // 解码base64
            //var lyricList = lyricCout(base64);
            //var lyricbase = decodeKrc(base64);
            var lyricString = krcCout(base64);
            const lyricList = krcTolist(lyricString);
            console.log("krcString:",lyricString);
            const lyricTransList = extractTranslations(lyricString);
            console.log("krcTranslateString:",lyricTransList);
            WorkerScript.sendMessage({ type: "getLyricInfo", data: { info: lyricList, translate: lyricTransList } });
        });
    });
}

// 酷狗歌词转list
function lrcCout(base64) {
    var lyrics = Qt.atob(base64);
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

function krcCout(base64Str) {
    const base64 = base64Str;
    const byteData = base64DecodeToUint8Array(base64Str);
    console.log(Array.from(byteData.slice(0, 20)).map(b => b.toString(16).padStart(2, '0')).join(' '));
    console.log(byteData.length);
    const content = byteData.slice(4);


    // 3. XOR 解密（标准酷狗密钥）
    const key = [64, 71, 97, 119, 94, 50, 116, 71, 81, 54, 49, 45, 206, 210, 110, 105];
    const decrypted = new Uint8Array(content.length);
    for (let i = 0; i < content.length; i++) {
        decrypted[i] = content[i] ^ key[i % key.length];
    }


    const hex = Array.from(decrypted.slice(0, 32))
            .map(b => b.toString(16).padStart(2, '0'))
            .join(' ');
        console.log('[DEBUG] 解密后前32字节:', hex);

    let compressedData;
        if (decrypted.length > 6) {
            compressedData = decrypted.slice(2);
        } else {
            compressedData = decrypted;
        }

    //let krcString;
    //try {
    //    krcString = inflateRaw(compressedData);
    //} catch(e) {
    //    console.log("解码错误！");
    //}

    let krcText;
    try {
        krcText = inflateRaw(compressedData);
    } catch (e) {
        console.log("没有inflateRaw：",e.message);
        try {
            krcText = inflate(compressedData);
        } catch (e) {
            console.log("没有inflate：",e.message);
            krcText = "";
        }
    }
    //return compressedData;
    const lyricsString = utf8ArrayToString(krcText);
    return lyricsString;

}

function krcTolist(lyricsText) {
    const lines = lyricsText.split('\n');
    const result = [];

    // 匹配句子头：[起始毫秒,持续毫秒]
    const lineRegex = /\[(\d+),(\d+)\](.*)/;
    // 匹配每个字：<偏移,持续时间,0>字符
    const wordRegex = /<(\d+),(\d+),\d+>([^<]*)/g;

    for (let line of lines) {
        line = line.trim();
        if (!line) continue;

        const match = lineRegex.exec(line);
        if (!match) continue;

        const startTime = parseInt(match[1], 10);
        const content = match[3];

        // 提取所有字
        const words = [];
        let wordMatch;
        // 重置正则（每次重新创建以避免lastIndex问题）
        const regex = /<(\d+),(\d+),\d+>([^<]*)/g;
        while ((wordMatch = regex.exec(content)) !== null) {
            const offset = parseInt(wordMatch[1], 10);
            const duration = parseInt(wordMatch[2], 10);
            const text = wordMatch[3];
            if (text !== '') {
                words.push({ offset, duration, text });
            }
        }

        // 如果没有字，跳过这一句（实际KRC每句都有字）
        if (words.length === 0) continue;

        // 拼接整句文本
        const fullText = words.map(w => w.text).join('');

        result.push({
            time: startTime,
            text: fullText,
            info: words   // 这里用info字段，包含每个字的详细信息
        });
    }

    // 按时间排序（KRC通常已经有序，但保险起见）
    result.sort((a, b) => a.time - b.time);
    return result;
}

function base64DecodeToUint8Array(base64) {
    // 1. 定义 Base64 字符表
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
    const lookup = new Uint8Array(256);
    for (let i = 0; i < chars.length; i++) {
        lookup[chars.charCodeAt(i)] = i;
    }

    // 2. 移除填充字符（等号）
    base64 = base64.replace(/=+$/, '');

    const bytes = [];
    for (let i = 0; i < base64.length; i += 4) {
        // 取出4个字符的索引值（如果超出则忽略）
        const enc1 = lookup[base64.charCodeAt(i)];
        const enc2 = i + 1 < base64.length ? lookup[base64.charCodeAt(i + 1)] : undefined;
        const enc3 = i + 2 < base64.length ? lookup[base64.charCodeAt(i + 2)] : undefined;
        const enc4 = i + 3 < base64.length ? lookup[base64.charCodeAt(i + 3)] : undefined;

        // 组合成字节
        if (enc1 !== undefined && enc2 !== undefined) {
            bytes.push((enc1 << 2) | (enc2 >> 4));
            if (enc3 !== undefined) {
                bytes.push(((enc2 & 15) << 4) | (enc3 >> 2));
                if (enc4 !== undefined) {
                    bytes.push(((enc3 & 3) << 6) | enc4);
                }
            }
        }
    }
    return new Uint8Array(bytes);
}

function utf8ArrayToString(uint8Array) {
    let str = '';
    let i = 0;
    const len = uint8Array.length;
    while (i < len) {
        let byte1 = uint8Array[i++];
        if (byte1 < 0x80) {
            str += String.fromCharCode(byte1);
        } else if (byte1 >= 0xC0 && byte1 < 0xE0) {
            let byte2 = uint8Array[i++];
            str += String.fromCharCode(((byte1 & 0x1F) << 6) | (byte2 & 0x3F));
        } else if (byte1 >= 0xE0 && byte1 < 0xF0) {
            let byte2 = uint8Array[i++];
            let byte3 = uint8Array[i++];
            str += String.fromCharCode(((byte1 & 0x0F) << 12) | ((byte2 & 0x3F) << 6) | (byte3 & 0x3F));
        } else if (byte1 >= 0xF0 && byte1 < 0xF8) {
            let byte2 = uint8Array[i++];
            let byte3 = uint8Array[i++];
            let byte4 = uint8Array[i++];
            let codePoint = ((byte1 & 0x07) << 18) | ((byte2 & 0x3F) << 12) | ((byte3 & 0x3F) << 6) | (byte4 & 0x3F);
            if (codePoint > 0xFFFF) {
                let high = 0xD800 + ((codePoint - 0x10000) >> 10);
                let low = 0xDC00 + ((codePoint - 0x10000) & 0x3FF);
                str += String.fromCharCode(high, low);
            } else {
                str += String.fromCharCode(codePoint);
            }
        } else {
            // 无效字节，替换为 �
            str += '\uFFFD';
        }
    }
    return str;
}

function extractTranslations(krcText) {
    // 匹配 [language:...] 行
    const match = krcText.match(/\[language:([^\]]+)\]/);
    if (!match) return []; // 没有翻译

    const base64 = match[1];
    try {
        const jsonStr = Qt.atob(base64);
        const data = JSON.parse(jsonStr);
        // 提取 content 数组中的第一个元素（通常只有一个）
        const content = data.content[0].type === 1 ? data.content[0] : data.content[1] || data.content[0];
        if (!content) return [];

        // lyricContent 是一个数组，每个元素是 [ "翻译文本" ]
        const lyricContent = content.lyricContent || [];
        // 将每个子数组的第一个元素提取出来，组成翻译列表
        const translations = lyricContent.map(item => {
            const text = (item && item[0]) || '';
            return text.trim() === '' ? '' : text;
        });
        return translations;
    } catch (e) {
        console.error('解析翻译失败:', e);
        return [];
    }
}