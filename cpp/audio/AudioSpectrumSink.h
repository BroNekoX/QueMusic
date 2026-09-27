// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 频谱订阅协议：音频回调线程只持有本文件定义的「句柄」，绝不直接引用订阅者的
// QObject。裸指针 + destroyed 信号那套做法挡不住竞争——destroyed 是在 QObject
// 析构过程中发出的，此时派生类成员往往已经析构，音频线程若在此前读到旧地址
// 仍会踩到已销毁对象。
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

// 跨线程订阅句柄：音频线程与订阅者之间的唯一通道。
//  1) 引擎只保存 shared_ptr<AudioSpectrumSinkHandle>，不保存任何裸 sink 指针；
//  2) 订阅者析构前调用 detach()，detach() 返回后保证没有任何线程还在调用 sink；
//  3) 引擎与订阅者共享句柄所有权，任一方先销毁都不会让调用方踩空指针。
class AudioSpectrumSinkHandle
{
public:
    explicit AudioSpectrumSinkHandle(AudioSpectrumSink *sink = nullptr) : m_sink(sink) {}

    // 音频线程调用：持锁转发。锁只在推送期间持有，pushSamples 必须是纯写入、不阻塞
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