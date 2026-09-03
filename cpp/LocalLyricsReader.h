// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
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

    // Parse LRC (standard line timestamps + enhanced word-level timestamps)
    // into the lyric model used by PlayerMaxCenter.
    static QVariantList parseLrc(const QString &contents);

    // 同上，额外识别"与上一行共用时间轴的翻译行"并单独成表
    static Parsed parseLrcWithTranslation(const QString &contents);

    // Read an audio file's sidecar LRC first, then embedded metadata lyrics.
    // The result contains: found (bool), source (sidecar/embedded),
    // lyrics (list), translate (list).
    static QVariantMap read(const QString &filePath);

private:
    static Parsed parseEmbeddedLyrics(const QString &filePath);
    static QVariantList parsePlainLyrics(const QString &text);
    static QVariantMap result(const QString &source, const Parsed &parsed);
};

#endif // LOCALLYRICSREADER_H
