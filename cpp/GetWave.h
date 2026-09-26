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

    // 渲染帧回调：窗口每帧调用一次，有新数据才重算频谱
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
    // 与引擎共享的订阅句柄；析构时先 detach() 再让成员析构，音频线程才碰不到已销毁对象
    std::shared_ptr<AudioSpectrumSinkHandle> m_handle;

    QList<qreal>        m_spectrumData;
    QVector<QPointF>    m_wavePath;
    // 音频线程只写、渲染线程只读，全程无锁：音频回调绝不能等 GUI/渲染线程
    AudioRing           m_ring;
    std::vector<float>  m_mix;
    std::vector<float>  m_snapshot;
    // 仅用于渲染线程与 GUI 线程之间交换频谱结果，音频线程永不触碰
    mutable QMutex      m_visMutex;

    int                 m_bands = 96;
    int                 m_fftSize = 4096;
    bool                m_enabled = true;

    qreal               m_smoothFactor = 0.6;

    // 复用缓冲区，避免每次分配
    QVector<Complex>    m_fftData;
    QVector<float>      m_magnitudes;
    QVector<float>      m_samples;

    // 帧驱动：窗口每帧触发 updateSpectrum()，有新数据才重算
    QAtomicInteger<int> m_dataReady = 0;
    QAtomicInteger<int> m_sampleRate = 48000;

    QQuickWindow *m_renderWindow = nullptr;
    QMetaObject::Connection m_frameConnection;
};

#endif // GETWAVE_H