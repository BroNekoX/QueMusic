// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// FFmpeg 解码器：解封装 + 解码 + 重采样，统一输出 float32 交错 PCM
#pragma once

#include <QByteArray>
#include <QString>

#include <atomic>
#include <vector>

struct AVCodecContext;
struct AVFormatContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;

class FfmpegDecoder
{
public:
    struct Meta
    {
        qint64 durationMs = 0;
        int sampleRate = 0;
        int channels = 0;
        int bitRate = 0;
        QString codec;
        QString title;      // 无标签时回退为文件名
        QString tagTitle;   // 仅标签值，可能为空
        QString artist;
        QString album;
        QString date;
        double rgTrackDb = 0.0;
        double rgAlbumDb = 0.0;
        double rgTrackPeak = 0.0;
    };

    FfmpegDecoder();
    ~FfmpegDecoder();

    FfmpegDecoder(const FfmpegDecoder &) = delete;
    FfmpegDecoder &operator=(const FfmpegDecoder &) = delete;

    // httpHeaders：额外请求头（每行 "Key: Value\r\n"，如远端鉴权用的 Authorization）
    bool open(const QString &pathOrUrl, QString *error, const QByteArray &httpHeaders = {});
    void close();
    bool isOpen() const { return m_fmt != nullptr; }

    const Meta &meta() const { return m_meta; }

    // 仅供解码线程调用
    void setTargetFormat(int sampleRate, int channels);
    int targetSampleRate() const { return m_outRate; }

    // 返回写入的帧数；0 = EOF；-1 = 出错
    int readFrames(float *out, int frames);
    bool seekTo(qint64 ms);

    void abort() { m_abort.store(true, std::memory_order_relaxed); }
    void clearAbort() { m_abort.store(false, std::memory_order_relaxed); }

private:
    static int interruptCallback(void *opaque);
    bool buildResampler();
    int convertFrame(const AVFrame *frame);
    int decodeNextFrame();

    AVFormatContext *m_fmt = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    SwrContext *m_swr = nullptr;
    AVPacket *m_pkt = nullptr;
    AVFrame *m_frame = nullptr;

    int m_streamIndex = -1;
    int m_outRate = 48000;
    int m_outChannels = 2;

    std::vector<float> m_stage;
    int m_stageFrames = 0;
    int m_stageOffset = 0;

    double m_lastPtsMs = 0.0;
    double m_skipUntilMs = 0.0;
    bool m_eof = false;
    std::atomic<bool> m_abort{false};
    Meta m_meta;
};
