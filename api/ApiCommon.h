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

} // namespace ApiCommon

#endif // APICOMMON_H
