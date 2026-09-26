// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef GETWAVE_H
#define GETWAVE_H

#include "audio/AudioEngine.h"
#include "audio/AudioSpectrumSink.h"

#include <QMutex>
#include <QObject>
#include <QPointF>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QtQuick/QQuickWindow>

#include <atomic>
#include <complex>
#include <vector>

// 频谱条：音频回调线程把混音写进环形历史，渲染线程跟 vsync 逐帧取最近一窗做 FFT
class GetWave : public QObject, public AudioSpectrumSink
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(AudioEngine *engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(QQuickWindow *renderWindow READ renderWindow WRITE setRenderWindow NOTIFY renderWindowChanged)
    Q_PROPERTY(QVector<QPointF> wavePath READ wavePath NOTIFY wavePathChanged)
    Q_PROPERTY(int bands READ bands WRITE setBands NOTIFY bandsChanged)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit GetWave(QObject *parent = nullptr);

    AudioEngine *engine() const { return m_engine; }
    void setEngine(AudioEngine *engine);

    QQuickWindow *renderWindow() const { return m_renderWindow; }
    void setRenderWindow(QQuickWindow *window);

    QVector<QPointF> wavePath() const;

    int bands() const { return m_bands.load(std::memory_order_relaxed); }
    void setBands(int bands);

    bool enabled() const { return m_enabled.load(std::memory_order_relaxed); }
    void setEnabled(bool on);

    // 音频回调线程调用：只做降混与环形覆盖写，不取锁、不分配
    void pushSamples(const float *interleaved, int frames, int channels, int sampleRate) override;

signals:
    void engineChanged();
    void renderWindowChanged();
    void wavePathChanged();
    void bandsChanged();
    void enabledChanged();

private:
    static constexpr int kWindow = 4096; // 2 的幂：一次分析的采样窗口
    static constexpr int kMask = kWindow - 1;

    // 以下均在渲染线程（frameSwapped）执行
    void updateSpectrum();
    void fft(QVector<std::complex<float>> &data);
    void computeSpectrum(const float *samples, float sampleRate);
    void buildPath(QVector<QPointF> &out) const;

    AudioEngine *m_engine = nullptr;
    QQuickWindow *m_renderWindow = nullptr;
    QMetaObject::Connection m_frameConnection;

    std::vector<float> m_history;
    std::vector<float> m_snapshot;
    std::atomic<quint32> m_writePos{0};
    std::atomic<int> m_sampleRate{48000};
    quint32 m_lastPos = 0;

    QVector<std::complex<float>> m_fftData;
    QVector<float> m_magnitudes;
    QVector<qreal> m_spectrum;   // bands 个，镜像后的显示值
    QVector<qreal> m_bandsValue; // bands / 2 个，本窗原始值
    QVector<qreal> m_level;      // bands / 2 个，逐窗平滑后的电平

    // wavePath 渲染线程写、GUI 线程读，只在换手时短暂持锁
    mutable QMutex m_pathMutex;
    QVector<QPointF> m_wavePath;
    QVector<QPointF> m_pendingPath;

    std::atomic<int> m_bands{128};
    std::atomic<bool> m_enabled{false};
};

#endif // GETWAVE_H
