// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "GetWave.h"

#include "audio/AudioEngine.h"

#include <QDebug>
#include <cmath>
#include <algorithm>

namespace {
constexpr int kRingFrames = 16384;   // 约 0.1~0.3 秒，只服务频谱显示
constexpr int kMaxPushFrames = 8192;
}

// 构造 / 析构
GetWave::GetWave(QObject *parent) : QObject(parent)
{
    // 句柄先于一切回调建立：音频线程只持有它，从不直接引用 this
    m_handle = std::make_shared<AudioSpectrumSinkHandle>(this);

    m_spectrumData.reserve(m_bands);
    for (int i = 0; i < m_bands; ++i)
        m_spectrumData.append(0.0);

    // 预分配 FFT 缓冲区，固定大小 4096
    m_fftData.resize(m_fftSize);
    m_magnitudes.resize(m_fftSize / 2);
    m_ring.configure(kRingFrames, 1);
    m_mix.resize(kMaxPushFrames);
    m_snapshot.resize(kRingFrames);
}

void GetWave::setBands(int b)
{
    if (b < 4) b = 4;
    if (b % 2 != 0) b += 1;
    if (m_bands != b) {
        m_bands = b;
        m_spectrumData.clear();
        m_spectrumData.reserve(m_bands);
        for (int i = 0; i < m_bands; ++i)
            m_spectrumData.append(0.0);
        emit bandsChanged();
    }
}

void GetWave::setEnabled(bool e)
{
    m_enabled = e;
    emit enabledChanged();

    m_dataReady.storeRelease(0);

    {
        QMutexLocker locker(&m_visMutex);
        m_spectrumData.fill(0.0);
        m_wavePath.clear();
    }
    emit spectrumChanged();
    emit wavePathChanged();
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

std::shared_ptr<AudioSpectrumSinkHandle> GetWave::spectrumSinkHandle()
{
    return m_handle;
}

GetWave::~GetWave()
{
    // 顺序很重要：detach() 会等到音频线程退出本对象后才返回，
    // 之后析构 m_visMutex / 环缓冲 / 频谱数组才不会被音频线程踩到
    if (m_handle)
        m_handle->detach();
    // 主动退订，不依赖 QObject::destroyed —— 那个信号发出时派生类成员已经析构
    if (m_engine)
        m_engine->setSpectrumSink(nullptr);
    if (m_renderWindow && m_frameConnection)
        disconnect(m_frameConnection);
}

// QML 读取频谱
QList<qreal> GetWave::spectrumData() const
{
    QMutexLocker locker(&m_visMutex);
    return m_spectrumData;
}

QVector<QPointF> GetWave::wavePath() const
{
    QMutexLocker locker(&m_visMutex);
    return m_wavePath;
}

// 音频线程调用：降混到单声道写入无锁环，不取任何锁
void GetWave::pushSamples(const float *interleaved, int frames, int channels, int sampleRate)
{
    if (!m_enabled || frames <= 0 || channels <= 0)
        return;

    const int n = qMin(frames, kMaxPushFrames);
    for (int i = 0; i < n; ++i)
        m_mix[size_t(i)] = interleaved[size_t(i) * channels];

    if (m_ring.space() < n)
        return;   // 渲染线程跟不上时丢弃本批，绝不阻塞音频线程
    m_ring.write(m_mix.data(), n);

    m_sampleRate.storeRelease(sampleRate);
    m_dataReady.storeRelease(1);
}

// 渲染线程帧回调：每帧一次，有新数据才重算频谱，跟随窗口刷新率
void GetWave::updateSpectrum()
{
    if (!m_enabled)
        return;
    if (!m_dataReady.loadAcquire())
        return;
    m_dataReady.storeRelease(0);

    const int got = m_ring.read(m_snapshot.data(), kRingFrames);
    if (got < 64)
        return;

    {
        QMutexLocker locker(&m_visMutex);
        computeSpectrumFromFFT(m_snapshot.data(), got, float(m_sampleRate.loadAcquire()));
        rebuildWavePath(m_bands, 512.0, 80.0);
    }
    emit spectrumChanged();
    emit wavePathChanged();
}

void GetWave::setRenderWindow(QQuickWindow *window)
{
    if (m_renderWindow == window) return;
    if (m_renderWindow && m_frameConnection)
        disconnect(m_frameConnection);
    m_renderWindow = window;
    if (m_renderWindow)
        m_frameConnection = connect(m_renderWindow, &QQuickWindow::frameSwapped,
                                    this, &GetWave::updateSpectrum, Qt::DirectConnection);
    emit renderWindowChanged();
}

// 以下为FFT实现模块
void GetWave::fft(QVector<Complex> &data)
{
    int n = data.size();
    if (n <= 1) return;

    // 位反转重排
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap(data[i], data[j]);
    }

    // Cooley-Tukey 蝶形运算
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0f * M_PI / len;
        Complex wlen(cosf(angle), sinf(angle));
        for (int i = 0; i < n; i += len) {
            Complex w(1.0f, 0.0f);
            int half = len >> 1;
            for (int k = 0; k < half; ++k) {
                Complex u = data[i + k];
                Complex v = data[i + k + half] * w;
                data[i + k] = u + v;
                data[i + k + half] = u - v;
                w *= wlen;
            }
        }
    }
}

// 核心：从 PCM 数据计算对数分布频谱（使用复用的成员缓冲区）
void GetWave::computeSpectrumFromFFT(const float *samples, int n, float sampleRate)
{
    if (n < 64) return;

    int fftN = m_fftSize;   // 固定大小

    // 清空FFT输入（其余位置填零）
    std::fill(m_fftData.begin(), m_fftData.end(), Complex(0.0f, 0.0f));

    // 加汉宁窗，填充到 m_fftData
    float windowSum = 0.0f;
    int copyLen = std::min(n, fftN);
    for (int i = 0; i < copyLen; ++i) {
        float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (n - 1)));  // Hanning
        m_fftData[i] = Complex(samples[i] * window, 0.0f);
        windowSum += window;
    }

    // 执行 FFT
    fft(m_fftData);

    // 计算各频点幅值（正频率部分）
    int halfN = fftN / 2;
    for (int i = 0; i < halfN; ++i) {
        float re = m_fftData[i].real();
        float im = m_fftData[i].imag();
        m_magnitudes[i] = sqrtf(re * re + im * im) / (windowSum + 1e-9f);
    }

    // 对数频段划分
    float freqLow = 30.0f;
    float freqHigh = sampleRate * 0.48f;
    if (freqHigh > sampleRate * 0.49f) freqHigh = sampleRate * 0.48f;

    int halfBands = m_bands / 2;
    QVector<qreal> rawBands(halfBands, 0.0);

    for (int b = 0; b < halfBands; ++b) {
        float logLow  = logf(freqLow);
        float logHigh = logf(freqHigh);
        float t1 = (b)     / qreal(halfBands);
        float t2 = (b + 1) / qreal(halfBands);
        float f1 = expf(logLow + (logHigh - logLow) * t1);
        float f2 = expf(logLow + (logHigh - logLow) * t2);

        int bin1 = qMax(1, int(f1 * fftN / sampleRate));
        int bin2 = qMin(halfN - 1, int(f2 * fftN / sampleRate));
        if (bin2 <= bin1) bin2 = bin1 + 1;

        float sum = 0.0f;
        for (int k = bin1; k < bin2; ++k)
            sum += m_magnitudes[k];
        float avg = sum / (bin2 - bin1);

        float dB = 20.0f * log10f(avg + 1e-6f);
        float scaled = (dB + 50.0f) / 45.0f;
        rawBands[b] = qBound(0.0, scaled, 1.0);
    }

    // 平滑 + 镜像输出
    for (int i = 0; i < halfBands; ++i) {
        int leftIdx  = halfBands - 1 - i;
        int rightIdx = halfBands + i;
        qreal val = rawBands[i];

        m_spectrumData[leftIdx]  = m_spectrumData[leftIdx]  * (1.0 - m_smoothFactor)
                                  + val * m_smoothFactor;
        m_spectrumData[rightIdx] = m_spectrumData[rightIdx] * (1.0 - m_smoothFactor)
                                   + val * m_smoothFactor;
    }
}

void GetWave::rebuildWavePath(int bands, qreal width, qreal height)
{
    if (bands < 0 || width <= 0 || height <= 0) return;

    qreal barW = width / (bands - 1);

    m_wavePath.clear();
    m_wavePath.reserve(bands + 3);

    m_wavePath.append(QPointF(0, height));

    for (int i = 0; i < (bands - 1); ++i) {
        qreal x = (i + 0.5) * barW;
        qreal valueData = m_spectrumData[i] / 2 + m_spectrumData[i + 1] / 2;
        qreal y = height - valueData * height;
        m_wavePath.append(QPointF(x, y));
    }

    m_wavePath.append(QPointF(width, height));
    m_wavePath.append(QPointF(0, height));
}