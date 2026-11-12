// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// SOLA 变速（保持音高）：可变输入跳距 + 相关搜索对齐 + Hann 交叠相加
#pragma once

#include <vector>

class TimeStretch
{
public:
    void prepare(int channels, int sampleRate);
    void reset();
    void setSpeed(double speed);
    double speed() const { return m_speed; }
    bool bypass() const { return m_speed == 1.0; }

    void push(const float *in, int frames);
    int pull(float *out, int frames);
    int guardFrames() const;

private:
    void grind();
    void ensureCapacity();

    int m_channels = 2;
    double m_speed = 1.0;
    int m_window = 2048;
    int m_hopOut = 1024;
    int m_hopIn = 1024;
    int m_search = 512;
    int m_corr = 512;

    int m_dropFrames = 0;   // 已消费但未回收的帧数
    std::vector<float> m_win;
    std::vector<float> m_lap;
    std::vector<float> m_prev;
    std::vector<float> m_inBuf;
    std::vector<float> m_outBuf;
};
