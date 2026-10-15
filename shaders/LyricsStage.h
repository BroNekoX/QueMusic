// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#ifndef LYRICSSTAGE_H
#define LYRICSSTAGE_H

#include <QColor>
#include <QQuickItem>
#include <QSGMaterial>
#include <QtQmlIntegration/qqmlintegration.h>

// std140 布局（须与 stage.vert 一致）。刻意只用 vec4 拼装，避开 vec3/vec2 的对齐陷阱：
//   [0]mat4 qt_Matrix [64]float qt_Opacity+12填充 [80]uColor [96]uColor2 [112]uSize+填充
//   [128]uCam(theta,phi,radius,focal) [144]uCtrl(targetY,time,preset,audioGain)
//   [160]uAudio(low,mid,high,air) [176]uMisc(level,count,density,-)   合计 192 字节
class StageMaterial : public QSGMaterial
{
public:
    StageMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;
    int compare(const QSGMaterial *other) const override;

    int preset = 0;
    int count = 14000;
    float size[2] = {1.0f, 1.0f};
    float cam[4] = {0.0f, 0.30f, 6.6f, 2.40f};       // theta / phi / radius / focal
    float ctrl[4] = {0.30f, 0.0f, 0.0f, 1.0f};       // targetY / time / preset / audioGain
    float audio[4] = {0.0f, 0.0f, 0.0f, 0.0f};       // Low / Mid / High / Air
    float misc[4] = {0.0f, 14000.0f, 1.0f, 0.0f};    // Level / Count / Density / -
    float colors[8] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
};

// 点云舞台：几万个发光点，位置/大小/亮度全部由 stage.vert 在 GPU 算出。
class LyricsStage : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int preset READ preset WRITE setPreset NOTIFY presetChanged FINAL)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged FINAL)
    Q_PROPERTY(qreal time READ time WRITE setTime NOTIFY timeChanged FINAL)
    Q_PROPERTY(qreal camTheta READ camTheta WRITE setCamTheta NOTIFY camChanged FINAL)
    Q_PROPERTY(qreal camPhi READ camPhi WRITE setCamPhi NOTIFY camChanged FINAL)
    Q_PROPERTY(qreal camRadius READ camRadius WRITE setCamRadius NOTIFY camChanged FINAL)
    Q_PROPERTY(qreal camFocal READ camFocal WRITE setCamFocal NOTIFY camChanged FINAL)
    Q_PROPERTY(qreal camTargetY READ camTargetY WRITE setCamTargetY NOTIFY camChanged FINAL)
    Q_PROPERTY(qreal audioLow READ audioLow WRITE setAudioLow NOTIFY audioLowChanged FINAL)
    Q_PROPERTY(qreal audioMid READ audioMid WRITE setAudioMid NOTIFY audioMidChanged FINAL)
    Q_PROPERTY(qreal audioHigh READ audioHigh WRITE setAudioHigh NOTIFY audioHighChanged FINAL)
    Q_PROPERTY(qreal audioAir READ audioAir WRITE setAudioAir NOTIFY audioAirChanged FINAL)
    Q_PROPERTY(qreal audioLevel READ audioLevel WRITE setAudioLevel NOTIFY audioLevelChanged FINAL)
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1 NOTIFY color1Changed FINAL)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2 NOTIFY color2Changed FINAL)
    Q_PROPERTY(qreal audioGain READ audioGain WRITE setAudioGain NOTIFY audioGainChanged FINAL)
    Q_PROPERTY(qreal density READ density WRITE setDensity NOTIFY densityChanged FINAL)

public:
    explicit LyricsStage(QQuickItem *parent = nullptr);

    int preset() const { return m_preset; }
    void setPreset(int p);
    int count() const { return m_count; }
    void setCount(int c);
    qreal time() const { return m_time; }
    void setTime(qreal t);

    qreal camTheta() const { return m_cam[0]; }
    qreal camPhi() const { return m_cam[1]; }
    qreal camRadius() const { return m_cam[2]; }
    qreal camFocal() const { return m_cam[3]; }
    void setCamTheta(qreal v);
    void setCamPhi(qreal v);
    void setCamRadius(qreal v);
    void setCamFocal(qreal v);
    qreal camTargetY() const { return m_camTargetY; }
    void setCamTargetY(qreal v);

    qreal audioLow() const { return m_audio[0]; }
    void setAudioLow(qreal v);
    qreal audioMid() const { return m_audio[1]; }
    void setAudioMid(qreal v);
    qreal audioHigh() const { return m_audio[2]; }
    void setAudioHigh(qreal v);
    qreal audioAir() const { return m_audio[3]; }
    void setAudioAir(qreal v);
    qreal audioLevel() const { return m_audioLevel; }
    void setAudioLevel(qreal v);

    QColor color1() const { return m_color1; }
    void setColor1(const QColor &c);
    QColor color2() const { return m_color2; }
    void setColor2(const QColor &c);

    qreal audioGain() const { return m_audioGain; }
    void setAudioGain(qreal v);
    qreal density() const { return m_density; }
    void setDensity(qreal v);

signals:
    void presetChanged();
    void countChanged();
    void timeChanged();
    void camChanged();
    void audioLowChanged();
    void audioMidChanged();
    void audioHighChanged();
    void audioAirChanged();
    void audioLevelChanged();
    void color1Changed();
    void color2Changed();
    void audioGainChanged();
    void densityChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newG, const QRectF &oldG) override;

private:
    int m_preset = 0;
    int m_count = 14000;
    int m_geoCount = -1;
    int m_geoPreset = -1;
    qreal m_time = 0.0;
    qreal m_cam[4] = {0.0, 0.30, 6.6, 2.40};         // theta / phi / radius / focal
    qreal m_camTargetY = 0.30;
    qreal m_audio[4] = {0.0, 0.0, 0.0, 0.0};         // low / mid / high / air
    qreal m_audioLevel = 0.0;
    qreal m_audioGain = 1.0;
    qreal m_density = 1.0;
    QColor m_color1 = QColor::fromString("#00ee66");
    QColor m_color2 = QColor::fromString("#00b1ee");
};

#endif // LYRICSSTAGE_H