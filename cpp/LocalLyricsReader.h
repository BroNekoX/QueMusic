// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 本地歌词读取：sidecar LRC 与音频内嵌元数据歌词。

#ifndef LOCALLYRICSREADER_H
#define LOCALLYRICSREADER_H

#include <QVariantList>
#include <QVariantMap>

class LocalLyricsReader
{
public:
    // 正文与翻译：translate 与 lyrics 同下标，未识别到翻译时为空表
    struct Parsed
    {
        QVariantList lyrics;
        QVariantList translate;
    };

    // 解析 LRC（标准行时间戳 + 增强的逐字时间戳）为歌词模型
    static QVariantList parseLrc(const QString &contents);

    // 同上，并额外识别"与上一行共用时间轴的翻译行"单独成表
    static Parsed parseLrcWithTranslation(const QString &contents);

    // 先读音频同目录的 sidecar LRC，再读内嵌元数据歌词；返回 found / source / lyrics / translate
    static QVariantMap read(const QString &filePath);

private:
    static Parsed parseEmbeddedLyrics(const QString &filePath);
    static QVariantList parsePlainLyrics(const QString &text);
    static QVariantMap result(const QString &source, const Parsed &parsed);
};

#endif // LOCALLYRICSREADER_H
