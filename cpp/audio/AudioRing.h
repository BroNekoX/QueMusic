// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 单生产者/单消费者无锁环形缓冲：解码线程写，音频回调线程读
#pragma once

#include <algorithm>
#include <atomic>
#include <cstring>
#include <vector>

class AudioRing
{
public:
    void configure(int capacityFrames, int channels)
    {
        m_channels = std::max(1, channels);
        m_capacity = std::max(1024, capacityFrames) * m_channels;
        m_data.assign(size_t(m_capacity), 0.0f);
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }

    int channels() const { return m_channels; }
    int capacity() const { return m_capacity; }

    int available() const
    {
        // 游标单调递增不回头，相减恒非负；长播下差值才可能超过 int，故夹到上限
        const quint64 d = m_tail.load(std::memory_order_acquire) - m_head.load(std::memory_order_acquire);
        return d > quint64(INT_MAX) ? INT_MAX : int(d);
    }

    int space() const
    {
        const int used = available();
        return used < m_capacity ? m_capacity - used : 0;
    }

    void clear()
    {
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    }

    int write(const float *src, int count)
    {
        const quint64 h = m_head.load(std::memory_order_relaxed);
        const quint64 t = m_tail.load(std::memory_order_relaxed);
        const int n = int(std::min<quint64>(quint64(count), quint64(m_capacity) - (t - h)));
        if (n <= 0)
            return 0;
        const int start = int(t % quint64(m_capacity));
        const int first = std::min(n, m_capacity - start);
        std::memcpy(m_data.data() + start, src, size_t(first) * sizeof(float));
        if (n > first)
            std::memcpy(m_data.data(), src + first, size_t(n - first) * sizeof(float));
        m_tail.store(t + quint64(n), std::memory_order_release);
        return n;
    }

    int read(float *dst, int count)
    {
        const quint64 h = m_head.load(std::memory_order_relaxed);
        const quint64 t = m_tail.load(std::memory_order_acquire);
        const int n = int(std::min<quint64>(quint64(count), t - h));
        if (n <= 0)
            return 0;
        const int start = int(h % quint64(m_capacity));
        const int first = std::min(n, m_capacity - start);
        std::memcpy(dst, m_data.data() + start, size_t(first) * sizeof(float));
        if (n > first)
            std::memcpy(dst + first, m_data.data(), size_t(n - first) * sizeof(float));
        m_head.store(h + quint64(n), std::memory_order_release);
        return n;
    }

private:
    std::vector<float> m_data;
    // 取模只在落盘索引时做，游标本身永不复位：48kHz 下 int 会在约 6 小时后溢出
    std::atomic<quint64> m_head{0};
    std::atomic<quint64> m_tail{0};
    int m_capacity = 0;
    int m_channels = 2;
};
