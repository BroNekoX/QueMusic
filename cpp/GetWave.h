// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef GETWAVE_H
#define GETWAVE_H

#include <QAtomicInteger>
#include <QObject>
#include <QAudioBuffer>
#include <QAudioBufferOutput>
#include <QMediaPlayer>
#include <QVector>
#include <QMutex>
#include <QImage>
#include <QtMath>
#include <complex>
#include <algorithm>
#include <QtQmlIntegration/qqmlintegration.h>

using Complex = std::complex<float>;

class GetWave : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QMediaPlayer* mediaPlayer READ mediaPlayer WRITE setMediaPlayer NOTIFY mediaPlayerChanged)
    Q_PROPERTY(QList<qreal> spectrumData READ spectrumData NOTIFY spectrumChanged)
    Q_PROPERTY(int bands READ bands WRITE setBands NOTIFY bandsChanged)
    Q_PROPERTY(QVector<QPointF> wavePath READ wavePath NOTIFY wavePathChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit GetWave(QObject *parent = nullptr);

    QMediaPlayer* mediaPlayer() const { return m_mediaPlayer; }
    void setMediaPlayer(QMediaPlayer *player);

    QList<qreal> spectrumData() const;
    QVector<QPointF> wavePath() const { return m_wavePath; }

    // 渲染帧回调：窗口每帧调用一次，有新数据才重算频谱
    Q_INVOKABLE void updateSpectrum();

    int bands() const { return m_bands; }
    void setBands(int b);
    bool enabled() const { return m_enabled; }
    void setEnabled(bool e);

signals:
    void mediaPlayerChanged();
    void spectrumChanged();
    void bandsChanged();
    void wavePathChanged();
    void enabledChanged();

private slots:
    void onBufferReceived(const QAudioBuffer &buffer);

private:
    void fft(QVector<Complex> &data);
    int nextPowerOfTwo(int n);
    void rebuildWavePath(int bands, qreal width, qreal height);

    void computeSpectrumFromFFT(const QVector<float> &samples, float sampleRate);

    QMediaPlayer       *m_mediaPlayer   = nullptr;
    QAudioBufferOutput *m_bufferOutput = nullptr;

    QList<qreal>        m_spectrumData;
    QVector<QPointF>    m_wavePath;
    QVector<float>      m_rawBuffer;
    mutable QMutex      m_mutex;

    int                 m_bands = 96;
    int                 m_fftSize = 4096;
    bool                m_enabled = true;

    qreal               m_smoothFactor = 0.6;

    // 复用缓冲区，避免每次分配
    QVector<Complex>    m_fftData;
    QVector<float>      m_magnitudes;

    // 帧驱动：窗口每帧触发 updateSpectrum()，有新数据才重算
    QAtomicInteger<int> m_dataReady = 0;
    QAtomicInteger<int> m_sampleRate = 48000;
};

#endif // GETWAVE_H