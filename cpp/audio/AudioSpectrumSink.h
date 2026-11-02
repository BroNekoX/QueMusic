// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 频谱订阅协议：音频回调线程只持句柄，不直接引用订阅者 QObject
// （destroyed 在析构过程中发出，挡不住竞争）。
#pragma once

#include <memory>
#include <mutex>

// 频谱数据接收方：pushSamples 由音频回调线程调用，实现方需保证无阻塞
class AudioSpectrumSink
{
public:
    virtual ~AudioSpectrumSink() = default;
    virtual void pushSamples(const float *interleaved, int frames, int channels, int sampleRate) = 0;
};

// 跨线程订阅句柄：引擎只持 shared_ptr，不持裸 sink；
// detach() 返回后保证没有线程还在调用 sink。
class AudioSpectrumSinkHandle
{
public:
    explicit AudioSpectrumSinkHandle(AudioSpectrumSink *sink = nullptr) : m_sink(sink) {}

    // 音频线程调用；pushSamples 必须无阻塞
    void push(const float *interleaved, int frames, int channels, int sampleRate)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_sink)
            m_sink->pushSamples(interleaved, frames, channels, sampleRate);
    }

    // 订阅者析构前调用：等待正在执行的推送结束，之后不再进入 sink
    void detach()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sink = nullptr;
    }

private:
    std::mutex m_mutex;
    AudioSpectrumSink *m_sink = nullptr;
};

// 订阅者向引擎自报句柄的接口：QObject 订阅者（GetWave）实现它，
// 引擎因此不需要认识具体类型，也不会把 QObject 生命周期带进音频线程
class AudioSpectrumSource
{
public:
    virtual ~AudioSpectrumSource() = default;
    virtual std::shared_ptr<AudioSpectrumSinkHandle> spectrumSinkHandle() = 0;
};