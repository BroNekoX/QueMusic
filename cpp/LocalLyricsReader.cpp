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

    static const QRegularExpression lineBreak(QStringLiteral("[\\r\\n]"));

    QList<RawLine> raw;
    const QString normalized = contents.startsWith(QChar(0xFEFF))
                                   ? contents.mid(1)
                                   : contents;
    const QStringList lines = normalized.split(lineBreak, Qt::KeepEmptyParts);

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

// 标签里的整段歌词文本：能解析出时间轴就按 LRC 处理（含翻译配对），否则整段作为一行
static LocalLyricsReader::Parsed parseEmbeddedText(const QString &text)
{
    const LocalLyricsReader::Parsed timed = LocalLyricsReader::parseLrcWithTranslation(text);
    if (!timed.lyrics.isEmpty())
        return timed;
    const QString plain = text.trimmed();
    if (plain.isEmpty())
        return {};
    return {{lyricLine(0, plain)}, {}, QString()};
}

// 把 SYLT 音节按时间落到各行：行内容与翻译不变，只补行内逐字 info
static LocalLyricsReader::Parsed attachSyllables(LocalLyricsReader::Parsed parsed,
                                                 const QList<QPair<qint64, QString>> &syllables)
{
    if (syllables.isEmpty())
        return parsed;
    const int lines = parsed.lyrics.size();
    int i = 0;
    for (int li = 0; li < lines; ++li) {
        QVariantMap line = parsed.lyrics.at(li).toMap();
        const qint64 start = line.value(QStringLiteral("time")).toLongLong();
        const bool last = li + 1 >= lines;
        const qint64 stop = last ? 0 : parsed.lyrics.at(li + 1).toMap()
                                              .value(QStringLiteral("time")).toLongLong();
        QVariantList words;
        while (i < syllables.size() && (last || syllables.at(i).first < stop)) {
            const qint64 time = syllables.at(i).first;
            const QString text = syllables.at(i).second;
            if (time >= start && !text.isEmpty()) {
                const qint64 next = i + 1 < syllables.size() ? syllables.at(i + 1).first
                                                              : time + 320;
                words << QVariantMap{
                    {QStringLiteral("offset"), QVariant::fromValue<qint64>(time - start)},
                    {QStringLiteral("duration"),
                     QVariant::fromValue<qint64>(qMax<qint64>(0, next - time))},
                    {QStringLiteral("text"), text}};
            }
            ++i;
        }
        if (!words.isEmpty()) {
            line.insert(QStringLiteral("info"), words);
            parsed.lyrics[li] = line;
        }
    }
    return parsed;
}

// 只有 SYLT 时：按"停顿"（间隔明显大于行内平均）分组为显示行，行内音节进 info
static LocalLyricsReader::Parsed groupSyllables(const QList<QPair<qint64, QString>> &items,
                                                qint64 defaultGap, int maxLineChars)
{
    const int n = items.size();
    QVariantList lyrics;
    for (int i = 0; i < n;) {
        const qint64 lineTime = items.at(i).first;
        int end = i + 1;
        int chars = qMax(0, items.at(i).second.trimmed().size());
        qint64 gapSum = 0;
        int gapCount = 0;
        while (end < n && chars < maxLineChars) {
            const qint64 gap = items.at(end).first - items.at(end - 1).first;
            const qint64 avg = gapCount > 0 ? gapSum / gapCount : defaultGap;
            if (gap >= qMax<qint64>(320, avg * 3 / 2))
                break;
            gapSum += gap;
            ++gapCount;
            chars += qMax(0, items.at(end).second.trimmed().size());
            ++end;
        }

        QVariantList words;
        QString fullText;
        for (int j = i; j < end; ++j) {
            const QString raw = items.at(j).second;
            if (raw.trimmed().isEmpty() && fullText.isEmpty())
                continue;                       // 行首空白丢弃，行内保留
            const qint64 start = items.at(j).first;
            const qint64 stop = j + 1 < n ? items.at(j + 1).first : start + defaultGap;
            words << QVariantMap{
                {QStringLiteral("offset"), QVariant::fromValue<qint64>(start - lineTime)},
                {QStringLiteral("duration"),
                 QVariant::fromValue<qint64>(qMax<qint64>(0, stop - start))},
                {QStringLiteral("text"), raw}};
            fullText += raw;
        }
        if (!fullText.trimmed().isEmpty()) {
            QVariantMap line = lyricLine(lineTime, fullText.trimmed());
            line.insert(QStringLiteral("info"), words);
            lyrics.append(line);
        }
        i = end;
    }
    return {lyrics, {}, QStringLiteral("embedded-sylt")};
}

LocalLyricsReader::Parsed LocalLyricsReader::parseEmbeddedLyrics(const QString &filePath)
{
    const QByteArray encodedPath = QFile::encodeName(filePath);
    if (encodedPath.isEmpty())
        return {};

    TagLib::FileRef ref(encodedPath.constData(), false);
    if (ref.isNull() || ref.file() == nullptr)
        return {};

    constexpr qint64 kDefaultGap = 320;   // 仅 SYLT 时的默认音节间隔(ms)
    constexpr int kMaxLineChars = 28;     // 仅 SYLT 时的单行字数兜底

    // SYLT（ID3v2 同步歌词，绝对毫秒）一条只是一个音节/词：有行级歌词时只用来补行内逐字，
    // 没有行级歌词时才按"停顿"分组为显示行。
    QList<QPair<qint64, QString>> syllables;
    if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(ref.file())) {
        if (auto *id3v2 = mpeg->ID3v2Tag()) {
            for (auto *frame : id3v2->frameList("SYLT")) {
                auto *sylt = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame *>(frame);
                if (!sylt || sylt->timestampFormat()
                                 != TagLib::ID3v2::SynchronizedLyricsFrame::AbsoluteMilliseconds)
                    continue;
                for (const auto &entry : sylt->synchedText())
                    syllables.append({entry.time, tagString(entry.text)});
            }

            // 行级歌词优先（USLT 通常带翻译），拿到就返回并补上逐字
            for (auto *frame : id3v2->frameList("USLT")) {
                auto *uslt = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame *>(frame);
                if (!uslt)
                    continue;
                const Parsed parsed = parseEmbeddedText(tagString(uslt->text()));
                if (!parsed.lyrics.isEmpty())
                    return attachSyllables(parsed, syllables);
            }
        }
    }

    // Vorbis/FLAC、MP4、ASF 等把歌词放在通用属性表里（也覆盖上面的 ID3v2 LYRICS）
    const TagLib::PropertyMap properties = ref.properties();
    for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
        const QString key = tagString(it->first).toUpper();
        if (!key.startsWith(QStringLiteral("LYRICS")))
            continue;
        const QString text = tagString(it->second.toString("")).trimmed();
        if (text.isEmpty())
            continue;
        const Parsed parsed = parseEmbeddedText(text);
        if (!parsed.lyrics.isEmpty())
            return attachSyllables(parsed, syllables);
    }

    if (!syllables.isEmpty())     // 只有 SYLT：按停顿分组为显示行
        return groupSyllables(syllables, kDefaultGap, kMaxLineChars);
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
    const QString base = audioInfo.absolutePath() + QLatin1Char('/')
                         + audioInfo.completeBaseName();
    // 同名 .lrc 优先，其次同名 .txt（内容不是 LRC 时自动跳过）
    for (const QString &ext : QStringList{QStringLiteral(".lrc"), QStringLiteral(".txt")}) {
        QFile sidecar(base + ext);
        if (!sidecar.open(QIODevice::ReadOnly))
            continue;
        const Parsed parsed = parseLrcWithTranslation(QString::fromUtf8(sidecar.readAll()));
        if (parsed.lyrics.isEmpty())
            continue;
        return result(QStringLiteral("sidecar"), parsed);
    }

    const Parsed embedded = parseEmbeddedLyrics(localPath);
    if (!embedded.lyrics.isEmpty()) {
        return result(embedded.source.isEmpty() ? QStringLiteral("embedded")
                                                : embedded.source,
                      embedded);
    }
    return result(QStringLiteral("none"), {});
}