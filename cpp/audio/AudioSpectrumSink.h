// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 频谱订阅协议：音频回调线程只持句柄，不直接引用订阅者 QObject
// （destroyed 在析构过程中发出，挡不住竞争）。
#pragma once

#include <atomic>
#include <memory>
#include <thread>

// 频谱数据接收方：pushSamples 由音频回调线程调用，实现方需保证无阻塞
class AudioSpectrumSink
{
public:
    virtual ~AudioSpectrumSink() = default;
    virtual void pushSamples(const float *interleaved, int frames, int channels, int sampleRate) = 0;
};

// 音频线程无锁；detach() 返回后保证不再进入 sink
class AudioSpectrumSinkHandle
{
public:
    explicit AudioSpectrumSinkHandle(AudioSpectrumSink *sink = nullptr) : m_sink(sink) {}

    // 音频线程调用；pushSamples 必须无阻塞
    void push(const float *interleaved, int frames, int channels, int sampleRate)
    {
        m_inFlight.fetch_add(1, std::memory_order_acq_rel);
        if (AudioSpectrumSink *sink = m_sink.load(std::memory_order_acquire))
            sink->pushSamples(interleaved, frames, channels, sampleRate);
        m_inFlight.fetch_sub(1, std::memory_order_release);
    }

    // 订阅者析构前调用：返回后不再进入 sink
    void detach()
    {
        m_sink.store(nullptr, std::memory_order_release);
        while (m_inFlight.load(std::memory_order_acquire) != 0)
            std::this_thread::yield();
    }

private:
    std::atomic<AudioSpectrumSink *> m_sink;
    std::atomic<int> m_inFlight{0};
};

// 订阅者向引擎自报句柄的接口：QObject 订阅者（GetWave）实现它，
// 引擎因此不需要认识具体类型，也不会把 QObject 生命周期带进音频线程
class AudioSpectrumSource
{
public:
    virtual ~AudioSpectrumSource() = default;
    virtual std::shared_ptr<AudioSpectrumSinkHandle> spectrumSinkHandle() = 0;
};