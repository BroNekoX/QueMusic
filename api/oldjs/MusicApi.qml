// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
// MusicApi — 在线音乐 API 门面（过渡层）
// 数据请求已全部迁移到 C++ 端 MusicApiService（api/MusicApiService.h），
// 本文件仅做参数转发 + 结果分发，保持对 QML 旧调用点完全兼容。
pragma Singleton
import QtQuick
import DownloadManager 1.0

QtObject {
    id: root

    //存储预显示的区域
    property ListModel searchSongsResults: ListModel{}//搜索结果
    property ListModel newSongs: ListModel{}//新歌区域
    property ListModel recommendSongs: ListModel{}//[]//推荐歌曲
    property ListModel musicPlaylists: ListModel{}//歌单存储区
    property ListModel playlistSong: ListModel{}//歌单内详细歌曲
    property ListModel allMusicSong: ListModel{}//全部歌曲
    property var allPlaylistMenu: []//全部歌单分类项
    property ListModel hotPlayLists: ListModel{}
    property ListModel getHotlistMenu: ListModel{}//热门歌单分类
    property var playlistmenuInfo: []//歌单分类获取信息
    property ListModel musicToplist: ListModel{}//歌曲排行榜存储
    //property ListModel emptyList: ListModel{}
    property var lyricsData: []
    property var lyricsTranslate: []
    property int songSource: Options.settings.mainMusicSource
    property bool loadState: false
    property var globalid
    property var globaltagid
    property QtObject globalinfo: QtObject {
        property string artist
        property int duration
        property string descript
        property string playcount
    }
    property int nowIndex: 0
    //0.Kugou 1.NetEase 2.QQ 3.?

    onLoadStateChanged: {
        if(loadState) {
            loaded()
        } else {
            finished()
        }
    }

    signal updateModel()
    signal loaded()
    signal finished()
    signal urlplay(string playurl,string title,string artist,string cover,string solve,string hash,int source)

    // C++ 单例总部（main.cpp 注册的 context property）
    property QtObject service: musicApiService

    // 默认源与 C++ 总部保持同步
    onSongSourceChanged: service.songSource = MusicApi.songSource

    // ---- 字段归一化：统一字段兜底 + 旧字段名别名（兼容旧 QML 页面）----
    function normalizeItem(it) {
        var item = it || {};
        // 统一字段（旧字段 → 新字段，防 undefined）
        item.title     = item.title !== undefined ? item.title : (item.specialname || item.songname || "");
        item.artist    = item.artist !== undefined ? item.artist : (item.username || item.singername || item.author_name || "");
        item.cover     = item.cover !== undefined ? item.cover : (item.imgurl || item.album_img || "");
        item.hash      = item.hash !== undefined ? item.hash : String(item.specialid || item.albumid || item.id || "");
        item.duration  = item.duration !== undefined ? item.duration : 0;
        item.album     = item.album !== undefined ? item.album : (item.intro || item.album_name || "");
        item.playcount = item.playcount !== undefined ? item.playcount : 0;
        item.paytype   = item.paytype !== undefined ? item.paytype : 0;
        if (item.hashhq === undefined) item.hashhq = item.hash;
        if (item.hashsq === undefined) item.hashsq = item.hash;
        // 旧字段名别名（部分 QML 页面直接读酷狗旧字段）
        if (item.specialname === undefined) item.specialname = item.title;
        if (item.username   === undefined) item.username   = item.artist;
        if (item.imgurl     === undefined) item.imgurl     = item.cover;
        if (item.songname   === undefined) item.songname   = item.title;
        if (item.singername === undefined) item.singername = item.artist;
        if (item.intro      === undefined) item.intro      = item.album;
        if (item.album_name === undefined) item.album_name = item.album;
        if (item.specialid  === undefined) item.specialid  = item.hash;
        if (item.albumid    === undefined) item.albumid    = item.hash;
        return item;
    }
    function normalizeList(list) {
        var out = [];
        if (!list) return out;
        for (var i = 0; i < list.length; i++)
            out.push(MusicApi.normalizeItem(list[i]));
        return out;
    }

    // 接收 C++ 总部统一结果（协议与旧 musicWorker 完全一致）
    property Connections connectService: Connections {
        target: root.service
        function onResultReady(action, data, src) {
            console.log("MusicApiService消息:", action, "source:", src);
            switch(action) {
                // 搜索歌曲
                case "searchSongs":
                    // 酷狗: data.info; 网易云: data 直接是数组
                    var items = MusicApi.normalizeList(data.info || data);
                    if (items.length) {
                        MusicApi.searchSongsResults.append(items);
                    }
                    break;

                // 歌单分类（兼容 modelData 字符串显示 + .id 访问）
                case "getPlaylistMenu":
                    var menuRaw = data.info || [];
                    var menu = [];
                    for (var i = 0; i < menuRaw.length; i++) {
                        var it = MusicApi.normalizeItem(menuRaw[i]);
                        var s = new String(it.title || it.category || "");
                        s.id = it.id;
                        s.category = it.category;
                        s.title = it.title;
                        menu.push(s);
                    }
                    MusicApi.allPlaylistMenu = menu;
                    break;

                // 分类信息
                case "getMenuInfo":
                    MusicApi.playlistmenuInfo = data;
                    var tid = data.special_tag_id || data.tag_id;
                    if (tid) {
                        // 收到分类信息后自动拉取该分类下的歌单
                        MusicApi.getMusicPlaylists(tid, 1, 24, src);
                    }
                    break;

                // 歌单列表
                case "getMusicPlaylists":
                    MusicApi.musicPlaylists.append(MusicApi.normalizeList(data.info));
                    break;

                // 歌单内歌曲
                case "getPlaylistSongs":
                    MusicApi.playlistSong.append(MusicApi.normalizeList(data.info));
                    break;

                // 推荐歌曲
                case "getRecommendSongs":
                    MusicApi.recommendSongs.append(MusicApi.normalizeList(data.info));
                    break;

                // 热门歌单分类
                case "getHotPlaylistMenu":
                    MusicApi.getHotlistMenu.clear();
                    MusicApi.getHotlistMenu.append(MusicApi.normalizeList(data.info));
                    break;

                // 热门歌单
                case "getHotPlaylists":
                    MusicApi.hotPlayLists.append(MusicApi.normalizeList(data.info));
                    break;

                // 新歌
                case "getNewSongs":
                    MusicApi.newSongs.append(MusicApi.normalizeList(data.info));
                    break;

                // 排行榜
                case "getMusicToplist":
                    MusicApi.musicToplist.clear();
                    MusicApi.musicToplist.append(MusicApi.normalizeList(data.info));
                    break;

                // 歌曲播放/下载信息
                case "getMusicInfo":
                    if(data.type === 0) {
                        var cover = data.album_img;
                        // 酷狗可能有 {size} 占位符
                        if (cover && cover.indexOf("{size}") !== -1) {
                            cover = cover.replace("{size}", "512");
                        }
                        var solve = data.album_img;
                        if (solve && solve.indexOf("{size}") !== -1) {
                            solve = solve.replace("{size}", "128");
                        }
                        MusicApi.urlplay(data.backup_url, data.songName, data.author_name,
                                        cover, solve, data.hash, src);
                        var time = data.timeLength * (data.timeLength < 1000 ? 1000 : 1);
                        // 直接请求歌词
                        MusicApi.getLyricInfo(data.hash, time, src);
                    } else if(data.type === 1) {
                        var downloadUrl = data.url;
                        var fileName = data.fileName;
                        MusicApi.download(downloadUrl, fileName);
                    }
                    break;

                // 歌词
                case "getLyricInfo":
                    MusicApi.lyricsData = data.info;
                    MusicApi.lyricsTranslate = data.translate;
                    console.log("歌词获取完成, source:", src);
                    break;

                default:
                    console.log("MusicApiService: 未知消息类型:", action);
            }
            MusicApi.loadState = false;
        }
    }

    // 统一转发到 C++ 总部（src < 0 时用当前默认源）
    function _src(src) { return src === undefined || src < 0 ? MusicApi.songSource : src }

    // 搜索 type: 0:歌曲 1. 歌单 2. 专辑 3. 歌词
    function searchSongs(keyword, type = 0, page = 1, pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        service.searchSongs(keyword, type, page, pageSize, _src(src));
    }

    // 获取歌单的分类项
    function getPlaylistMenu(type = 2, src = -1) {
        MusicApi.loadState = true;
        service.getPlaylistMenu(type, _src(src));
    }

    // 获取歌单分类信息及tagid
    function getMenuInfo(id, src = -1) {
        MusicApi.loadState = true;
        service.getMenuInfo(id, _src(src));
    }

    // 获取分类中的歌单列表
    function getMusicPlaylists(tagid,page = 1,pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        service.getMusicPlaylists(tagid, page, pageSize, _src(src));
    }

    // 获取歌单内的详细歌曲
    function getPlaylistSongs(listid, page = 1, pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        service.getPlaylistSongs(listid, page, pageSize, _src(src));
    }

    // 获取推荐歌曲
    function getRecommendSongs(page = 1, pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        service.getRecommendSongs(page, pageSize, _src(src));
    }

    // 获取热门推荐分类
    function getHotPlaylistMenu(type = 3, src = -1) {
        MusicApi.loadState = true;
        service.getHotPlaylistMenu(type, _src(src));
    }

    //获取热门歌单
    function getHotPlaylists(page = 1,pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        service.getHotPlaylists(page, pageSize, _src(src));
    }

    // 获取新歌 type：1.华语新歌 2.欧美新歌 3.日韩新歌
    function getNewSongs(type = 1, page = 1, pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        service.getNewSongs(type, page, pageSize, _src(src));
    }

    // 获取排行榜
    function getMusicToplist(dateTime = 1, src = -1) {
        MusicApi.loadState = true;
        service.getMusicToplist(dateTime, _src(src));
    }

    //获取歌曲数据源 type: 0.播放 1.下载 2.收藏
    function getMusicInfo(hash,type = 0, src = -1) {
        MusicApi.loadState = true;
        service.getMusicInfo(hash, type, _src(src));
    }
    // 获取歌词
    function getLyricInfo(hash,duration, src = -1) {
        service.getLyricInfo(hash, duration, _src(src));
    }

    function download(path, name) {
        downloader.addDownload(path, name);
        Style.warned("已添加下载: " + name, 1);
    }

    property DownloadManager downloader: DownloadManager {
        downloadPath: Options.settings.downloadFolder
    }
}
