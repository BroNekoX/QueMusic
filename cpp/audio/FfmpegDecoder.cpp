// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "FfmpegDecoder.h"

#include <QFileInfo>
#include <QtGlobal>

#include <algorithm>
#include <cstring>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
#include <libswresample/swresample.h>
}

namespace {

std::once_flag g_networkInit;

QString avErrorText(int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(err, buf, sizeof(buf));
    return QString::fromLocal8Bit(buf);
}

QString dictValue(AVDictionary *dict, const char *key)
{
    const AVDictionaryEntry *e = av_dict_get(dict, key, nullptr, AV_DICT_IGNORE_SUFFIX);
    return e ? QString::fromUtf8(e->value).trimmed() : QString();
}

double dictNumber(AVDictionary *dict, const char *key)
{
    const QString text = dictValue(dict, key);
    if (text.isEmpty())
        return 0.0;
    bool ok = false;
    const double v = text.split(QLatin1Char(' ')).first().toDouble(&ok);
    return ok ? v : 0.0;
}

int layoutChannels(const AVChannelLayout &layout)
{
    return layout.nb_channels > 0 ? layout.nb_channels : 2;
}

} // namespace

FfmpegDecoder::FfmpegDecoder() = default;

FfmpegDecoder::~FfmpegDecoder()
{
    close();
}

int FfmpegDecoder::interruptCallback(void *opaque)
{
    auto *self = static_cast<FfmpegDecoder *>(opaque);
    return self->m_abort.load(std::memory_order_relaxed) ? 1 : 0;
}

bool FfmpegDecoder::open(const QString &pathOrUrl, QString *error, const QByteArray &httpHeaders)
{
    close();
    std::call_once(g_networkInit, [] { avformat_network_init(); });

    m_fmt = avformat_alloc_context();
    if (!m_fmt) {
        if (error) *error = QStringLiteral("无法分配格式上下文");
        return false;
    }
    m_fmt->interrupt_callback.callback = &FfmpegDecoder::interruptCallback;
    m_fmt->interrupt_callback.opaque = this;

    AVDictionary *opts = nullptr;
    // B 站 CDN 校验 Referer/UA，非浏览器 UA 会被拒
    const bool bili = pathOrUrl.contains(QLatin1String("bilivideo"), Qt::CaseInsensitive)
                      || pathOrUrl.contains(QLatin1String("bilibili"), Qt::CaseInsensitive);
    if (bili) {
        av_dict_set(&opts, "user_agent",
                    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                    "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
                    0);
        av_dict_set(&opts, "referer", "https://www.bilibili.com", 0);
    } else {
        av_dict_set(&opts, "user_agent", "QueMusic", 0);
    }
    av_dict_set(&opts, "rw_timeout", "15000000", 0);
    av_dict_set(&opts, "timeout", "15000000", 0);
    av_dict_set(&opts, "reconnect", "1", 0);
    av_dict_set(&opts, "reconnect_streamed", "1", 0);
    av_dict_set(&opts, "reconnect_delay_max", "5", 0);
    // 远端鉴权等自定义头（WebDAV 用）
    if (!httpHeaders.isEmpty())
        av_dict_set(&opts, "headers", httpHeaders.constData(), 0);

    const QByteArray target = pathOrUrl.toUtf8();
    int ret = avformat_open_input(&m_fmt, target.constData(), nullptr, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        if (error) *error = avErrorText(ret);
        close();
        return false;
    }

    ret = avformat_find_stream_info(m_fmt, nullptr);
    if (ret < 0) {
        if (error) *error = avErrorText(ret);
        close();
        return false;
    }

    const AVCodec *decoder = nullptr;
    m_streamIndex = av_find_best_stream(m_fmt, AVMEDIA_TYPE_AUDIO, -1, -1, &decoder, 0);
    if (m_streamIndex < 0 || !decoder) {
        if (error) *error = QStringLiteral("没有可解码的音频流");
        close();
        return false;
    }

    AVStream *stream = m_fmt->streams[m_streamIndex];
    m_codecCtx = avcodec_alloc_context3(decoder);
    if (!m_codecCtx) {
        if (error) *error = QStringLiteral("无法分配解码上下文");
        close();
        return false;
    }
    avcodec_parameters_to_context(m_codecCtx, stream->codecpar);
    m_codecCtx->pkt_timebase = stream->time_base;
    // 音频解码用不上帧级多线程：thread_count=0 会让 FFmpeg 按 CPU 核数创建线程上下文
    // 及其私有缓冲（本机 16 核即 16 份）。固定 1：48 kHz 立体声的解码开销远小于
    // 一个线程上下文的代价，而线程数与常驻缓冲会一直挂着。
    m_codecCtx->thread_count = 1;

    ret = avcodec_open2(m_codecCtx, decoder, nullptr);
    if (ret < 0) {
        if (error) *error = avErrorText(ret);
        close();
        return false;
    }

    m_pkt = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (!m_pkt || !m_frame) {
        if (error) *error = QStringLiteral("无法分配解码帧");
        close();
        return false;
    }

    m_meta.sampleRate = m_codecCtx->sample_rate;
    m_meta.channels = layoutChannels(m_codecCtx->ch_layout);
    m_meta.codec = QString::fromLatin1(avcodec_get_name(m_codecCtx->codec_id));
    m_meta.bitRate = m_codecCtx->bit_rate > 0 ? int(m_codecCtx->bit_rate / 1000)
                                              : int(m_fmt->bit_rate / 1000);

    if (m_fmt->duration > 0)
        m_meta.durationMs = m_fmt->duration / (AV_TIME_BASE / 1000);
    else if (stream->duration > 0)
        m_meta.durationMs = qint64(stream->duration * av_q2d(stream->time_base) * 1000.0);

    AVDictionary *meta = stream->metadata;
    m_meta.tagTitle = dictValue(meta, "title");
    m_meta.artist = dictValue(meta, "artist");
    if (m_meta.artist.isEmpty())
        m_meta.artist = dictValue(meta, "album_artist");
    m_meta.album = dictValue(meta, "album");
    m_meta.date = dictValue(meta, "date");
    if (m_meta.tagTitle.isEmpty())
        m_meta.tagTitle = dictValue(m_fmt->metadata, "title");
    if (m_meta.album.isEmpty())
        m_meta.album = dictValue(m_fmt->metadata, "album");
    if (m_meta.date.isEmpty())
        m_meta.date = dictValue(m_fmt->metadata, "date");
    m_meta.title = m_meta.tagTitle.isEmpty() ? QFileInfo(pathOrUrl).completeBaseName()
                                             : m_meta.tagTitle;

    m_meta.rgTrackDb = dictNumber(meta, "REPLAYGAIN_TRACK_GAIN");
    m_meta.rgAlbumDb = dictNumber(meta, "REPLAYGAIN_ALBUM_GAIN");
    m_meta.rgTrackPeak = dictNumber(meta, "REPLAYGAIN_TRACK_PEAK");

    m_eof = false;
    m_skipUntilMs = 0.0;
    m_stageFrames = m_stageOffset = 0;
    clearAbort();

    if (!buildResampler()) {
        if (error) *error = QStringLiteral("无法初始化重采样器");
        close();
        return false;
    }
    return true;
}

bool FfmpegDecoder::buildResampler()
{
    swr_free(&m_swr);

    AVChannelLayout inLayout;
    if (m_codecCtx->ch_layout.nb_channels > 0)
        av_channel_layout_copy(&inLayout, &m_codecCtx->ch_layout);
    else
        av_channel_layout_default(&inLayout, 2);

    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, m_outChannels);

    const int ret = swr_alloc_set_opts2(&m_swr, &outLayout, AV_SAMPLE_FMT_FLT, m_outRate,
                                        &inLayout, m_codecCtx->sample_fmt,
                                        m_codecCtx->sample_rate, 0, nullptr);
    av_channel_layout_uninit(&inLayout);
    av_channel_layout_uninit(&outLayout);
    if (ret < 0 || !m_swr)
        return false;
    if (swr_init(m_swr) < 0) {
        swr_free(&m_swr);
        return false;
    }
    return true;
}

void FfmpegDecoder::setTargetFormat(int sampleRate, int channels)
{
    const int rate = qBound(8000, sampleRate, 384000);
    const int ch = qBound(1, channels, 2);
    if (rate == m_outRate && ch == m_outChannels)
        return;
    m_outRate = rate;
    m_outChannels = ch;
    if (m_codecCtx)
        buildResampler();
}

void FfmpegDecoder::close()
{
    if (m_swr)
        swr_free(&m_swr);
    if (m_frame)
        av_frame_free(&m_frame);
    if (m_pkt)
        av_packet_free(&m_pkt);
    if (m_codecCtx)
        avcodec_free_context(&m_codecCtx);
    if (m_fmt)
        avformat_close_input(&m_fmt);
    m_streamIndex = -1;
    m_stage.clear();
    m_stageFrames = m_stageOffset = 0;
    m_meta = Meta();
    m_eof = false;
}

int FfmpegDecoder::convertFrame(const AVFrame *frame)
{
    if (!m_swr)
        return 0;

    const int capacity = swr_get_out_samples(m_swr, frame->nb_samples);
    if (capacity <= 0)
        return 0;

    const size_t need = size_t(capacity) * size_t(m_outChannels);
    if (m_stage.capacity() < need)
        m_stage.reserve(need);
    m_stage.resize(need);

    uint8_t *out = reinterpret_cast<uint8_t *>(m_stage.data());
    const int got = swr_convert(m_swr, &out, capacity,
                                const_cast<const uint8_t **>(frame->extended_data),
                                frame->nb_samples);
    if (got <= 0)
        return 0;

    m_stageFrames = got;
    m_stageOffset = 0;

    if (m_skipUntilMs > 0.0) {
        const double startMs = m_lastPtsMs;
        const double durMs = double(frame->nb_samples) * 1000.0 / qMax(1, m_codecCtx->sample_rate);
        if (startMs + durMs <= m_skipUntilMs) {
            m_stageFrames = m_stageOffset = 0;
            return 0;
        }
        if (startMs < m_skipUntilMs) {
            const double dropMs = m_skipUntilMs - startMs;
            m_stageOffset = qBound(0, int(dropMs * m_outRate / 1000.0), m_stageFrames);
        }
        m_skipUntilMs = 0.0;
    }

    return m_stageFrames - m_stageOffset;
}

int FfmpegDecoder::decodeNextFrame()
{
    for (;;) {
        const int ret = avcodec_receive_frame(m_codecCtx, m_frame);
        if (ret == 0) {
            const AVStream *stream = m_fmt->streams[m_streamIndex];
            const qint64 ts = (m_frame->best_effort_timestamp != AV_NOPTS_VALUE)
                                  ? m_frame->best_effort_timestamp
                                  : m_frame->pts;
            if (ts != AV_NOPTS_VALUE)
                m_lastPtsMs = double(ts) * av_q2d(stream->time_base) * 1000.0;
            const double durMs = double(m_frame->nb_samples) * 1000.0
                                 / qMax(1, m_codecCtx->sample_rate);
            const int produced = convertFrame(m_frame);
            m_lastPtsMs += durMs;
            av_frame_unref(m_frame);
            if (produced > 0)
                return produced;
            if (m_abort.load(std::memory_order_relaxed))
                return 0;
            continue;
        }
        if (ret == AVERROR(EAGAIN)) {
            if (m_eof)
                return 0;
            const int read = av_read_frame(m_fmt, m_pkt);
            if (read >= 0) {
                if (m_pkt->stream_index == m_streamIndex)
                    avcodec_send_packet(m_codecCtx, m_pkt);
                av_packet_unref(m_pkt);
                continue;
            }
            m_eof = true;
            avcodec_send_packet(m_codecCtx, nullptr);
            continue;
        }
        return 0;
    }
}

int FfmpegDecoder::readFrames(float *out, int frames)
{
    if (!m_codecCtx || !m_swr || frames <= 0)
        return 0;

    int produced = 0;
    while (produced < frames) {
        if (m_stageOffset >= m_stageFrames) {
            m_stageFrames = m_stageOffset = 0;
            if (m_abort.load(std::memory_order_relaxed) || decodeNextFrame() == 0)
                break;
        }
        const int take = qMin(frames - produced, m_stageFrames - m_stageOffset);
        std::memcpy(out + size_t(produced) * m_outChannels,
                    m_stage.data() + size_t(m_stageOffset) * m_outChannels,
                    size_t(take) * m_outChannels * sizeof(float));
        m_stageOffset += take;
        produced += take;
    }
    return produced;
}

bool FfmpegDecoder::seekTo(qint64 ms)
{
    if (!m_fmt || !m_codecCtx || m_streamIndex < 0)
        return false;

    const qint64 target = qMax<qint64>(0, ms);
    if (avformat_seek_file(m_fmt, -1, INT64_MIN, target * 1000, INT64_MAX,
                           AVSEEK_FLAG_BACKWARD) < 0)
        return false;

    avcodec_flush_buffers(m_codecCtx);
    if (m_swr)
        swr_init(m_swr);
    if (m_pkt)
        av_packet_unref(m_pkt);
    m_stageFrames = m_stageOffset = 0;
    m_eof = false;
    m_skipUntilMs = double(target);
    m_lastPtsMs = double(target);
    return true;
}
