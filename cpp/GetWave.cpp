// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "GetWave.h"

#include "audio/AudioEngine.h"

#include <cmath>
#include <algorithm>

namespace {
constexpr int kRingFrames = 16384;   // 约 0.1~0.3 秒，只服务频谱显示
constexpr int kMaxPushFrames = 8192;

// 频谱显示参数
constexpr qreal kMinFreq = 30.0;   // 频段下限
constexpr qreal kDbFloor = -62.0;  // 满高约 -6 dB，留出余量不轻易削顶
constexpr qreal kDbRange = 56.0;
constexpr qreal kAttack  = 0.6;    // 上升跟随
constexpr qreal kRelease = 0.3;    // 下降缓释：柱子不逐帧乱抖
}

GetWave::GetWave(QObject *parent) : QObject(parent)
{
    // 句柄先于一切回调建立：音频线程只持有它，从不直接引用 this
    m_handle = std::make_shared<AudioSpectrumSinkHandle>(this);

    m_spectrumData.reserve(m_bands);
    for (int i = 0; i < m_bands; ++i)
        m_spectrumData.append(0.0);

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
    m_spectrumData.fill(0.0);
    m_wavePath.clear();
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
    // 必须先 detach()：它返回后才保证音频线程不再进入本对象，析构成员才安全
    if (m_handle)
        m_handle->detach();
    // 主动退订，不依赖 QObject::destroyed —— 那个信号发出时派生类成员已经析构
    if (m_engine)
        m_engine->setSpectrumSink(nullptr);
    if (m_renderWindow && m_frameConnection)
        disconnect(m_frameConnection);
}

// QML 读取频谱：与 updateSpectrum 同在 GUI 线程，无需加锁
QList<qreal> GetWave::spectrumData() const
{
    return m_spectrumData;
}

QVector<QPointF> GetWave::wavePath() const
{
    return m_wavePath;
}

// 音频线程调用：降混成单声道写入无锁环，不取锁、不分配
void GetWave::pushSamples(const float *interleaved, int frames, int channels, int sampleRate)
{
    if (!m_enabled || frames <= 0 || channels <= 0)
        return;

    const int n = qMin(frames, kMaxPushFrames);
    // 真降混：双声道取平均、多声道求平均。只取左声道时，相位相反的段落会互相抵消，
    // 波形会忽高忽低，看起来就是"抖"
    if (channels == 2) {
        for (int i = 0; i < n; ++i)
            m_mix[size_t(i)] = (interleaved[size_t(2 * i)] + interleaved[size_t(2 * i) + 1]) * 0.5f;
    } else if (channels == 1) {
        for (int i = 0; i < n; ++i)
            m_mix[size_t(i)] = interleaved[size_t(i)];
    } else {
        for (int i = 0; i < n; ++i) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c)
                sum += interleaved[size_t(i) * size_t(channels) + size_t(c)];
            m_mix[size_t(i)] = sum / float(channels);
        }
    }

    if (m_ring.space() < n)
        return;   // 消费端跟不上时丢弃本批，绝不阻塞音频线程
    m_ring.write(m_mix.data(), n);

    m_sampleRate.storeRelease(sampleRate);
    m_dataReady.storeRelease(1);
}

void GetWave::updateSpectrum()
{
    if (!m_enabled)
        return;
    if (!m_dataReady.loadAcquire())
        return;
    m_dataReady.storeRelease(0);

    // 一次取空环：既保证用的是最新数据，也不让生产端因积压而丢批
    const int got = m_ring.read(m_snapshot.data(), kRingFrames);
    if (got < 64)
        return;

    computeSpectrumFromFFT(m_snapshot.data(), got, float(m_sampleRate.loadAcquire()));
    rebuildWavePath(m_bands, 512.0, 80.0);

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
        // 用 afterAnimating：Qt 文档明确它是 GUI 线程信号、且可用于同步外部动画系统。
        // 比 frameSwapped 少一次队列投递，也不会在渲染线程 emit 触发 QML 绑定
        m_frameConnection = connect(m_renderWindow, &QQuickWindow::afterAnimating,
                                    this, &GetWave::updateSpectrum);
    emit renderWindowChanged();
}

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

// 从 PCM 计算对数分布频谱（复用缓冲，不分配）
void GetWave::computeSpectrumFromFFT(const float *samples, int n, float sampleRate)
{
    if (n < 64) return;

    const int fftN = m_fftSize;

    std::fill(m_fftData.begin(), m_fftData.end(), Complex(0.0f, 0.0f));

    // 汉宁窗铺满实际参与的样本；分母用 copyLen 而不是 n：
    // n 与真正参与变换的长度不一致时，每帧的窗形都不一样，频谱会跟着抖
    const int copyLen = std::min(n, fftN);
    float windowSum = 0.0f;
    for (int i = 0; i < copyLen; ++i) {
        const float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / float(copyLen - 1)));
        m_fftData[i] = Complex(samples[i] * window, 0.0f);
        windowSum += window;
    }

    fft(m_fftData);

    const int halfN = fftN / 2;
    for (int i = 0; i < halfN; ++i) {
        float re = m_fftData[i].real();
        float im = m_fftData[i].imag();
        m_magnitudes[i] = sqrtf(re * re + im * im) / (windowSum + 1e-9f);
    }

    // 对数频段划分：上下限只与采样率有关，对数在循环外算一次
    const float freqLow = float(kMinFreq);
    const float freqHigh = sampleRate * 0.48f;
    const float logLow  = logf(freqLow);
    const float logHigh = logf(freqHigh);

    const int halfBands = m_bands / 2;
    // 复用成员缓冲，别每帧分配（下面每项都会写满，无需清零）
    if (m_rawBands.size() != halfBands)
        m_rawBands.resize(halfBands);
    QVector<qreal> &rawBands = m_rawBands;

    for (int b = 0; b < halfBands; ++b) {
        const float t1 = b / qreal(halfBands);
        const float t2 = (b + 1) / qreal(halfBands);
        const float f1 = expf(logLow + (logHigh - logLow) * t1);
        const float f2 = expf(logLow + (logHigh - logLow) * t2);

        int bin1 = qMax(1, int(f1 * fftN / sampleRate));
        int bin2 = qMin(halfN - 1, int(f2 * fftN / sampleRate));
        if (bin2 <= bin1) bin2 = bin1 + 1;

        float sum = 0.0f;
        for (int k = bin1; k < bin2; ++k)
            sum += m_magnitudes[k];
        float avg = sum / (bin2 - bin1);

        const float dB = 20.0f * log10f(avg + 1e-6f);
        rawBands[b] = qBound(0.0, (qreal(dB) - kDbFloor) / kDbRange, 1.0);
    }

    // 相邻三点混合，削掉孤立尖峰：个别频段一跳一跳，整体就显得毛躁
    qreal prev = rawBands.at(0);
    for (int b = 1; b + 1 < halfBands; ++b) {
        const qreal cur = rawBands.at(b);
        rawBands[b] = (prev + 2.0 * cur + rawBands.at(b + 1)) * 0.25;
        prev = cur;
    }

    // 上升跟得快、下降放得慢，然后镜像到左右两侧。
    // 单系数平滑会让下降和上升一样快，柱子就逐帧乱抖
    for (int i = 0; i < halfBands; ++i) {
        const qreal value = rawBands.at(i);
        qreal &left  = m_spectrumData[halfBands - 1 - i];
        qreal &right = m_spectrumData[halfBands + i];
        left  += (value - left)  * (value > left  ? kAttack : kRelease);
        right += (value - right) * (value > right ? kAttack : kRelease);
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