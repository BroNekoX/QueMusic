// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 可播放媒体后缀的唯一来源：本地扫描、WebDAV 列表、已下载列表共用一份，
// 避免三处白名单分叉（同一文件在网盘能播、下载页却不显示）。
#pragma once

#include <QFileInfo>
#include <QString>
#include <QStringList>

namespace MediaTypes {

inline const QStringList &extensions()
{
    static const QStringList exts = {
        QStringLiteral("mp3"),  QStringLiteral("flac"), QStringLiteral("ape"),
        QStringLiteral("wav"),  QStringLiteral("aiff"), QStringLiteral("dsf"),
        QStringLiteral("dff"),  QStringLiteral("m4a"),  QStringLiteral("m4b"),
        QStringLiteral("aac"),  QStringLiteral("alac"), QStringLiteral("ogg"),
        QStringLiteral("opus"), QStringLiteral("wma"),  QStringLiteral("ac3"),
        QStringLiteral("eac3"), QStringLiteral("mp4"),  QStringLiteral("mkv"),
        QStringLiteral("avi"),  QStringLiteral("wmv"),  QStringLiteral("mpeg4"),
    };
    return exts;
}

inline bool isAudioFile(const QFileInfo &info)
{
    return extensions().contains(info.suffix().toLower());
}

inline bool isAudioName(const QString &fileName)
{
    const int dot = fileName.lastIndexOf(QLatin1Char('.'));
    return dot > 0 && extensions().contains(fileName.mid(dot + 1).toLower());
}

} // namespace MediaTypes
