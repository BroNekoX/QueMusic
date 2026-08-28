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
    return line;
}

QString localFilePath(const QString &filePath)
{
    const QString fromUrl = QUrl::fromUserInput(filePath).toLocalFile();
    return fromUrl.isEmpty() ? filePath : fromUrl;
}

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
            QString fraction = match.captured(3);
            while (fraction.size() < 3)
                fraction.append(QLatin1Char('0'));
            const qint64 milliseconds = fraction.isEmpty() ? 0 : fraction.left(3).toLongLong();
            times.append((minutes * 60 + seconds) * 1000 + milliseconds);
        }

        if (!hasTimestamp)
            continue;
        text.remove(timestamp);
        text = text.trimmed();
        if (text.isEmpty())
            continue;
        for (const qint64 time : times)
            lyrics.append(lyricLine(time, text));
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

    // USLT and generic LYRICS tags are unsynchronized. Keep the complete text
    // in one item so the player can display it without pretending timestamps.
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

    // SYLT is the native ID3v2 synchronized-lyrics frame. Only millisecond
    // timestamps can be mapped to the playback position without extra data.
    if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(ref.file())) {
        if (auto *id3v2 = mpeg->ID3v2Tag()) {
            const auto synchronized = id3v2->frameList("SYLT");
            for (auto *frame : synchronized) {
                auto *sylt = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame *>(frame);
                if (!sylt || sylt->timestampFormat()
                    != TagLib::ID3v2::SynchronizedLyricsFrame::AbsoluteMilliseconds)
                    continue;

                QVariantList lyrics;
                for (const auto &entry : sylt->synchedText()) {
                    const QString text = tagString(entry.text).trimmed();
                    if (!text.isEmpty())
                        lyrics.append(lyricLine(entry.time, text));
                }
                if (!lyrics.isEmpty())
                    return lyrics;
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
