// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "TimeStretch.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
// 预留余量，保证回调内不分配
constexpr int kMaxPushFrames = 24576;
} // namespace

void TimeStretch::prepare(int channels, int sampleRate)
{
    m_channels = qBound(1, channels, 2);
    const int scale = qBound(1, sampleRate / 24000, 4);
    m_window = 1024 * scale;
    m_hopOut = m_window / 2;
    m_search = m_hopOut / 2;
    m_corr = m_window / 4;

    m_win.resize(size_t(m_window));
    for (int i = 0; i < m_window; ++i)
        m_win[size_t(i)] = 0.5f - 0.5f * std::cos(2.0 * M_PI * (i + 0.5) / m_window);

    m_lap.assign(size_t(m_window) * m_channels, 0.0f);
    m_prev.clear();
    m_inBuf.clear();
    m_outBuf.clear();
    setSpeed(m_speed);
    ensureCapacity();
}

void TimeStretch::reset()
{
    std::fill(m_lap.begin(), m_lap.end(), 0.0f);
    m_prev.clear();
    m_inBuf.clear();
    m_outBuf.clear();
    m_dropFrames = 0;
}

void TimeStretch::setSpeed(double speed)
{
    m_speed = qBound(0.25, speed, 4.0);
    m_hopIn = qMax(16, int(std::lround(m_hopOut * m_speed)));
    ensureCapacity();
}

// 按当前速度预留稳态最大驻留量
void TimeStretch::ensureCapacity()
{
    if (m_channels <= 0)
        return;
    const size_t inNeed = (size_t(guardFrames()) * 4 + size_t(kMaxPushFrames)) * size_t(m_channels);
    const size_t outNeed = size_t(guardFrames()) * 6 * size_t(m_channels);
    const size_t prevNeed = size_t(m_corr) * size_t(m_channels);
    if (m_inBuf.capacity() < inNeed)
        m_inBuf.reserve(inNeed);
    if (m_outBuf.capacity() < outNeed)
        m_outBuf.reserve(outNeed);
    if (m_prev.capacity() < prevNeed)
        m_prev.reserve(prevNeed);
}

int TimeStretch::guardFrames() const
{
    return m_hopIn + std::abs(m_hopOut - m_hopIn) + m_search + m_window;
}

void TimeStretch::push(const float *in, int frames)
{
    if (frames <= 0 || !in)
        return;

    // 每次 push 只回收一次已消费数据，避免每跳一步都搬移整个缓冲
    if (m_dropFrames > 0) {
        const long drop = long(m_dropFrames) * m_channels;
        m_inBuf.erase(m_inBuf.begin(), m_inBuf.begin() + drop);
        m_dropFrames = 0;
    }

    const size_t offset = m_inBuf.size();
    m_inBuf.resize(offset + size_t(frames) * m_channels);
    std::memcpy(m_inBuf.data() + offset, in, size_t(frames) * m_channels * sizeof(float));

    // 产出速度跟不上时截断，避免缓冲无限增长
    const int maxFrames = guardFrames() * 4;
    const int buffered = int(m_inBuf.size()) / m_channels;
    if (buffered > maxFrames) {
        const long drop = long(buffered - maxFrames) * m_channels;
        m_inBuf.erase(m_inBuf.begin(), m_inBuf.begin() + drop);
    }
    grind();
}

int TimeStretch::pull(float *out, int frames)
{
    grind();
    const int available = int(m_outBuf.size()) / m_channels;
    const int take = std::min(frames, available);
    if (take <= 0)
        return 0;
    std::memcpy(out, m_outBuf.data(), size_t(take) * m_channels * sizeof(float));
    m_outBuf.erase(m_outBuf.begin(), m_outBuf.begin() + long(take) * m_channels);
    return take;
}

void TimeStretch::grind()
{
    const int ch = m_channels;
    // 输入跳距与输出跳距的差必须先补掉，相关搜索只负责修正残余偏移
    const int center = m_hopOut - m_hopIn;
    const int reach = std::abs(center) + m_search;
    const int guard = m_hopIn + reach + m_window;
    const int total = int(m_inBuf.size()) / ch;
    // 从上次消费位置继续；push() 回收缓冲后会把 m_dropFrames 归零
    int pos = m_dropFrames;

    while (total - pos >= guard) {
        const float *data = m_inBuf.data() + size_t(pos) * ch;
        int lo = qMax(-m_hopIn, center - m_search);
        int hi = qMin(reach, center + m_search);
        if (hi < lo)
            lo = hi = qBound(-m_hopIn, center, reach);

        int best = lo;
        if (!m_prev.empty()) {
            double bestScore = -1e30;
            const int step = qMax(1, (hi - lo) / 128);
            auto scoreAt = [&](int d) {
                const float *cand = data + size_t(m_hopIn + d) * ch;
                double score = 0.0;
                for (int k = 0; k < m_corr; ++k) {
                    const float *a = m_prev.data() + size_t(k) * ch;
                    const float *b = cand + size_t(k) * ch;
                    for (int c = 0; c < ch; ++c)
                        score += double(a[c]) * double(b[c]);
                }
                return score;
            };
            for (int d = lo; d <= hi; d += step) {
                const double score = scoreAt(d);
                if (score > bestScore) {
                    bestScore = score;
                    best = d;
                }
            }
            const int fineLo = qMax(lo, best - step);
            const int fineHi = qMin(hi, best + step);
            for (int d = fineLo; d <= fineHi; ++d) {
                const double score = scoreAt(d);
                if (score > bestScore) {
                    bestScore = score;
                    best = d;
                }
            }
            best = qBound(lo, best, hi);
        }

        const float *seg = data + size_t(m_hopIn + best) * ch;
        for (int k = 0; k < m_window; ++k) {
            const float w = m_win[size_t(k)];
            float *dst = m_lap.data() + size_t(k) * ch;
            const float *src = seg + size_t(k) * ch;
            for (int c = 0; c < ch; ++c)
                dst[c] += w * src[c];
        }

        m_outBuf.insert(m_outBuf.end(), m_lap.begin(),
                        m_lap.begin() + long(m_hopOut) * ch);

        const int remain = m_window - m_hopOut;
        std::memmove(m_lap.data(), m_lap.data() + size_t(m_hopOut) * ch,
                     size_t(remain) * ch * sizeof(float));
        std::memset(m_lap.data() + size_t(remain) * ch, 0,
                    size_t(m_hopOut) * ch * sizeof(float));

        const size_t corrSamples = size_t(m_corr) * ch;
        if (m_prev.size() != corrSamples)
            m_prev.resize(corrSamples);
        std::memcpy(m_prev.data(), seg + size_t(m_hopOut) * ch,
                    corrSamples * sizeof(float));

        pos += m_hopIn;
    }
    m_dropFrames = pos;
}
