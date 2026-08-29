// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "LocalLyricsReader.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>

#include <fileref.h>
#include <mpegfile.h>
#include <id3v2tag.h>
#include <synchronizedlyricsframe.h>
#include <unsynchronizedlyricsframe.h>
#include <tpropertymap.h>

namespace {

QString tagString(const TagLib::String &value)
{
    return QString::fromUtf8(value.toCString(true));
}

QVariantMap lyricLine(qint64 time, const QString &text)
{
    QVariantMap line;
    line.insert(QStringLiteral("time"), time);
    line.insert(QStringLiteral("text"), text);
    // 在线歌词的数据契约：无逐字信息时 info 为 null，有逐字时是
    // [{offset, duration, text}]。这里保持一致，避免 QML 侧做额外判断。
    line.insert(QStringLiteral("info"), QVariant());
} // namespace

QVariantList LocalLyricsReader::parseLrc(const QString &contents)
{
    static const QRegularExpression timestamp(
        QStringLiteral(R"(\[(\d{1,3}):(\d{2})(?:\.(\d{1,3}))?\])"));

    QVariantList lyrics;
    const QString normalized = contents.startsWith(QChar(0xFEFF))
        ? contents.mid(1)
        : contents;
    const QStringList lines = normalized.split(QRegularExpression(QStringLiteral("[\\r\\n]")),
                                               Qt::KeepEmptyParts);

    for (const QString &line : lines) {
        QRegularExpressionMatchIterator matches = timestamp.globalMatch(line);
        QString text = line;
        bool hasTimestamp = false;
        QList<qint64> times;
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            hasTimestamp = true;
            const qint64 minutes = match.captured(1).toLongLong();
            const qint64 seconds = match.captured(2).toLongLong();
            times.append((minutes * 60 + seconds) * 1000
                         + threeDigitFraction(match.captured(3)));
        }

        if (!hasTimestamp)
            continue;
        text.remove(timestamp);
        const WordLabels labels = extractWordLabels(text);
        const QString plainText = withoutWordLabels(text);
        if (plainText.isEmpty())
            continue;
        const bool isOther = plainText.startsWith(QStringLiteral("（"))
                           && plainText.endsWith(QStringLiteral("）"));

        for (const qint64 time : times) {
            // 没有字级标签的普通行：info 保持 null，走原来的整行高亮
            if (labels.words.isEmpty() && labels.leading.isEmpty()) {
                lyrics.append(lyricLine(time, plainText));
                continue;
            }

            QList<WordEntry> entries;
            if (!labels.leading.isEmpty()) {
                WordEntry leadingEntry;
                leadingEntry.effective = time;
                leadingEntry.text = labels.leading;
                entries.append(leadingEntry);
            }
            for (const TimedWord &word : labels.words) {
                WordEntry entry;
                if (word.relative) {
                    entry.effective = time + word.offset;
                    entry.hasExplicitDuration = true;
                    entry.explicitDuration = word.duration;
                } else {
                    entry.effective = word.absoluteTime;
                }
                entry.text = word.text;
                entries.append(entry);
            }

            QVariantList info;
            QString fullText;
            qint64 previousDuration = 0;
            for (int i = 0; i < entries.size(); ++i) {
                const WordEntry &entry = entries.at(i);
                QString wordText = entry.text;
                if (isOther) {
                    wordText.remove(QStringLiteral("（"));
                    wordText.remove(QStringLiteral("）"));
                }

                const qint64 offset = qMax<qint64>(0, entry.effective - time);
                qint64 duration = 0;
                if (entry.hasExplicitDuration) {
                    duration = entry.explicitDuration; // KRC 风格沿用在线逐字时长
                } else if (i + 1 < entries.size()) {
                    duration = qMax<qint64>(0, entries.at(i + 1).effective - entry.effective);
                } else {
                    duration = i > 0 ? previousDuration : 0; // 行尾字沿用上一字时长
                }

                QVariantMap word;
                word.insert(QStringLiteral("offset"), offset);
                word.insert(QStringLiteral("duration"), duration);
                word.insert(QStringLiteral("text"), wordText);
                info.append(word);
                fullText += wordText;
                previousDuration = duration;
            }

            QVariantMap item = lyricLine(time, fullText);
            item.insert(QStringLiteral("info"), info);
            if (isOther)
                item.insert(QStringLiteral("isOther"), true);
            lyrics.append(item);
        }
    }

    std::stable_sort(lyrics.begin(), lyrics.end(), [](const QVariant &left, const QVariant &right) {
        return left.toMap().value(QStringLiteral("time")).toLongLong()
             < right.toMap().value(QStringLiteral("time")).toLongLong();
    });
    return lyrics;
}

QVariantList LocalLyricsReader::parsePlainLyrics(const QString &text)
{
    const QString cleaned = text.trimmed();
    if (cleaned.isEmpty())
        return {};

    // USLT 与通用 LYRICS 标签没有时间轴，保留完整文本为一整行，
    // 播放器按无语义时间显示，不伪装出时间戳。
    return {lyricLine(0, cleaned)};
}

QVariantList LocalLyricsReader::parseEmbeddedLyrics(const QString &filePath)
{
    const QByteArray encodedPath = QFile::encodeName(filePath);
    if (encodedPath.isEmpty())
        return {};

    TagLib::FileRef ref(encodedPath.constData(), false);
    if (ref.isNull() || ref.file() == nullptr)
        return {};

    // SYLT 是 ID3v2 的原生同步歌词帧，时间戳为绝对毫秒。每一条目本身
    // 就是一个音节/词，直接映射成在线歌词的字级 info 结构。
    if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(ref.file())) {
        if (auto *id3v2 = mpeg->ID3v2Tag()) {
            const auto synchronized = id3v2->frameList("SYLT");
            for (auto *frame : synchronized) {
                auto *sylt = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame *>(frame);
                if (!sylt || sylt->timestampFormat()
                    != TagLib::ID3v2::SynchronizedLyricsFrame::AbsoluteMilliseconds)
                    continue;

                const auto entries = sylt->synchedText();
                const int count = static_cast<int>(entries.size());
                if (count > 0) {
                    QVariantList lyrics;
                    for (int i = 0; i < count; ++i) {
                        const QString text = tagString(entries[i].text).trimmed();
                        if (text.isEmpty())
                            continue;
                        const qint64 time = entries[i].time;
                        const qint64 nextTime = i + 1 < count ? entries[i + 1].time : time;

                        QVariantMap line = lyricLine(time, text);
                        QVariantList words;
                        QVariantMap word;
                        word.insert(QStringLiteral("offset"), QVariant::fromValue<qint64>(0));
                        word.insert(QStringLiteral("duration"),
                                    qMax<qint64>(0, nextTime - time));
                        word.insert(QStringLiteral("text"), text);
                        words.append(word);
                        line.insert(QStringLiteral("info"), words);
                        lyrics.append(line);
                    }
                    if (!lyrics.isEmpty())
                        return lyrics;
                }
            }

            const auto unsynchronized = id3v2->frameList("USLT");
            for (auto *frame : unsynchronized) {
                auto *uslt = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame *>(frame);
                if (!uslt)
                    continue;
                const QString text = tagString(uslt->text());
                const QVariantList timed = parseLrc(text);
                if (!timed.isEmpty())
                    return timed;
                const QVariantList plain = parsePlainLyrics(text);
                if (!plain.isEmpty())
                    return plain;
            }
        }
    }

    // TagLib exposes Vorbis/FLAC, MP4, ASF and other text metadata through the
    // common property map. This also covers ID3v2 LYRICS properties not selected
    // by the frame-specific path above.
    const TagLib::PropertyMap properties = ref.properties();
    for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
        const QString key = tagString(it->first).toUpper();
        if (!key.startsWith(QStringLiteral("LYRICS")))
            continue;
        const QString text = tagString(it->second.toString("")).trimmed();
        if (text.isEmpty())
            continue;
        const QVariantList timed = parseLrc(text);
        return timed.isEmpty() ? parsePlainLyrics(text) : timed;
    }
    return {};
}

QVariantMap LocalLyricsReader::result(const QString &source, const QVariantList &lyrics)
{
    QVariantMap value;
    value.insert(QStringLiteral("found"), !lyrics.isEmpty());
    value.insert(QStringLiteral("source"), source);
    value.insert(QStringLiteral("lyrics"), lyrics);
    return value;
}

QVariantMap LocalLyricsReader::read(const QString &filePath)
{
    const QString localPath = localFilePath(filePath);
    if (localPath.isEmpty())
        return result(QStringLiteral("none"), {});

    const QFileInfo audioInfo(localPath);
    const QString sidecarPath = audioInfo.absolutePath() + QLatin1Char('/')
                              + audioInfo.completeBaseName() + QStringLiteral(".lrc");
    QFile sidecar(sidecarPath);
    if (sidecar.open(QIODevice::ReadOnly)) {
        const QVariantList lyrics = parseLrc(QString::fromUtf8(sidecar.readAll()));
        if (!lyrics.isEmpty())
            return result(QStringLiteral("sidecar"), lyrics);
    }

    const QVariantList embedded = parseEmbeddedLyrics(localPath);
    if (!embedded.isEmpty())
        return result(QStringLiteral("embedded"), embedded);
    return result(QStringLiteral("none"), {});
}
