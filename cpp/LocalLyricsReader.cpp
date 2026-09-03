// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "LocalLyricsReader.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>
#include <numeric>

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
    return line;
}

QString localFilePath(const QString &filePath)
{
    const QString fromUrl = QUrl::fromUserInput(filePath).toLocalFile();
    return fromUrl.isEmpty() ? filePath : fromUrl;
}

// 毫秒小数：LRC 允许 1~3 位小数，统一补齐到 3 位再截取，避免 0.5 被读成 5ms。
qint64 threeDigitFraction(const QString &fraction)
{
    QString value = fraction;
    while (value.size() < 3)
        value.append(QLatin1Char('0'));
    return value.left(3).toLongLong();
}

// 字级标签沿用在线歌词的两种写法：
//   1) 增强 LRC：<mm:ss[.f]>，时间是相对整首歌的绝对位置
//   2) KRC 风格：<offset,duration,flag>，offset/duration 相对所在行
struct TimedWord
{
    bool relative = false;
    qint64 absoluteTime = 0;
    qint64 offset = 0;
    qint64 duration = 0;
    QString text;
};

struct WordLabels
{
    QList<TimedWord> words;
    QString leading; // 第一个字级标签之前未标注的文本（增强 LRC 常见）
};

QString withoutWordLabels(const QString &contents);

WordLabels extractWordLabels(const QString &contents)
{
    static const QRegularExpression re(QStringLiteral(
        R"(<(?:(?:(\d{1,3}):(\d{2})(?:\.(\d{1,3}))?)|(?:(\d+),(\d+),\d+))>([^<]*))"));

    WordLabels labels;
    int firstStart = -1;
    QRegularExpressionMatchIterator it = re.globalMatch(contents);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        TimedWord word;
        word.text = match.captured(6);
        if (word.text.isEmpty())
            continue;
        if (firstStart < 0)
            firstStart = match.capturedStart(0);
        if (!match.captured(4).isEmpty()) {
            word.relative = true;
            word.offset = match.captured(4).toLongLong();
            word.duration = match.captured(5).toLongLong();
        } else {
            word.absoluteTime = match.captured(1).toLongLong() * 60000
                                + match.captured(2).toLongLong() * 1000
                                + threeDigitFraction(match.captured(3));
        }
        labels.words.append(word);
    }
    if (firstStart > 0)
        labels.leading = withoutWordLabels(contents.left(firstStart));
    return labels;
}

QString withoutWordLabels(const QString &contents)
{
    static const QRegularExpression re(QStringLiteral(
        R"(<(?:(?:(?:\d{1,3}):(?:\d{2})(?:\.\d{1,3})?)|(?:\d+,\d+,\d+))>)"));
    QString plain = contents;
    plain.remove(re);
    return plain.trimmed();
}

struct WordEntry
{
    qint64 effective = 0; // 合成到整首歌坐标后的时间，便于统一算 offset/duration
    bool hasExplicitDuration = false;
    qint64 explicitDuration = 0;
    QString text;
};

// 一行已解析但尚未定稿的歌词（多时间戳行会被展开成多条）
struct RawLine
{
    qint64 time = 0;
    QString text;
    QVariant info; // null：整行高亮；否则 [{offset, duration, text}]
    bool isOther = false;
};

// 双语 LRC 的写法：翻译行与原行共用同一时间轴，紧跟在原行之后
bool isTranslationOf(const RawLine &original, const RawLine &candidate)
{
    return candidate.time == original.time && !candidate.text.isEmpty()
           && candidate.text != original.text;
}

QVariantMap toLyricMap(const RawLine &line)
{
    QVariantMap map = lyricLine(line.time, line.text);
    if (line.info.isValid())
        map.insert(QStringLiteral("info"), line.info);
    if (line.isOther)
        map.insert(QStringLiteral("isOther"), true);
    return map;
}

LocalLyricsReader::Parsed splitTranslations(const QList<RawLine> &raw)
{
    QList<RawLine> lines;
    QStringList translations;
    bool anyTranslation = false;
    for (int i = 0; i < raw.size();) {
        const bool paired = i + 1 < raw.size() && isTranslationOf(raw.at(i), raw.at(i + 1));
        lines.append(raw.at(i));
        translations.append(paired ? raw.at(i + 1).text : QString());
        anyTranslation = anyTranslation || paired;
        i += paired ? 2 : 1;
    }

    // 多时间戳行可能乱序，正文与翻译按同下标一起重排
    QList<int> order(lines.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&lines](int left, int right) {
        return lines.at(left).time < lines.at(right).time;
    });

    LocalLyricsReader::Parsed parsed;
    for (const int index : order) {
        parsed.lyrics.append(toLyricMap(lines.at(index)));
        if (anyTranslation)
            parsed.translate.append(translations.at(index));
    }
    return parsed;
}

} // namespace

QVariantList LocalLyricsReader::parseLrc(const QString &contents)
{
    return parseLrcWithTranslation(contents).lyrics;
}

LocalLyricsReader::Parsed LocalLyricsReader::parseLrcWithTranslation(const QString &contents)
{
    static const QRegularExpression timestamp(
        QStringLiteral(R"(\[(\d{1,3}):(\d{2})(?:\.(\d{1,3}))?\])"));

    QList<RawLine> raw;
    const QString normalized = contents.startsWith(QChar(0xFEFF))
                                   ? contents.mid(1)
                                   : contents;
    const QStringList lines = normalized.split(QRegularExpression(QStringLiteral("[\\r\\n]")),
                                               Qt::KeepEmptyParts);

    for (const QString &line : lines) {
        QRegularExpressionMatchIterator matches = timestamp.globalMatch(line);
        QString text = line;
        QList<qint64> times;
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            const qint64 minutes = match.captured(1).toLongLong();
            const qint64 seconds = match.captured(2).toLongLong();
            times.append((minutes * 60 + seconds) * 1000
                         + threeDigitFraction(match.captured(3)));
        }

        if (times.isEmpty())
            continue;
        text.remove(timestamp);
        const WordLabels labels = extractWordLabels(text);
        const QString plainText = withoutWordLabels(text);
        if (plainText.isEmpty())
            continue;
        const bool isOther = plainText.startsWith(QStringLiteral("（"))
                             && plainText.endsWith(QStringLiteral("）"));

        for (const qint64 time : times) {
            RawLine current;
            current.time = time;
            current.isOther = isOther;
            // 没有字级标签的普通行：info 保持 null，走原来的整行高亮
            if (labels.words.isEmpty() && labels.leading.isEmpty()) {
                current.text = plainText;
                raw.append(current);
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

            current.text = fullText;
            current.info = info;
            raw.append(current);
        }
    }

    return splitTranslations(raw);
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

LocalLyricsReader::Parsed LocalLyricsReader::parseEmbeddedLyrics(const QString &filePath)
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
                        return {lyrics, {}};
                }
            }

            const auto unsynchronized = id3v2->frameList("USLT");
            for (auto *frame : unsynchronized) {
                auto *uslt = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame *>(frame);
                if (!uslt)
                    continue;
                const QString text = tagString(uslt->text());
                const Parsed timed = parseLrcWithTranslation(text);
                if (!timed.lyrics.isEmpty())
                    return timed;
                const QVariantList plain = parsePlainLyrics(text);
                if (!plain.isEmpty())
                    return {plain, {}};
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
        const Parsed timed = parseLrcWithTranslation(text);
        return timed.lyrics.isEmpty() ? Parsed{parsePlainLyrics(text), {}} : timed;
    }
    return {};
}

QVariantMap LocalLyricsReader::result(const QString &source, const Parsed &parsed)
{
    QVariantMap value;
    value.insert(QStringLiteral("found"), !parsed.lyrics.isEmpty());
    value.insert(QStringLiteral("source"), source);
    value.insert(QStringLiteral("lyrics"), parsed.lyrics);
    value.insert(QStringLiteral("translate"), parsed.translate);
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
        const Parsed parsed = parseLrcWithTranslation(QString::fromUtf8(sidecar.readAll()));
        if (!parsed.lyrics.isEmpty())
            return result(QStringLiteral("sidecar"), parsed);
    }

    const Parsed embedded = parseEmbeddedLyrics(localPath);
    if (!embedded.lyrics.isEmpty())
        return result(QStringLiteral("embedded"), embedded);
    return result(QStringLiteral("none"), {});
}