// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef GETWAVE_H
#define GETWAVE_H

#include "audio/AudioEngine.h"
#include "audio/AudioRing.h"
#include "audio/AudioSpectrumSink.h"

#include <QAtomicInteger>
#include <QObject>
#include <QPointer>
#include <QVector>
#include <QImage>
#include <QtMath>
#include <algorithm>
#include <complex>
#include <memory>
#include <vector>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QtQuick/QQuickWindow>

using Complex = std::complex<float>;

class GetWave : public QObject, public AudioSpectrumSink, public AudioSpectrumSource
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(AudioEngine* engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(QList<qreal> spectrumData READ spectrumData NOTIFY spectrumChanged)
    Q_PROPERTY(int bands READ bands WRITE setBands NOTIFY bandsChanged)
    Q_PROPERTY(QVector<QPointF> wavePath READ wavePath NOTIFY wavePathChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QQuickWindow* renderWindow READ renderWindow WRITE setRenderWindow NOTIFY renderWindowChanged)

public:
    explicit GetWave(QObject *parent = nullptr);
    ~GetWave() override;

    AudioEngine* engine() const { return m_engine.data(); }
    void setEngine(AudioEngine *engine);

    // AudioSpectrumSource：把可跨线程安全使用的句柄交给引擎，引擎不认识本类型
    std::shared_ptr<AudioSpectrumSinkHandle> spectrumSinkHandle() override;

    // AudioSpectrumSink：只由句柄在持锁状态下转发，音频线程从不直接持有 this
    void pushSamples(const float *interleaved, int frames, int channels, int sampleRate) override;

    QList<qreal> spectrumData() const;
    QVector<QPointF> wavePath() const;

    // 帧回调（GUI 线程）：有新数据才重算频谱
    Q_INVOKABLE void updateSpectrum();

    int bands() const { return m_bands; }
    void setBands(int b);
    bool enabled() const { return m_enabled; }
    void setEnabled(bool e);

    QQuickWindow* renderWindow() const { return m_renderWindow; }
    void setRenderWindow(QQuickWindow *window);

signals:
    void engineChanged();
    void spectrumChanged();
    void bandsChanged();
    void wavePathChanged();
    void enabledChanged();
    void renderWindowChanged();

private:
    void fft(QVector<Complex> &data);
    void rebuildWavePath(int bands, qreal width, qreal height);

    void computeSpectrumFromFFT(const float *samples, int frames, float sampleRate);

    // QPointer：引擎可能先于本对象销毁，析构时不能再解引用裸指针
    QPointer<AudioEngine> m_engine;
    // 与引擎共享的订阅句柄（析构顺序见 ~GetWave）
    std::shared_ptr<AudioSpectrumSinkHandle> m_handle;

    // 频谱结果只在 GUI 线程读写（音频线程经 m_ring 单向投递），因此无需加锁
    QList<qreal>        m_spectrumData;
    QVector<QPointF>    m_wavePath;
    // 音频线程写、GUI 线程读的无锁环：音频回调绝不能等 GUI/渲染线程
    AudioRing           m_ring;
    std::vector<float>  m_mix;
    std::vector<float>  m_snapshot;

    int                 m_bands = 96;
    int                 m_fftSize = 4096;
    bool                m_enabled = true;

    // 复用缓冲区，避免每次分配
    QVector<Complex>    m_fftData;
    QVector<float>      m_magnitudes;
    QVector<qreal>      m_rawBands;

    QAtomicInteger<int> m_dataReady = 0;   // 音频线程置位，帧回调消费
    QAtomicInteger<int> m_sampleRate = 48000;

    QQuickWindow *m_renderWindow = nullptr;
    QMetaObject::Connection m_frameConnection;
};

#endif // GETWAVE_H