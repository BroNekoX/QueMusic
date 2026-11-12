// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "PitchShifter.h"

#include <QtGlobal>

#include <cmath>
#include <cstring>

void PitchShifter::prepare(int channels, int sampleRate)
{
    m_channels = qBound(1, channels, 2);
    m_stretch.prepare(m_channels, sampleRate);
    m_buf.clear();
    m_buf.reserve(size_t(16384) * size_t(m_channels));
    m_offset = 0;
    m_pos = 0.0;
    setRatios(m_speed, m_pitch);
}

void PitchShifter::reset()
{
    m_stretch.reset();
    m_buf.clear();
    m_offset = 0;
    m_pos = 0.0;
}

void PitchShifter::setRatios(double speed, double pitch)
{
    m_speed = qBound(0.25, speed, 4.0);
    m_pitch = qBound(0.25, pitch, 4.0);
    m_stretch.setSpeed(m_speed / m_pitch);
}

void PitchShifter::push(const float *in, int frames)
{
    m_stretch.push(in, frames);
}

int PitchShifter::pull(float *out, int frames)
{
    const int ch = m_channels;
    int produced = 0;

    while (produced < frames) {
        const int total = int(m_buf.size()) / ch;
        const double available = double(total - m_offset) - m_pos;
        if (available < 4.0) {
            if (m_offset > 0 && total > 4096) {
                // m_pos 本就是相对 m_offset 的小数偏移，压实后只把基准归零
                m_buf.erase(m_buf.begin(), m_buf.begin() + long(m_offset) * ch);
                m_offset = 0;
            }
            const int need = frames - produced + 8;
            const size_t old = m_buf.size();
            m_buf.resize(old + size_t(need) * ch);
            const int got = m_stretch.pull(m_buf.data() + old, need);
            m_buf.resize(old + size_t(got) * ch);
            if (got <= 0)
                break;
            continue;
        }

        const int maxIndex = int(m_buf.size()) / ch - 3;
        const int base = qBound(1, m_offset + int(m_pos), qMax(1, maxIndex));
        const double frac = m_pos - std::floor(m_pos);
        for (int c = 0; c < ch; ++c) {
            const float *p = m_buf.data() + size_t(base) * ch + c;
            const double y0 = double(p[-int(ch)]);
            const double y1 = double(p[0]);
            const double y2 = double(p[ch]);
            const double y3 = double(p[2 * ch]);
            const double a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
            const double a1 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
            const double a2 = -0.5 * y0 + 0.5 * y2;
            const double v = ((a0 * frac + a1) * frac + a2) * frac + y1;
            out[size_t(produced) * ch + c] = float(qBound(-1.0, v, 1.0));
        }

        m_pos += m_pitch;
        ++produced;
        if (m_pos >= 1.0) {
            const int whole = int(m_pos);
            m_offset += whole;
            m_pos -= whole;
        }
    }
    return produced;
}
