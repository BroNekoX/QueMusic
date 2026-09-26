// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "GetWave.h"

#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {

using Complex = std::complex<float>;

constexpr double kWaveWidth = 512.0; // 与 waveItem 尺寸一致，路径按像素直出
constexpr double kWaveHeight = 80.0;
constexpr double kMinFreq = 30.0;
constexpr double kDbFloor = -62.0; // 满高约在 -6dB，留出余量不轻易削顶
constexpr double kDbRange = 56.0;
constexpr double kAttack = 0.6;   // 上升跟随
constexpr double kRelease = 0.3; // 下降缓释：柱子不逐帧乱抖

} // namespace

GetWave::GetWave(QObject *parent) : QObject(parent)
{
    m_history.assign(kWindow, 0.0f);
    m_snapshot.assign(kWindow, 0.0f);
    m_fftData.resize(kWindow);
    m_magnitudes.resize(kWindow / 2);
    m_spectrum.fill(0.0, m_bands.load());
    m_bandsValue.fill(0.0, m_bands.load() / 2);
    m_level.fill(0.0, m_bands.load() / 2);
}

void GetWave::setEngine(AudioEngine *engine)
{
    if (m_engine == engine)
        return;
    if (m_engine)
        m_engine->setSpectrumSink(nullptr);
    m_engine = engine;
    if (m_engine)
        m_engine->setSpectrumSink(this);
    emit engineChanged();
}

void GetWave::setRenderWindow(QQuickWindow *window)
{
    if (m_renderWindow == window)
        return;
    if (m_frameConnection)
        disconnect(m_frameConnection);
    m_renderWindow = window;
    if (m_renderWindow)
        m_frameConnection = connect(m_renderWindow, &QQuickWindow::frameSwapped,
                                    this, &GetWave::updateSpectrum, Qt::DirectConnection);
    emit renderWindowChanged();
}

void GetWave::setBands(int bands)
{
    bands = qBound(16, bands + (bands & 1), 512); // 取偶数：频段要左右镜像
    if (m_bands.load() == bands)
        return;
    {
        QMutexLocker lock(&m_pathMutex);
        m_bands.store(bands);
        m_spectrum.fill(0.0, bands);
        m_bandsValue.fill(0.0, bands / 2);
        m_level.fill(0.0, bands / 2);
    }
    emit bandsChanged();
}

void GetWave::setEnabled(bool on)
{
    if (enabled() == on)
        return;
    m_enabled.store(on);
    if (!on) {
        {
            QMutexLocker lock(&m_pathMutex);
            m_wavePath.clear();
        }
        emit wavePathChanged();
    }
    emit enabledChanged();
}

QVector<QPointF> GetWave::wavePath() const
{
    QMutexLocker lock(&m_pathMutex);
    return m_wavePath;
}

void GetWave::pushSamples(const float *interleaved, int frames, int channels, int sampleRate)
{
    if (!enabled() || frames <= 0 || channels <= 0)
        return;
    if (frames > kWindow) {
        interleaved += size_t(frames - kWindow) * size_t(channels);
        frames = kWindow;
    }

    const quint32 pos = m_writePos.load(std::memory_order_relaxed);
    float *dst = m_history.data();
    if (channels == 2) {
        for (int i = 0; i < frames; ++i)
            dst[(pos + quint32(i)) & kMask] = (interleaved[2 * i] + interleaved[2 * i + 1]) * 0.5f;
    } else if (channels == 1) {
        for (int i = 0; i < frames; ++i)
            dst[(pos + quint32(i)) & kMask] = interleaved[i];
    } else {
        for (int i = 0; i < frames; ++i) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c)
                sum += interleaved[size_t(i) * size_t(channels) + size_t(c)];
            dst[(pos + quint32(i)) & kMask] = sum / float(channels);
        }
    }
    m_writePos.store(pos + quint32(frames), std::memory_order_release);
    m_sampleRate.store(sampleRate, std::memory_order_relaxed);
}

void GetWave::updateSpectrum()
{
    if (!enabled())
        return;
    const quint32 pos = m_writePos.load(std::memory_order_acquire);
    if (pos == m_lastPos)
        return;
    m_lastPos = pos;

    for (int i = 0; i < kWindow; ++i)
        m_snapshot[i] = m_history[(pos - quint32(kWindow) + quint32(i)) & kMask];

    computeSpectrum(m_snapshot.data(), float(m_sampleRate.load(std::memory_order_relaxed)));
    buildPath(m_pendingPath);
    {
        QMutexLocker lock(&m_pathMutex);
        m_wavePath.swap(m_pendingPath);
    }
    emit wavePathChanged();
}

void GetWave::fft(QVector<Complex> &data)
{
    const int n = data.size();
    if (n <= 1)
        return;

    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(data[i], data[j]);
    }

    for (int len = 2; len <= n; len <<= 1) {
        const float angle = -2.0f * float(M_PI) / float(len);
        const Complex wlen(std::cos(angle), std::sin(angle));
        const int half = len >> 1;
        for (int i = 0; i < n; i += len) {
            Complex w(1.0f, 0.0f);
            for (int k = 0; k < half; ++k) {
                const Complex u = data[i + k];
                const Complex v = data[i + k + half] * w;
                data[i + k] = u + v;
                data[i + k + half] = u - v;
                w *= wlen;
            }
        }
    }
}

void GetWave::computeSpectrum(const float *samples, float sampleRate)
{
    std::fill(m_fftData.begin(), m_fftData.end(), Complex(0.0f, 0.0f));

    float windowSum = 0.0f;
    for (int i = 0; i < kWindow; ++i) {
        const float w = 0.5f * (1.0f - std::cos(2.0f * float(M_PI) * float(i) / float(kWindow - 1)));
        m_fftData[i] = Complex(samples[i] * w, 0.0f);
        windowSum += w;
    }

    fft(m_fftData);

    constexpr int kHalf = kWindow / 2;
    for (int i = 0; i < kHalf; ++i) {
        const float re = m_fftData[i].real();
        const float im = m_fftData[i].imag();
        m_magnitudes[i] = std::sqrt(re * re + im * im) / (windowSum + 1e-9f);
    }

    const int halfBands = m_bands.load() / 2;
    // 频段边界按比例递推，省掉每段两次 exp
    const float binStep = std::exp((std::log(sampleRate * 0.48f) - std::log(float(kMinFreq)))
                                   / float(halfBands));
    float edge = std::exp(std::log(float(kMinFreq))) * kWindow / sampleRate;
    for (int b = 0; b < halfBands; ++b) {
        const float next = edge * binStep;
        const int bin1 = qMax(1, int(edge));
        int bin2 = qMin(kHalf - 1, int(next));
        if (bin2 <= bin1)
            bin2 = bin1 + 1;

        float sum = 0.0f;
        for (int k = bin1; k < bin2; ++k)
            sum += m_magnitudes[k];
        const float db = 20.0f * std::log10(sum / float(bin2 - bin1) + 1e-6f);
        m_bandsValue[b] = qBound(0.0, (double(db) - kDbFloor) / kDbRange, 1.0);
        edge = next;
    }

    // 相邻段三点混合，削掉孤立尖峰
    qreal prev = m_bandsValue.at(0);
    for (int b = 1; b + 1 < halfBands; ++b) {
        const qreal cur = m_bandsValue.at(b);
        m_bandsValue[b] = (prev + 2.0 * cur + m_bandsValue.at(b + 1)) * 0.25;
        prev = cur;
    }

    // 上升快、下降慢
    for (int b = 0; b < halfBands; ++b) {
        qreal &level = m_level[b];
        const qreal value = m_bandsValue.at(b);
        level += (value - level) * (value > level ? kAttack : kRelease);
    }

    // 低频居中、向两侧镜像
    for (int i = 0; i < halfBands; ++i) {
        const qreal value = m_level.at(i);
        m_spectrum[halfBands - 1 - i] = value;
        m_spectrum[halfBands + i] = value;
    }
}

void GetWave::buildPath(QVector<QPointF> &out) const
{
    const int bands = m_bands.load();
    const qreal step = kWaveWidth / (bands - 1);
    out.resize(bands + 2);
    out[0] = QPointF(0.0, kWaveHeight);
    for (int i = 0; i + 1 < bands; ++i) {
        const qreal value = (m_spectrum.at(i) + m_spectrum.at(i + 1)) * 0.5;
        out[i + 1] = QPointF((qreal(i) + 0.5) * step, kWaveHeight * (1.0 - value));
    }
    out[bands] = QPointF(kWaveWidth, kWaveHeight);
    out[bands + 1] = QPointF(0.0, kWaveHeight);
}
