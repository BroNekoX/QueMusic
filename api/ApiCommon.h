// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// ApiCommon — 在线音乐 API 统一字段层（纯头文件）
// 所有平台解析结果都转换为同一套字段，QML ListModel 直接消费。
#ifndef APICOMMON_H
#define APICOMMON_H

#include <QVariantList>
#include <QVariantMap>

namespace ApiCommon {

// 统一字段名（与旧 musicWorker.mjs 输出完全一致，QML 无需改动）
inline const QString kTitle     = QStringLiteral("title");
inline const QString kArtist    = QStringLiteral("artist");
inline const QString kCover     = QStringLiteral("cover");
inline const QString kHash      = QStringLiteral("hash");    // 通用 ID
inline const QString kHashHq    = QStringLiteral("hashhq");  // 高品质 ID
inline const QString kHashSq    = QStringLiteral("hashsq");  // 无损 ID
inline const QString kPaytype   = QStringLiteral("paytype");

// paytype 取值约定（唯一权威，各平台必须把自家枚举归一到这两个值之一）：
//   0 = 免费可播放；3 = 需要会员/付费
// 消费端只认 3：VIP 角标（components/QListView.qml）、「仅免费 / 仅 VIP」筛选
// （MusicApiService::songList）。所以平台侧绝不能把自家枚举原样透传 ——
// 网易云 fee=1（VIP专享）被映射成 1、酷狗 pay_type=1（会员）被直传成 1，
// 两边都落进没有消费端认的槽位，会员曲目反而不显示 VIP 角标。
inline constexpr int kPaytypeFree = 0;
inline constexpr int kPaytypePaid = 3;
inline const QString kDuration  = QStringLiteral("duration"); // 秒
inline const QString kAlbum     = QStringLiteral("album");
inline const QString kPlaycount = QStringLiteral("playcount");

// 快速构造统一歌曲/歌单对象（hashhq/hashsq 缺省时回退为 hash）
inline QVariantMap song(QString title, QString artist, QString cover, QString hash,
                        int duration = 0, QString album = QString(),
                        QString hashhq = QString(), QString hashsq = QString(),
                        int paytype = 0, qint64 playcount = 0)
{
    return {
        {kTitle,     title},
        {kArtist,    artist},
        {kCover,     cover},
        {kHash,      hash},
        {kHashHq,    hashhq.isEmpty() ? hash : hashhq},
        {kHashSq,    hashsq.isEmpty() ? hash : hashsq},
        {kPaytype,   paytype},
        {kDuration,  duration},
        {kAlbum,     album},
        {kPlaycount, playcount},
    };
}

// 列表结果统一包装为 { info: [...] }（与旧协议一致）
inline QVariantMap listResult(const QVariantList &items)
{
    QVariantMap m;
    m.insert(QStringLiteral("info"), items);
    return m;
}

// 统一评论字段：各平台解析后都归一到这一套，QML 侧不做平台分支
inline QVariantMap comment(QString user, QString avatar, QString content,
                           qint64 time = 0, int liked = 0, int replies = 0)
{
    return {
        {QStringLiteral("user"),    user},
        {QStringLiteral("avatar"),  avatar},
        {QStringLiteral("content"), content},
        {QStringLiteral("time"),    time}, // 秒
        {QStringLiteral("liked"),   liked},
        {QStringLiteral("replies"), replies},
    };
}

} // namespace ApiCommon

#endif // APICOMMON_H
