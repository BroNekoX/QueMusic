// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
#include "GetWave.h"
#include <QDebug>
#include <cmath>
#include <algorithm>

// 构造 / 析构
GetWave::GetWave(QObject *parent) : QObject(parent)
{
    m_spectrumData.reserve(m_bands);
    for (int i = 0; i < m_bands; ++i)
        m_spectrumData.append(0.0);

    // 预分配FFT缓冲区，固定大小4096
    m_fftData.resize(m_fftSize);
    m_magnitudes.resize(m_fftSize / 2);
}

GetWave::~GetWave()
{
    // bufferOutput 的 parent 是 this，自动释放
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
    QMutexLocker locker(&m_mutex);
    m_rawBuffer.clear();
    m_spectrumData.fill(0.0);
    m_wavePath.clear();

    emit spectrumChanged();
    emit wavePathChanged();
}

void GetWave::setMediaPlayer(QMediaPlayer *player)
{
    if (m_mediaPlayer == player) return;

    if (m_mediaPlayer && m_bufferOutput) {
        disconnect(m_bufferOutput, &QAudioBufferOutput::audioBufferReceived,
                   this, &GetWave::onBufferReceived);
    }

    m_mediaPlayer = player;
    emit mediaPlayerChanged();

    if (!m_mediaPlayer) return;

    if (m_bufferOutput) {
        m_bufferOutput->deleteLater();
        m_bufferOutput = nullptr;
    }

    m_bufferOutput = new QAudioBufferOutput(this);
    m_mediaPlayer->setAudioBufferOutput(m_bufferOutput);

    connect(m_bufferOutput, &QAudioBufferOutput::audioBufferReceived,
            this, &GetWave::onBufferReceived, Qt::DirectConnection);
}

// QML 读取频谱
QList<qreal> GetWave::spectrumData() const
{
    QMutexLocker locker(&m_mutex);
    return m_spectrumData;
}

// 收到buffer事件动作
void GetWave::onBufferReceived(const QAudioBuffer &buffer)
{
    if (!buffer.isValid()) return;
    if (!m_enabled) return;

    const QAudioFormat &fmt = buffer.format();
    const int sampleRate = fmt.sampleRate();

    // ---- 第一阶段：快速追加数据，并复制一份当前完整样本（加锁） ----
    QVector<float> samplesCopy;
    {
        QMutexLocker locker(&m_mutex);

        // 取第一个声道/通常左声道的数据
        if (fmt.sampleFormat() == QAudioFormat::Float) {
            const float *data = buffer.constData<float>();
            int frames = buffer.sampleCount() / fmt.channelCount();
            m_rawBuffer.reserve(m_rawBuffer.size() + frames);
            for (int i = 0; i < frames; ++i)
                m_rawBuffer.append(data[i * fmt.channelCount()]);
        } else if (fmt.sampleFormat() == QAudioFormat::Int16) {
            const qint16 *data = buffer.constData<qint16>();
            int frames = buffer.sampleCount() / fmt.channelCount();
            m_rawBuffer.reserve(m_rawBuffer.size() + frames);
            for (int i = 0; i < frames; ++i)
                m_rawBuffer.append(data[i * fmt.channelCount()] / 32768.0f);
        }

        // 保留约 0.1 秒的数据
        int maxSamples = sampleRate * 0.1;
        if (m_rawBuffer.size() > maxSamples) {
            m_rawBuffer.remove(0, m_rawBuffer.size() - maxSamples);
        }

        // 复制当前数据用于异步处理（避免后续锁竞争）
        samplesCopy = m_rawBuffer;
    } // 锁释放，音频线程不再阻塞

    // ---- 第二阶段：异步进行FFT和频谱计算 ----
    QtConcurrent::run([this, samplesCopy, sampleRate]() {
        // 加锁保护共享成员 m_spectrumData / m_wavePath
        {
            QMutexLocker locker(&m_mutex);
            computeSpectrumFromFFT(samplesCopy, sampleRate);
            rebuildWavePath(m_bands, 512.0, 80.0);
        } // 锁释放，再发送信号（减少锁持有）
        emit spectrumChanged();
        emit wavePathChanged();
    });
}

// 以下为FFT实现模块
int GetWave::nextPowerOfTwo(int n)
{
    int p = 1;
    while (p < n) p <<= 1;
    return p;
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

// 核心：从 PCM 数据计算对数分布频谱（使用复用的成员缓冲区）
void GetWave::computeSpectrumFromFFT(const QVector<float> &samples, float sampleRate)
{
    int n = samples.size();
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

    // 平滑 + 镜像输出（更新 m_spectrumData，调用者已加锁）
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