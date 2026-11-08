// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "LocalMedia.h"

#include "LocalLyricsReader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QUrl>
#include <QtConcurrent>

#include <attachedpictureframe.h>
#include <audioproperties.h>
#include <fileref.h>
#include <flacfile.h>
#include <flacpicture.h>
#include <id3v2tag.h>
#include <mp4coverart.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <mpegfile.h>
#include <tag.h>
#include <tbytevector.h>
#include <tfile.h>
#include <tpropertymap.h>
#include <tstring.h>

namespace {

QString localPath(const QString &filePath)
{
    const QString path = QUrl::fromUserInput(filePath).toLocalFile();
    return path.isEmpty() ? filePath : path;
}

QString tagString(const TagLib::String &value)
{
    return QString::fromUtf8(value.toCString(true));
}

TagLib::ByteVector toByteVector(const QByteArray &data)
{
    return TagLib::ByteVector(data.constData(), size_t(data.size()));
}

TagLib::String toTagString(const QString &text)
{
    return TagLib::String(text.toUtf8().constData(), TagLib::String::UTF8);
}

QString imageMime(const QByteArray &data)
{
    if (data.startsWith(QByteArrayLiteral("\x89PNG")))
        return "image/png";
    if (data.startsWith(QByteArrayLiteral("GIF8")))
        return "image/gif";
    if (data.startsWith(QByteArrayLiteral("BM")))
        return "image/bmp";
    return "image/jpeg";
}

bool embedCover(TagLib::File *file, const QByteArray &data)
{
    const QString mime = imageMime(data);
    if (auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file)) {
        TagLib::ID3v2::Tag *tag = mpeg->ID3v2Tag(true);
        if (tag == nullptr)
            return false;
        tag->removeFrames("APIC");
        auto *frame = new TagLib::ID3v2::AttachedPictureFrame;
        frame->setMimeType(toTagString(mime));
        frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
        frame->setPicture(toByteVector(data));
        tag->addFrame(frame);
        return true;
    }
    if (auto *flac = dynamic_cast<TagLib::FLAC::File *>(file)) {
        flac->removePictures();
        auto *picture = new TagLib::FLAC::Picture;
        picture->setMimeType(toTagString(mime));
        picture->setType(TagLib::FLAC::Picture::FrontCover);
        picture->setData(toByteVector(data));
        flac->addPicture(picture);
        return true;
    }
    if (auto *mp4 = dynamic_cast<TagLib::MP4::File *>(file)) {
        TagLib::MP4::Tag *tag = mp4->tag();
        if (tag == nullptr)
            return false;
        auto format = TagLib::MP4::CoverArt::JPEG;
        if (mime == QLatin1String("image/png"))
            format = TagLib::MP4::CoverArt::PNG;
        else if (mime == QLatin1String("image/gif"))
            format = TagLib::MP4::CoverArt::GIF;
        else if (mime == QLatin1String("image/bmp"))
            format = TagLib::MP4::CoverArt::BMP;
        TagLib::MP4::CoverArtList covers;
        covers.append(TagLib::MP4::CoverArt(format, toByteVector(data)));
        tag->setItem("covr", covers);
        return true;
    }
    return false;
}

QString sidecarPath(const QString &path, const char *suffix)
{
    const QFileInfo info(path);
    return info.absolutePath() + QLatin1Char('/') + info.completeBaseName()
           + QLatin1String(suffix);
}

} // namespace

LocalMedia::LocalMedia(QObject *parent)
    : QObject(parent)
{
}

QVariantMap LocalMedia::readMetadata(const QString &filePath)
{
    QVariantMap meta;
    if (filePath.isEmpty())
        return meta;

    const QString path = localPath(filePath);

    // 与音频同名的 .json 是应用自己的元数据 sidecar，优先使用。
    QFile sidecar(sidecarPath(path, ".json"));
    if (sidecar.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(sidecar.readAll());
        if (doc.isObject())
            return doc.object().toVariantMap();
    }

    const QByteArray encodedPath = QFile::encodeName(path);
    TagLib::FileRef ref(encodedPath.constData(), false);
    if (ref.isNull() || ref.file() == nullptr)
        return meta;

    if (const TagLib::Tag *tag = ref.tag()) {
        if (!tag->title().isEmpty())
            meta.insert("title", tagString(tag->title()));
        if (!tag->artist().isEmpty())
            meta.insert("artist", tagString(tag->artist()));
        if (!tag->album().isEmpty())
            meta.insert("album", tagString(tag->album()));
    }

    const QVariantMap lyricInfo = LocalLyricsReader::read(path);
    if (lyricInfo.value("found").toBool()) {
        meta.insert("lyrics", lyricInfo.value("lyrics"));
        meta.insert("translate", lyricInfo.value("translate"));
    }

    return meta;
}

QVariantMap LocalMedia::readAudioInfo(const QString &filePath)
{
    QVariantMap info;
    const QString path = localPath(filePath);
    if (path.isEmpty())
        return info;

    const QFileInfo fileInfo(path);
    info.insert("fileName", fileInfo.fileName());
    info.insert("size", fileInfo.size());
    info.insert("format", fileInfo.suffix().toUpper());

    const QByteArray encoded = QFile::encodeName(path);
    TagLib::FileRef ref(encoded.constData());
    if (ref.isNull() || ref.file() == nullptr)
        return info;

    if (const TagLib::Tag *tag = ref.tag()) {
        info.insert("title", tagString(tag->title()));
        info.insert("artist", tagString(tag->artist()));
        info.insert("album", tagString(tag->album()));
        info.insert("genre", tagString(tag->genre()));
        info.insert("year", int(tag->year()));
        info.insert("track", int(tag->track()));
    }
    if (const TagLib::AudioProperties *properties = ref.audioProperties()) {
        info.insert("duration", properties->lengthInSeconds());
        info.insert("bitrate", properties->bitrate());
        info.insert("sampleRate", properties->sampleRate());
        info.insert("channels", properties->channels());
    }
    return info;
}

QString LocalMedia::readLyricsText(const QString &filePath)
{
    const QString path = localPath(filePath);
    if (path.isEmpty())
        return QString();

    QFile sidecar(sidecarPath(path, ".lrc"));
    if (sidecar.open(QIODevice::ReadOnly))
        return QString::fromUtf8(sidecar.readAll());

    const QByteArray encoded = QFile::encodeName(path);
    TagLib::FileRef ref(encoded.constData(), false);
    if (ref.isNull() || ref.file() == nullptr)
        return QString();

    const TagLib::StringList values = ref.file()->properties().value("LYRICS");
    return values.isEmpty() ? QString() : tagString(values.front());
}

bool LocalMedia::writeMetadata(const QString &filePath, const QVariantMap &meta)
{
    const QString path = localPath(filePath);
    if (path.isEmpty() || !QFileInfo::exists(path))
        return false;

    const QByteArray encoded = QFile::encodeName(path);
    TagLib::FileRef ref(encoded.constData());
    if (ref.isNull() || ref.tag() == nullptr)
        return false;

    const auto tagged = [&meta](const char *key) {
        return toTagString(meta.value(QLatin1String(key)).toString());
    };
    TagLib::Tag *tag = ref.tag();
    tag->setTitle(tagged("title"));
    tag->setArtist(tagged("artist"));
    tag->setAlbum(tagged("album"));
    tag->setGenre(tagged("genre"));
    tag->setYear(meta.value("year").toString().toUInt());
    tag->setTrack(meta.value("track").toString().toUInt());

    const QString lyrics = meta.value("lyrics").toString().trimmed();
    if (!lyrics.isEmpty()) {
        TagLib::PropertyMap properties = ref.file()->properties();
        properties.replace("LYRICS", TagLib::StringList(toTagString(lyrics)));
        ref.file()->setProperties(properties);
    }

    const QString cover = meta.value("cover").toString();
    if (!cover.isEmpty()) {
        QFile image(localPath(cover));
        if (image.open(QIODevice::ReadOnly))
            embedCover(ref.file(), image.readAll());
    }
    return ref.save();
}

QString LocalMedia::coverHint(const QString &filePath)
{
    if (filePath.isEmpty())
        return QString();

    const QString path = localPath(filePath);
    if (const auto it = m_coverHints.constFind(path); it != m_coverHints.constEnd())
        return it.value();

    QString hint;
    QFile file(sidecarPath(path, ".json"));
    if (file.open(QIODevice::ReadOnly)) {
        const QVariantMap meta = QJsonDocument::fromJson(file.readAll()).object().toVariantMap();
        hint = meta.value("cover").toString();
    }

    if (m_coverHints.size() > 8192)
        m_coverHints.clear();
    m_coverHints.insert(path, hint);
    return hint;
}

void LocalMedia::readMetadataAsync(const QString &filePath)
{
    if (filePath.isEmpty()) {
        emit metadataReady(filePath, {});
        return;
    }

    auto *watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this,
            [this, watcher, filePath]() {
                watcher->deleteLater();
                emit metadataReady(filePath, watcher->result());
            });
    watcher->setFuture(QtConcurrent::run(&LocalMedia::readMetadata, filePath));
}

void LocalMedia::writeMetadataAsync(const QString &filePath, const QVariantMap &meta)
{
    auto *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, filePath]() {
        watcher->deleteLater();
        emit metadataSaved(filePath, watcher->result());
    });
    watcher->setFuture(QtConcurrent::run(&LocalMedia::writeMetadata, filePath, meta));
}
