#ifndef MODULE_H
#define MODULE_H

#include "qcloudmusicapiglobal.h"

#include <QVariantMap>

class NeteaseCloudMusicApiPrivate;
class QCLOUDMUSICAPI_EXPORT NeteaseCloudMusicApi : public QObject {
    Q_OBJECT
public:
    explicit NeteaseCloudMusicApi(QObject* parent = nullptr);

private:
    QVariantMap request(QString uri, QVariantMap data, QVariantMap options);
    NeteaseCloudMusicApiPrivate* d_ptr = nullptr;

public:

    // api
    Q_INVOKABLE QVariantMap api(QVariantMap);

    // 初始化名字
    Q_INVOKABLE QVariantMap activate_init_profile(QVariantMap);

    // 私人 DJ

    // 实际请求参数如下, 部分内容省略, 敏感信息已进行混淆
    // 可按需修改此 API 的代码
    /* {"extInfo":"{\"lastRequestTimestamp\":1692358373509,\"lbsInfoList\":[{\"lat\":40.23076381,\"lon\":129.07545186,\"time\":1692358543},{\"lat\":40.23076381,\"lon\":129.07545186,\"time\":1692055283}],\"listenedTs\":false,\"noAidjToAidj\":true}","header":"{}"} */
    Q_INVOKABLE QVariantMap aidj_content_rcmd(QVariantMap);

    // 专辑动态信息
    Q_INVOKABLE QVariantMap album_detail_dynamic(QVariantMap);

    // 数字专辑详情
    Q_INVOKABLE QVariantMap album_detail(QVariantMap);

    // 数字专辑-语种风格馆
    Q_INVOKABLE QVariantMap album_list_style(QVariantMap);

    // 数字专辑-新碟上架
    Q_INVOKABLE QVariantMap album_list(QVariantMap);

    // 全部新碟
    Q_INVOKABLE QVariantMap album_new(QVariantMap);

    // 最新专辑
    Q_INVOKABLE QVariantMap album_newest(QVariantMap);

    // 获取专辑歌曲的音质
    Q_INVOKABLE QVariantMap album_privilege(QVariantMap);

    // 数字专辑&数字单曲-榜单
    Q_INVOKABLE QVariantMap album_songsaleboard(QVariantMap);

    // 收藏/取消收藏专辑
    Q_INVOKABLE QVariantMap album_sub(QVariantMap);

    // 已收藏专辑列表
    Q_INVOKABLE QVariantMap album_sublist(QVariantMap);

    // 专辑内容
    Q_INVOKABLE QVariantMap album(QVariantMap);

    // 歌手专辑列表
    Q_INVOKABLE QVariantMap artist_album(QVariantMap);

    // 歌手介绍
    Q_INVOKABLE QVariantMap artist_desc(QVariantMap);

    // 歌手动态信息
    Q_INVOKABLE QVariantMap artist_detail_dynamic(QVariantMap);

    // 歌手详情
    Q_INVOKABLE QVariantMap artist_detail(QVariantMap);

    // 歌手粉丝
    Q_INVOKABLE QVariantMap artist_fans(QVariantMap);

    // 歌手粉丝数量
    Q_INVOKABLE QVariantMap artist_follow_count(QVariantMap);

    // 歌手分类

    /*
    type 取值
    1:男歌手
    2:女歌手
    3:乐队

    area 取值
    -1:全部
    7华语
    96欧美
    8:日本
    16韩国
    0:其他

    initial 取值 a-z/A-Z
*/
    Q_INVOKABLE QVariantMap artist_list(QVariantMap);

    // 歌手相关MV
    Q_INVOKABLE QVariantMap artist_mv(QVariantMap);

    // 关注歌手新 MV
    Q_INVOKABLE QVariantMap artist_new_mv(QVariantMap);

    // 关注歌手新歌
    Q_INVOKABLE QVariantMap artist_new_song(QVariantMap);

    // 歌手全部歌曲
    Q_INVOKABLE QVariantMap artist_songs(QVariantMap);

    // 收藏与取消收藏歌手
    Q_INVOKABLE QVariantMap artist_sub(QVariantMap);

    // 关注歌手列表
    Q_INVOKABLE QVariantMap artist_sublist(QVariantMap);

    // 歌手热门 50 首歌曲
    Q_INVOKABLE QVariantMap artist_top_song(QVariantMap);

    // 歌手相关视频
    Q_INVOKABLE QVariantMap artist_video(QVariantMap);

    // 歌手单曲
    Q_INVOKABLE QVariantMap artists(QVariantMap);

    // 听歌识曲
    Q_INVOKABLE QVariantMap audio_match(QVariantMap);

    // 更新头像
    Q_INVOKABLE QVariantMap avatar_upload(QVariantMap);

    // 批量请求接口
    Q_INVOKABLE QVariantMap batch(QVariantMap);

    // 首页轮播图
    Q_INVOKABLE QVariantMap banner(QVariantMap);

    // 广播电台 - 分类/地区信息
    Q_INVOKABLE QVariantMap broadcast_category_region_get(QVariantMap);

    // 广播电台 - 我的收藏
    Q_INVOKABLE QVariantMap broadcast_channel_collect_list(QVariantMap);

    // 广播电台 - 电台信息
    Q_INVOKABLE QVariantMap broadcast_channel_currentinfo(QVariantMap);

    // 广播电台 - 全部电台
    Q_INVOKABLE QVariantMap broadcast_channel_list(QVariantMap);

    // 音乐日历
    Q_INVOKABLE QVariantMap calendar(QVariantMap);

    // 发送验证码
    Q_INVOKABLE QVariantMap captcha_sent(QVariantMap);

    // 校验验证码
    Q_INVOKABLE QVariantMap captcha_verify(QVariantMap);

    // 检测手机号码是否已注册
    Q_INVOKABLE QVariantMap cellphone_existence_check(QVariantMap);

    // 获取指定维度音乐排行榜详情
    Q_INVOKABLE QVariantMap chart_detail(QVariantMap query);

    // 获取指定维度音乐排行榜列表
    Q_INVOKABLE QVariantMap chart_song_detail(QVariantMap query);

    // 歌曲可用性
    Q_INVOKABLE QVariantMap check_music(QVariantMap);

    // 云盘歌曲信息匹配纠正
    Q_INVOKABLE QVariantMap cloud_match(QVariantMap);

    // 云盘上传
    Q_INVOKABLE QVariantMap cloud(QVariantMap);

    // 搜索
    Q_INVOKABLE QVariantMap cloudsearch(QVariantMap);

    // 专辑评论
    Q_INVOKABLE QVariantMap comment_album(QVariantMap);

    // 电台评论
    Q_INVOKABLE QVariantMap comment_dj(QVariantMap);

    // 获取动态评论
    Q_INVOKABLE QVariantMap comment_event(QVariantMap);

    // 楼层评论
    Q_INVOKABLE QVariantMap comment_floor(QVariantMap);

    // 热门评论
    Q_INVOKABLE QVariantMap comment_hot(QVariantMap);

    // 评论抱一抱列表
    Q_INVOKABLE QVariantMap comment_hug_list(QVariantMap);

    // 点赞与取消点赞评论
    Q_INVOKABLE QVariantMap comment_like(QVariantMap);

    // 歌曲评论
    Q_INVOKABLE QVariantMap comment_music(QVariantMap);

    // MV评论
    Q_INVOKABLE QVariantMap comment_mv(QVariantMap);

    // 新版评论接口
    Q_INVOKABLE QVariantMap comment_new(QVariantMap);

    // 歌单评论
    Q_INVOKABLE QVariantMap comment_playlist(QVariantMap);

    // 视频评论
    Q_INVOKABLE QVariantMap comment_video(QVariantMap);

    // 发送与删除评论
    Q_INVOKABLE QVariantMap comment(QVariantMap);

    // 国家编码列表
    Q_INVOKABLE QVariantMap countries_code_list(QVariantMap);

    // 获取达人用户信息
    Q_INVOKABLE QVariantMap creator_authinfo_get(QVariantMap);

    // 签到

    /*
    0为安卓端签到 3点经验, 1为网页签到,2点经验
    签到成功 {'android': {'point': 3, 'code': 200}, 'web': {'point': 2, 'code': 200}}
    重复签到 {'android': {'code': -2, 'msg': '重复签到'}, 'web': {'code': -2, 'msg': '重复签到'}}
    未登录 {'android': {'code': 301}, 'web': {'code': 301}}
    */
    Q_INVOKABLE QVariantMap daily_signin(QVariantMap);

    // 数字专辑详情
    Q_INVOKABLE QVariantMap digitalAlbum_detail(QVariantMap);

    // 购买数字专辑
    Q_INVOKABLE QVariantMap digitalAlbum_ordering(QVariantMap);

    // 我的数字专辑
    Q_INVOKABLE QVariantMap digitalAlbum_purchased(QVariantMap);

    // 数字专辑销量
    Q_INVOKABLE QVariantMap digitalAlbum_sales(QVariantMap);

    // 电台banner
    Q_INVOKABLE QVariantMap dj_banner(QVariantMap);

    // 电台非热门类型
    Q_INVOKABLE QVariantMap dj_category_excludehot(QVariantMap);

    // 电台推荐类型
    Q_INVOKABLE QVariantMap dj_category_recommend(QVariantMap);

    // 电台分类列表
    Q_INVOKABLE QVariantMap dj_catelist(QVariantMap);

    // 电台详情
    Q_INVOKABLE QVariantMap dj_detail(QVariantMap);

    // DIFM电台 - 分类
    Q_INVOKABLE QVariantMap dj_difm_all_style_channel(QVariantMap);

    // DIFM电台 - 收藏列表
    Q_INVOKABLE QVariantMap dj_difm_subscribe_channels_get(QVariantMap);

    // DIFM电台 - 收藏频道
    Q_INVOKABLE QVariantMap dj_difm_channel_subscribe(QVariantMap);

    // DIFM电台 - 取消收藏频道
    Q_INVOKABLE QVariantMap dj_difm_channel_unsubscribe(QVariantMap);

    // DIFM电台 - 播放列表
    Q_INVOKABLE QVariantMap dj_difm_playing_tracks_list(QVariantMap);

    // 热门电台
    Q_INVOKABLE QVariantMap dj_hot(QVariantMap);

    // 付费电台
    Q_INVOKABLE QVariantMap dj_paygift(QVariantMap);

    // 电台个性推荐
    Q_INVOKABLE QVariantMap dj_personalize_recommend(QVariantMap);

    // 电台节目详情
    Q_INVOKABLE QVariantMap dj_program_detail(QVariantMap);

    // 电台24小时节目榜
    Q_INVOKABLE QVariantMap dj_program_toplist_hours(QVariantMap);

    // 电台节目榜
    Q_INVOKABLE QVariantMap dj_program_toplist(QVariantMap);

    // 电台节目列表
    Q_INVOKABLE QVariantMap dj_program(QVariantMap);

    // 类别热门电台
    Q_INVOKABLE QVariantMap dj_radio_hot(QVariantMap);

    // 精选电台分类

    /*
    有声书 10001
    知识技能 453050
    商业财经 453051
    人文历史 11
    外语世界 13
    亲子宝贝 14
    创作|翻唱 2001
    音乐故事 2
    3D|电子 10002
    相声曲艺 8
    情感调频 3
    美文读物 6
    脱口秀 5
    广播剧 7
    二次元 3001
    明星做主播 1
    娱乐|影视 4
    科技科学 453052
    校园|教育 4001
    旅途|城市 12
    */
    Q_INVOKABLE QVariantMap dj_recommend_type(QVariantMap);

    // 精选电台
    Q_INVOKABLE QVariantMap dj_recommend(QVariantMap);

    // 订阅与取消电台
    Q_INVOKABLE QVariantMap dj_sub(QVariantMap);

    // 订阅电台列表
    Q_INVOKABLE QVariantMap dj_sublist(QVariantMap);

    // 电台详情
    Q_INVOKABLE QVariantMap dj_subscriber(QVariantMap);

    // 电台今日优选
    Q_INVOKABLE QVariantMap dj_today_perfered(QVariantMap);

    // 电台24小时主播榜
    Q_INVOKABLE QVariantMap dj_toplist_hours(QVariantMap);

    // 电台新人榜
    Q_INVOKABLE QVariantMap dj_toplist_newcomer(QVariantMap);

    // 付费精品
    Q_INVOKABLE QVariantMap dj_toplist_pay(QVariantMap);

    // 电台最热主播榜
    Q_INVOKABLE QVariantMap dj_toplist_popular(QVariantMap);

    // 新晋电台榜/热门电台榜
    Q_INVOKABLE QVariantMap dj_toplist(QVariantMap);

    // 电台排行榜获取
    Q_INVOKABLE QVariantMap djRadio_top(QVariantMap);

    // eapi 解析接口
    Q_INVOKABLE QVariantMap eapi_decrypt(QVariantMap);

    // 删除动态
    Q_INVOKABLE QVariantMap event_del(QVariantMap);

    // 转发动态
    Q_INVOKABLE QVariantMap event_forward(QVariantMap);

    // 动态
    Q_INVOKABLE QVariantMap event(QVariantMap);

    // 粉丝年龄比例
    Q_INVOKABLE QVariantMap fanscenter_basicinfo_age_get(QVariantMap);

    // 粉丝性别比例
    Q_INVOKABLE QVariantMap fanscenter_basicinfo_gender_get(QVariantMap);

    // 粉丝省份比例
    Q_INVOKABLE QVariantMap fanscenter_basicinfo_province_get(QVariantMap);

    // 粉丝数量
    Q_INVOKABLE QVariantMap fanscenter_overview_get(QVariantMap);

    // 粉丝来源
    Q_INVOKABLE QVariantMap fanscenter_trend_list(QVariantMap);

    // 垃圾桶
    Q_INVOKABLE QVariantMap fm_trash(QVariantMap);

    // 关注与取消关注用户
    Q_INVOKABLE QVariantMap follow(QVariantMap);

    // 根据nickname获取userid
    Q_INVOKABLE QVariantMap get_userids(QVariantMap);

    // 历史每日推荐歌曲详情
    Q_INVOKABLE QVariantMap history_recommend_songs_detail(QVariantMap);

    // 历史每日推荐歌曲
    Q_INVOKABLE QVariantMap history_recommend_songs(QVariantMap);

    // 首页-发现 block page
    // 这个接口为移动端接口，首页-发现页，数据结构可以参考 https://github.com/hcanyz/flutter-netease-music-api/blob/master/lib/src/api/uncategorized/bean.dart#L259 HomeBlockPageWrap
    // query.refresh 是否刷新数据
    Q_INVOKABLE QVariantMap homepage_block_page(QVariantMap);

    // 首页-发现 dragon ball
    // 这个接口为移动端接口，首页-发现页（每日推荐、歌单、排行榜 那些入口）
    // 数据结构可以参考 https://github.com/hcanyz/flutter-netease-music-api/blob/master/lib/src/api/uncategorized/bean.dart#L290 HomeDragonBallWrap
    // !需要登录或者游客登录，非登录返回 []
    Q_INVOKABLE QVariantMap homepage_dragon_ball(QVariantMap);

    //热门话题
    Q_INVOKABLE QVariantMap hot_topic(QVariantMap);

    //抱一抱评论
    Q_INVOKABLE QVariantMap hug_comment(QVariantMap);

    // 多级行政区划数据获取接口
    Q_INVOKABLE QVariantMap lbs_city_code(QVariantMap query);

    // 红心与取消红心歌曲
    Q_INVOKABLE QVariantMap like(QVariantMap);

    // 喜欢音乐列表
    Q_INVOKABLE QVariantMap likelist(QVariantMap);

    // 听歌足迹 - 本周/本月收听时长
    Q_INVOKABLE QVariantMap listen_data_realtime_report(QVariantMap);

    // 听歌足迹 - 周/月/年收听报告
    Q_INVOKABLE QVariantMap listen_data_report(QVariantMap);

    // 听歌足迹 - 今日收听
    Q_INVOKABLE QVariantMap listen_data_today_song(QVariantMap);

    // 听歌足迹 - 总收听时长
    Q_INVOKABLE QVariantMap listen_data_total(QVariantMap);

    // 听歌足迹 - 年度听歌足迹
    Q_INVOKABLE QVariantMap listen_data_year_report(QVariantMap);

    // 一起听 接受邀请
    Q_INVOKABLE QVariantMap listentogether_accept(QVariantMap);

    // 一起听 结束房间
    Q_INVOKABLE QVariantMap listentogether_end(QVariantMap);

    // 一起听 发送心跳
    Q_INVOKABLE QVariantMap listentogether_heatbeat(QVariantMap);

    // 一起听 发送播放状态
    Q_INVOKABLE QVariantMap listentogether_play_command(QVariantMap);

    // 一起听 房间情况
    Q_INVOKABLE QVariantMap listentogether_room_check(QVariantMap);

    // 一起听创建房间
    Q_INVOKABLE QVariantMap listentogether_room_create(QVariantMap);

    // 一起听状态
    Q_INVOKABLE QVariantMap listentogether_status(QVariantMap);

    // 一起听 更新播放列表
    Q_INVOKABLE QVariantMap listentogether_sync_list_command(QVariantMap);

    // 一起听 当前列表获取
    Q_INVOKABLE QVariantMap listentogether_sync_playlist_get(QVariantMap);

    // 手机登录
    Q_INVOKABLE QVariantMap login_cellphone(QVariantMap);

    // 二维码检测扫码状态接口
    Q_INVOKABLE QVariantMap login_qr_check(QVariantMap);

    // 二维码生成接口
    Q_INVOKABLE QVariantMap login_qr_create(QVariantMap);

    // 二维码 key 生成接口
    Q_INVOKABLE QVariantMap login_qr_key(QVariantMap);

    // 登录刷新
    Q_INVOKABLE QVariantMap login_refresh(QVariantMap);

    // 登录状态
    Q_INVOKABLE QVariantMap login_status(QVariantMap);

    // 邮箱登录
    Q_INVOKABLE QVariantMap login(QVariantMap);

    // 退出登录
    Q_INVOKABLE QVariantMap logout(QVariantMap);

    // 新版歌词 - 包含逐字歌词
    Q_INVOKABLE QVariantMap lyric_new(QVariantMap);

    // 歌词
    Q_INVOKABLE QVariantMap lyric(QVariantMap);

    // 歌曲相关视频
    Q_INVOKABLE QVariantMap mlog_music_rcmd(QVariantMap);

    // 将mlog id转为video id
    Q_INVOKABLE QVariantMap mlog_to_video(QVariantMap);

    // mlog链接
    Q_INVOKABLE QVariantMap mlog_url(QVariantMap);

    // 评论
    Q_INVOKABLE QVariantMap msg_comments(QVariantMap);

    // @我
    Q_INVOKABLE QVariantMap msg_forwards(QVariantMap);

    // 通知
    Q_INVOKABLE QVariantMap msg_notices(QVariantMap);

    // 私信内容
    Q_INVOKABLE QVariantMap msg_private_history(QVariantMap);

    // 私信
    Q_INVOKABLE QVariantMap msg_private(QVariantMap);

    // 最近联系
    Q_INVOKABLE QVariantMap msg_recentcontact(QVariantMap);

    // 回忆坐标
    Q_INVOKABLE QVariantMap music_first_listen_info(QVariantMap);

    // 全部MV
    Q_INVOKABLE QVariantMap mv_all(QVariantMap);

    // MV 点赞转发评论数数据
    Q_INVOKABLE QVariantMap mv_detail_info(QVariantMap);

    // MV详情
    Q_INVOKABLE QVariantMap mv_detail(QVariantMap);

    // 网易出品
    Q_INVOKABLE QVariantMap mv_exclusive_rcmd(QVariantMap);

    // 最新MV
    Q_INVOKABLE QVariantMap mv_first(QVariantMap);

    // 收藏与取消收藏MV
    Q_INVOKABLE QVariantMap mv_sub(QVariantMap);

    // 已收藏MV列表
    Q_INVOKABLE QVariantMap mv_sublist(QVariantMap);

    // MV链接
    Q_INVOKABLE QVariantMap mv_url(QVariantMap);

    // 重复昵称检测
    Q_INVOKABLE QVariantMap nickname_check(QVariantMap);

    // 私人FM - 模式选择

    // aidj, DEFAULT, FAMILIAR, EXPLORE, SCENE_RCMD ( EXERCISE, FOCUS, NIGHT_EMO  )
    // 来不及解释这几个了
    Q_INVOKABLE QVariantMap personal_fm_mode(QVariantMap);

    // 私人FM
    Q_INVOKABLE QVariantMap personal_fm(QVariantMap);

    // 推荐电台
    Q_INVOKABLE QVariantMap personalized_djprogram(QVariantMap);

    // 推荐MV
    Q_INVOKABLE QVariantMap personalized_mv(QVariantMap);

    // 推荐新歌
    Q_INVOKABLE QVariantMap personalized_newsong(QVariantMap);

    // 独家放送列表
    Q_INVOKABLE QVariantMap personalized_privatecontent_list(QVariantMap);

    // 独家放送
    Q_INVOKABLE QVariantMap personalized_privatecontent(QVariantMap);

    // 推荐歌单
    Q_INVOKABLE QVariantMap personalized(QVariantMap);

    // 私信和通知接口
    Q_INVOKABLE QVariantMap pl_count(QVariantMap);

    // 全部歌单分类
    Q_INVOKABLE QVariantMap playlist_catlist(QVariantMap);

    // 歌单封面上传
    Q_INVOKABLE QVariantMap playlist_cover_update(QVariantMap);

    // 创建歌单
    Q_INVOKABLE QVariantMap playlist_create(QVariantMap);

    // 删除歌单
    Q_INVOKABLE QVariantMap playlist_delete(QVariantMap);

    // 更新歌单描述
    Q_INVOKABLE QVariantMap playlist_desc_update(QVariantMap);

    // 歌单动态信息
    Q_INVOKABLE QVariantMap playlist_detail_dynamic(QVariantMap);

    // 相关歌单推荐
    Q_INVOKABLE QVariantMap playlist_detail_rcmd_get(QVariantMap);

    // 歌单详情
    Q_INVOKABLE QVariantMap playlist_detail(QVariantMap);

    // 精品歌单 tags
    Q_INVOKABLE QVariantMap playlist_highquality_tags(QVariantMap);

    // 热门歌单分类
    Q_INVOKABLE QVariantMap playlist_hot(QVariantMap);

    // 歌单导入 - 元数据/文字/链接导入
    Q_INVOKABLE QVariantMap playlist_import_name_task_create(QVariantMap);

    // 歌单导入 - 任务状态
    Q_INVOKABLE QVariantMap playlist_import_task_status(QVariantMap);

    // 获取点赞过的视频
    Q_INVOKABLE QVariantMap playlist_mylike(QVariantMap);

    // 更新歌单名
    Q_INVOKABLE QVariantMap playlist_name_update(QVariantMap);

    // 编辑歌单顺序
    Q_INVOKABLE QVariantMap playlist_order_update(QVariantMap);

    // 公开隐私歌单
    Q_INVOKABLE QVariantMap playlist_privacy(QVariantMap);

    // 收藏与取消收藏歌单
    Q_INVOKABLE QVariantMap playlist_subscribe(QVariantMap);

    // 歌单收藏者
    Q_INVOKABLE QVariantMap playlist_subscribers(QVariantMap);

    // 更新歌单标签
    Q_INVOKABLE QVariantMap playlist_tags_update(QVariantMap);

    // 收藏视频到视频歌单
    Q_INVOKABLE QVariantMap playlist_track_add(QVariantMap);

    // 通过传过来的歌单id拿到所有歌曲数据
    // 支持传递参数limit来限制获取歌曲的数据数量 例如: /playlist/track/all?id=7044354223&limit=10
    Q_INVOKABLE QVariantMap playlist_track_all(QVariantMap);

    // 收藏单曲到歌单 从歌单删除歌曲
    Q_INVOKABLE QVariantMap playlist_track_delete(QVariantMap);

    // 收藏单曲到歌单 从歌单删除歌曲
    Q_INVOKABLE QVariantMap playlist_tracks(QVariantMap);

    // 歌单打卡
    Q_INVOKABLE QVariantMap playlist_update_playcount(QVariantMap);

    // 编辑歌单
    Q_INVOKABLE QVariantMap playlist_update(QVariantMap);

    // 最近播放的视频
    Q_INVOKABLE QVariantMap playlist_video_recent(QVariantMap);

    // 智能播放
    Q_INVOKABLE QVariantMap playmode_intelligence_list(QVariantMap);

    // 云随机播放
    Q_INVOKABLE QVariantMap playmode_song_vector(QVariantMap);

    // 推荐节目
    Q_INVOKABLE QVariantMap program_recommend(QVariantMap);

    // 更换手机
    Q_INVOKABLE QVariantMap rebind(QVariantMap);

    // 最近听歌列表
    Q_INVOKABLE QVariantMap recent_listen_list(QVariantMap);

    // 每日推荐歌单
    Q_INVOKABLE QVariantMap recommend_resource(QVariantMap);

    // 每日推荐歌曲-不感兴趣
    Q_INVOKABLE QVariantMap recommend_songs_dislike(QVariantMap);

    // 每日推荐歌曲
    Q_INVOKABLE QVariantMap recommend_songs(QVariantMap);

    // 最近播放-专辑
    Q_INVOKABLE QVariantMap record_recent_album(QVariantMap);

    // 最近播放-播客
    Q_INVOKABLE QVariantMap record_recent_dj(QVariantMap);

    // 最近播放-歌单
    Q_INVOKABLE QVariantMap record_recent_playlist(QVariantMap);

    // 最近播放-歌曲
    Q_INVOKABLE QVariantMap record_recent_song(QVariantMap);

    // 最近播放-视频
    Q_INVOKABLE QVariantMap record_recent_video(QVariantMap);

    // 最近播放-声音
    Q_INVOKABLE QVariantMap record_recent_voice(QVariantMap);

    // 游客登录
    Q_INVOKABLE QVariantMap register_anonimous(QVariantMap);

    // 注册账号
    Q_INVOKABLE QVariantMap register_cellphone(QVariantMap);

    // 相关视频
    Q_INVOKABLE QVariantMap related_allvideo(QVariantMap);

    // 相关歌单
    Q_INVOKABLE QVariantMap related_playlist(QVariantMap);

    // 点赞与取消点赞资源
    Q_INVOKABLE QVariantMap resource_like(QVariantMap);

    // 听歌打卡
    Q_INVOKABLE QVariantMap scrobble(QVariantMap);

    // 默认搜索关键词
    Q_INVOKABLE QVariantMap search_default(QVariantMap);

    // 热搜列表
    Q_INVOKABLE QVariantMap search_hot_detail(QVariantMap);

    // 热门搜索
    Q_INVOKABLE QVariantMap search_hot(QVariantMap);

    // 本地歌曲匹配音乐信息
    Q_INVOKABLE QVariantMap search_match(QVariantMap);

    // 多类型搜索
    Q_INVOKABLE QVariantMap search_multimatch(QVariantMap);

    // 搜索建议
    Q_INVOKABLE QVariantMap search_suggest(QVariantMap);

    // 搜索
    Q_INVOKABLE QVariantMap search(QVariantMap);

    // 私信专辑
    Q_INVOKABLE QVariantMap send_album(QVariantMap);

    // 私信歌单
    Q_INVOKABLE QVariantMap send_playlist(QVariantMap);

    // 私信歌曲
    Q_INVOKABLE QVariantMap send_song(QVariantMap);

    // 私信
    Q_INVOKABLE QVariantMap send_text(QVariantMap);

    // 设置
    Q_INVOKABLE QVariantMap setting(QVariantMap);

    // 分享歌曲到动态
    Q_INVOKABLE QVariantMap share_resource(QVariantMap);

    // 乐谱列表
    Q_INVOKABLE QVariantMap sheet_list(QVariantMap);

    // 乐谱预览
    Q_INVOKABLE QVariantMap sheet_preview(QVariantMap);

    // 乐签信息
    Q_INVOKABLE QVariantMap sign_happy_info(QVariantMap);

    // 签到进度
    Q_INVOKABLE QVariantMap signin_progress(QVariantMap);

    // 相似歌手
    Q_INVOKABLE QVariantMap simi_artist(QVariantMap);

    // 相似歌手
    Q_INVOKABLE QVariantMap simi_mv(QVariantMap);

    // 相似歌单
    Q_INVOKABLE QVariantMap simi_playlist(QVariantMap);

    // 相似歌曲
    Q_INVOKABLE QVariantMap simi_song(QVariantMap);

    // 相似用户
    Q_INVOKABLE QVariantMap simi_user(QVariantMap);

    // 副歌时间
    Q_INVOKABLE QVariantMap song_chorus(QVariantMap);

    // 歌曲详情
    Q_INVOKABLE QVariantMap song_detail(QVariantMap);

    // 会员下载歌曲记录
    Q_INVOKABLE QVariantMap song_downlist(QVariantMap);

    // 获取客户端歌曲下载链接 - v1
    // 此版本不再采用 br 作为音质区分的标准
    // 而是采用 standard, exhigh, lossless, hires, jyeffect(高清环绕声), sky(沉浸环绕声), jymaster(超清母带) 进行音质判断
    Q_INVOKABLE QVariantMap song_download_url_v1(QVariantMap);

    // 获取客户端歌曲下载链接
    Q_INVOKABLE QVariantMap song_download_url(QVariantMap);

    // 歌曲动态封面
    Q_INVOKABLE QVariantMap song_dynamic_cover(QVariantMap);

    // 歌曲是否喜爱
    Q_INVOKABLE QVariantMap song_like_check(QVariantMap);

    // 歌词摘录 - 添加/修改摘录歌词
    Q_INVOKABLE QVariantMap song_lyrics_mark_add(QVariantMap);

    // 歌词摘录 - 删除摘录歌词
    Q_INVOKABLE QVariantMap song_lyrics_mark_del(QVariantMap);

    // 歌词摘录 - 我的歌词本
    Q_INVOKABLE QVariantMap song_lyrics_mark_user_page(QVariantMap);

    // 歌词摘录 - 歌词摘录信息
    Q_INVOKABLE QVariantMap song_lyrics_mark(QVariantMap);

    // 会员本月下载歌曲记录
    Q_INVOKABLE QVariantMap song_monthdownlist(QVariantMap);

    // 歌曲链接 - v1
    // 此版本不再采用 br 作为音质区分的标准
    // 而是采用 standard, exhigh, lossless, hires, jyeffect(高清环绕声), sky(沉浸环绕声), jymaster(超清母带) 进行音质判断
    Q_INVOKABLE QVariantMap song_url_v1(QVariantMap);

    // 歌曲链接
    Q_INVOKABLE QVariantMap song_url(QVariantMap);

    // 音乐百科基础信息
    Q_INVOKABLE QVariantMap song_wiki_summary(QVariantMap);

    // 云村星评馆 - 简要评论列表
    Q_INVOKABLE QVariantMap starpick_comments_summary(QVariantMap);

    // 歌曲音质详情
    Q_INVOKABLE QVariantMap song_music_detail(QVariantMap);

    // 更新歌曲顺序
    Q_INVOKABLE QVariantMap song_order_update(QVariantMap);

    // 已购单曲
    Q_INVOKABLE QVariantMap song_purchased(QVariantMap);

    // 歌曲红心数量
    Q_INVOKABLE QVariantMap song_red_count(QVariantMap);

    // 已购买单曲
    Q_INVOKABLE QVariantMap song_singledownlist(QVariantMap);

    // 曲风-专辑
    Q_INVOKABLE QVariantMap style_album(QVariantMap);

    // 曲风-歌手
    Q_INVOKABLE QVariantMap style_artist(QVariantMap);

    // 曲风详情
    Q_INVOKABLE QVariantMap style_detail(QVariantMap);

    // 曲风列表
    Q_INVOKABLE QVariantMap style_list(QVariantMap);

    // 曲风-歌单
    Q_INVOKABLE QVariantMap style_playlist(QVariantMap);

    // 曲风偏好
    Q_INVOKABLE QVariantMap style_preference(QVariantMap);

    // 曲风-歌曲
    Q_INVOKABLE QVariantMap style_song(QVariantMap);

    // 年度听歌报告2017-2023
    Q_INVOKABLE QVariantMap summary_annual(QVariantMap);

    // 获取达人达标信息
    Q_INVOKABLE QVariantMap threshold_detail_get(QVariantMap);

    // 新碟上架
    Q_INVOKABLE QVariantMap top_album(QVariantMap);

    // 热门歌手
    Q_INVOKABLE QVariantMap top_artists(QVariantMap);

    // 排行榜
    Q_INVOKABLE QVariantMap top_list(QVariantMap);

    // MV排行榜
    Q_INVOKABLE QVariantMap top_mv(QVariantMap);

    // 精品歌单
    Q_INVOKABLE QVariantMap top_playlist_highquality(QVariantMap);

    // 分类歌单
    Q_INVOKABLE QVariantMap top_playlist(QVariantMap);

    // 新歌速递
    Q_INVOKABLE QVariantMap top_song(QVariantMap);

    // 获取话题详情热门动态
    Q_INVOKABLE QVariantMap topic_detail_event_hot(QVariantMap);

    // 获取话题详情
    Q_INVOKABLE QVariantMap topic_detail(QVariantMap);

    // 收藏的专栏
    Q_INVOKABLE QVariantMap topic_sublist(QVariantMap);

    // 歌手榜
    Q_INVOKABLE QVariantMap toplist_artist(QVariantMap);

    // 所有榜单内容摘要
    Q_INVOKABLE QVariantMap toplist_detail(QVariantMap);

    // 所有榜单介绍
    Q_INVOKABLE QVariantMap toplist(QVariantMap);

    // 专辑简要百科信息
    Q_INVOKABLE QVariantMap ugc_album_get(QVariantMap);

    // 歌手简要百科信息
    Q_INVOKABLE QVariantMap ugc_artist_get(QVariantMap);

    // 搜索歌手
    // 可传关键字或者歌手id
    Q_INVOKABLE QVariantMap ugc_artist_search(QVariantMap);

    // mv简要百科信息
    Q_INVOKABLE QVariantMap ugc_mv_get(QVariantMap);

    // 用户贡献内容
    Q_INVOKABLE QVariantMap ugc_detail(QVariantMap);

    // 歌曲简要百科信息
    Q_INVOKABLE QVariantMap ugc_song_get(QVariantMap);

    // 用户贡献条目、积分、云贝数量
    Q_INVOKABLE QVariantMap ugc_user_devote(QVariantMap);

    // 获取账号信息
    Q_INVOKABLE QVariantMap user_account(QVariantMap);

    // 用户创建的电台
    Q_INVOKABLE QVariantMap user_audio(QVariantMap);

    // 获取用户绑定信息
    Q_INVOKABLE QVariantMap user_binding(QVariantMap);

    // 云盘歌曲删除
    Q_INVOKABLE QVariantMap user_cloud_del(QVariantMap);

    // 云盘数据详情
    Q_INVOKABLE QVariantMap user_cloud_detail(QVariantMap);

    // 云盘数据
    Q_INVOKABLE QVariantMap user_cloud(QVariantMap);

    // 获取用户历史评论
    Q_INVOKABLE QVariantMap user_comment_history(QVariantMap);

    // 用户详情
    Q_INVOKABLE QVariantMap user_detail(QVariantMap);

    // 用户电台节目
    Q_INVOKABLE QVariantMap user_dj(QVariantMap);

    // 用户动态
    Q_INVOKABLE QVariantMap user_event(QVariantMap);

    // 当前账号关注的用户/歌手
    Q_INVOKABLE QVariantMap user_follow_mixed(QVariantMap);

    // 关注TA的人(粉丝)
    Q_INVOKABLE QVariantMap user_followeds(QVariantMap);

    // TA关注的人(关注)
    Q_INVOKABLE QVariantMap user_follows(QVariantMap);

    // 获取用户等级信息
    Q_INVOKABLE QVariantMap user_level(QVariantMap);

    // 用户徽章
    Q_INVOKABLE QVariantMap user_medal(QVariantMap);

    // 用户是否互相关注
    Q_INVOKABLE QVariantMap user_mutualfollow_get(QVariantMap);

    // 获取用户的收藏歌单列表
    Q_INVOKABLE QVariantMap user_playlist_collect(QVariantMap);

    // 获取用户的创建歌单列表
    Q_INVOKABLE QVariantMap user_playlist_create(QVariantMap);

    // 用户歌单
    Q_INVOKABLE QVariantMap user_playlist(QVariantMap);

    // 听歌排行
    Q_INVOKABLE QVariantMap user_record(QVariantMap);

    // 用户绑定手机
    Q_INVOKABLE QVariantMap user_replacephone(QVariantMap);

    // 用户状态 - 编辑
    Q_INVOKABLE QVariantMap user_social_status_edit(QVariantMap);

    // 用户状态 - 相同状态的用户
    Q_INVOKABLE QVariantMap user_social_status_rcmd(QVariantMap);

    // 用户状态 - 支持设置的状态
    Q_INVOKABLE QVariantMap user_social_status_support(QVariantMap);

    // 用户状态
    Q_INVOKABLE QVariantMap user_social_status(QVariantMap);

    // 收藏计数
    Q_INVOKABLE QVariantMap user_subcount(QVariantMap);

    // 编辑用户信息
    Q_INVOKABLE QVariantMap user_update(QVariantMap);

    // 验证接口-二维码生成
    Q_INVOKABLE QVariantMap verify_getQr(QVariantMap);

    // 验证接口-二维码检测
    Q_INVOKABLE QVariantMap verify_qrcodestatus(QVariantMap);

    // 视频分类列表
    Q_INVOKABLE QVariantMap video_category_list(QVariantMap);

    // 视频点赞转发评论数数据
    Q_INVOKABLE QVariantMap video_detail_info(QVariantMap);

    // 视频详情
    Q_INVOKABLE QVariantMap video_detail(QVariantMap);

    // 视频标签列表
    Q_INVOKABLE QVariantMap video_group_list(QVariantMap);

    // 视频标签/分类下的视频
    Q_INVOKABLE QVariantMap video_group(QVariantMap);

    // 视频标签/分类下的视频
    Q_INVOKABLE QVariantMap video_sub(QVariantMap);

    // 全部视频列表
    Q_INVOKABLE QVariantMap video_timeline_all(QVariantMap);

    // 推荐视频
    Q_INVOKABLE QVariantMap video_timeline_recommend(QVariantMap);

    // 视频链接
    Q_INVOKABLE QVariantMap video_url(QVariantMap);

    // 黑胶时光机
    Q_INVOKABLE QVariantMap vip_timemachine(QVariantMap);

    // 播客删除
    Q_INVOKABLE QVariantMap voice_delete(QVariantMap);

    // 播客声音详情
    Q_INVOKABLE QVariantMap voice_detail(QVariantMap);

    // 获取声音歌词
    Q_INVOKABLE QVariantMap voice_lyric(QVariantMap);

    // 播客列表详情
    Q_INVOKABLE QVariantMap voicelist_detail(QVariantMap);

    // 声音搜索
    Q_INVOKABLE QVariantMap voicelist_list_search(QVariantMap);

    // 播客声音列表
    Q_INVOKABLE QVariantMap voicelist_list(QVariantMap);

    // 播客列表
    Q_INVOKABLE QVariantMap voicelist_search(QVariantMap);

    // 播客声音排序
    Q_INVOKABLE QVariantMap voicelist_trans(QVariantMap);

    // 操作记录
    Q_INVOKABLE QVariantMap weblog(QVariantMap);

    // 云贝 todo 任务
    Q_INVOKABLE QVariantMap yunbei_tasks_todo(QVariantMap);

    // 云贝所有任务
    Q_INVOKABLE QVariantMap yunbei_tasks(QVariantMap);

    // 云贝今日签到信息
    Q_INVOKABLE QVariantMap yunbei_today(QVariantMap);

    // 云贝
    Q_INVOKABLE QVariantMap yunbei(QVariantMap);
};
#endif // MODULE_H
