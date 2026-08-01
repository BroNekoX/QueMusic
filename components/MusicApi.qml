// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
pragma Singleton
import QtQuick
import QtCore

QtObject {

    //存储预显示的区域
    property ListModel searchSongsResults: ListModel{}//搜索结果
    property ListModel newSongs: ListModel{}//新歌区域
    property ListModel recommendSongs: ListModel{}//[]//推荐歌曲
    property ListModel musicPlaylists: ListModel{}//歌单存储区
    property ListModel playlistSong: ListModel{}//歌单内详细歌曲
    property ListModel allMusicSong: ListModel{}//全部歌曲
    property list<string> allPlaylistMenu: []//全部歌单分类项
    property ListModel hotPlayLists: ListModel{}
    property ListModel getHotlistMenu: ListModel{}//热门歌单分类
    property list<string> playlistmenuInfo: []//歌单分类获取信息
    property ListModel musicToplist: ListModel{}//歌曲排行榜存储
    //property ListModel emptyList: ListModel{}
    property var lyricsData: []
    property var lyricsTranslate: []
    property int songSource: 0
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
    signal download(string path,string name)

    function updateSearchResults(data) {
        searchResults = data.info || data.songs || [];
        // 触发UI更新
        searchResultsChanged();
    }

    function updatePlaylistResults(data) {
        playlistResults = data.info || data.special || [];
        playlistResultsChanged();
    }

    property WorkerScript musicWorker: WorkerScript {
        source: "qrc:/QueMusic/api/musicWorker.mjs"

        onMessage: function(messageObject) {
            console.log("musicWorker消息:", messageObject.type, "source:", messageObject.source);
            var actionType = messageObject.type;
            var data = messageObject.data;
            var src = messageObject.source !== undefined ? messageObject.source : MusicApi.songSource;

            switch(actionType) {
                // 搜索歌曲
                case "searchSongs":
                    // 酷狗: data.info; 网易云: data 直接是数组
                    var items = data.info || data;
                    if (Array.isArray(items)) {
                        MusicApi.searchSongsResults.append(items);
                    }
                    break;

                // 歌单分类
                case "getPlaylistMenu":
                    MusicApi.allPlaylistMenu = data.info || [];
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
                    MusicApi.musicPlaylists.append(data.info);
                    break;

                // 歌单内歌曲
                case "getPlaylistSongs":
                    MusicApi.playlistSong.append(data.info);
                    break;

                // 推荐歌曲
                case "getRecommendSongs":
                    MusicApi.recommendSongs.append(data.info);
                    break;

                // 热门歌单分类
                case "getHotPlaylistMenu":
                    MusicApi.getHotlistMenu.clear();
                    MusicApi.getHotlistMenu.append(data.info);
                    break;

                // 热门歌单
                case "getHotPlaylists":
                    MusicApi.hotPlayLists.append(data.info);
                    break;

                // 新歌
                case "getNewSongs":
                    MusicApi.newSongs.append(data.info);
                    break;

                // 排行榜
                case "getMusicToplist":
                    MusicApi.musicToplist.clear();
                    MusicApi.musicToplist.append(data.info);
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
                    console.log("musicWorker: 未知消息类型:", actionType);
            }
            MusicApi.loadState = false;
        }
    }

    // 搜索 type: 0:歌曲 1. 歌单 2. 专辑 3. 歌词
    function searchSongs(keyword, type = 0, page = 1, pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "searchSongs",
            source: source,
            keyword: keyword,
            type: type,
            page: page,
            pageSize: pageSize
        });
    }

    // 获取歌单的分类项
    function getPlaylistMenu(type = 2, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getPlaylistMenu",
            source: source,
            type: type
        });
    }

    // 获取歌单分类信息及tagid
    function getMenuInfo(id, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getMenuInfo",
            source: source,
            id: id
        });
    }

    // 获取分类中的歌单列表
    function getMusicPlaylists(tagid,page = 1,pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getMusicPlaylists",
            source: source,
            tagid: tagid,
            page: page,
            pageSize: pageSize
        });
    }

    // 获取歌单内的详细歌曲
    function getPlaylistSongs(listid, page = 1, pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getPlaylistSongs",
            source: source,
            listid: listid,
            page: page,
            pageSize: pageSize
        });
    }

    // 获取推荐歌曲
    function getRecommendSongs(page = 1, pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getRecommendSongs",
            source: source,
            page: page,
            pageSize: pageSize
        });
    }

    // 获取热门推荐分类
    function getHotPlaylistMenu(type = 3, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getHotPlaylistMenu",
            source: source,
            type: type
        });
    }

    //获取热门歌单
    function getHotPlaylists(page = 1,pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getHotPlaylists",
            source: source,
            page: page,
            pageSize: pageSize
        });
    }

    // 获取新歌 type：1.华语新歌 2.欧美新歌 3.日韩新歌
    function getNewSongs(type = 1, page = 1, pageSize = 20, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getNewSongs",
            source: source,
            type: type,
            page: page,
            pageSize: pageSize
        });
    }

    // 获取排行榜
    function getMusicToplist(dateTime = 1, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getMusicToplist",
            source: source,
            dateTime: dateTime
        });
    }

    //获取歌曲数据源 type: 0.播放 1.下载 2.收藏
    function getMusicInfo(hash,type = 0, src = -1) {
        MusicApi.loadState = true;
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getMusicInfo",
            source: source,
            id: hash,
            type: type
        });
    }
    // 获取歌词
    function getLyricInfo(hash,duration, src = -1) {
        var source = songSource;
        if(src !== -1) source = src;
        musicWorker.sendMessage({
            action: "getLyricInfo",
            source: source,
            id: hash,
            duration: duration
        });
    }


    // 处理搜索结果的函数
    function handleSearchResults(data) {
        // 解析并显示搜索结果
        console.log("搜索结果:", data);

        // 这里可以将数据传递给UI组件显示
        // 例如：mainContent.updateSearchResults(data);
    }

    function handlePlaylistResults(data) {
        // 处理歌单列表
        console.log("歌单列表:", data);
    }
}