// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#ifndef LYRICSSTAGE_H
#define LYRICSSTAGE_H

#include <QColor>
#include <QQuickItem>
#include <QSGMaterial>
#include <QtQmlIntegration/qqmlintegration.h>

// 点云舞台的材质：所有 uniform 由 updateUniformData 写进 std140 缓冲（布局须与 stage.vert 一致）
class StageMaterial : public QSGMaterial
{
public:
    StageMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;
    int compare(const QSGMaterial *other) const override;

    int preset = 0;
    int count = 14000;
    float time = 0.0f;
    float mouse[2] = {0.0f, 0.0f};
    float size[2] = {1.0f, 1.0f};
    float audio[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};   // Low/Mid/High/Air/Level
    float colors[8] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    float params[5] = {1.0f, 14000.0f, 9.0f, 1.0f, 1.0f}; // Audio/Count/CamDist/CamSens/Density
};

// 3d 歌词主题的点云舞台：几万个发光点，位置/大小/亮度全部由 stage.vert 在 GPU 算出。
// 与背景着色器共用同一套相机 ⇒ 点与方块场在同一个三维空间里。
class LyricsStage : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int preset READ preset WRITE setPreset NOTIFY presetChanged)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
    Q_PROPERTY(qreal time READ time WRITE setTime NOTIFY timeChanged)
    Q_PROPERTY(qreal mouseX READ mouseX WRITE setMouseX NOTIFY mouseXChanged)
    Q_PROPERTY(qreal mouseY READ mouseY WRITE setMouseY NOTIFY mouseYChanged)
    Q_PROPERTY(qreal audioLow READ audioLow WRITE setAudioLow NOTIFY audioLowChanged)
    Q_PROPERTY(qreal audioMid READ audioMid WRITE setAudioMid NOTIFY audioMidChanged)
    Q_PROPERTY(qreal audioHigh READ audioHigh WRITE setAudioHigh NOTIFY audioHighChanged)
    Q_PROPERTY(qreal audioAir READ audioAir WRITE setAudioAir NOTIFY audioAirChanged)
    Q_PROPERTY(qreal audioLevel READ audioLevel WRITE setAudioLevel NOTIFY audioLevelChanged)
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1 NOTIFY color1Changed)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2 NOTIFY color2Changed)
    Q_PROPERTY(qreal audioGain READ audioGain WRITE setAudioGain NOTIFY audioGainChanged)
    Q_PROPERTY(qreal camDist READ camDist WRITE setCamDist NOTIFY camDistChanged)
    Q_PROPERTY(qreal camSens READ camSens WRITE setCamSens NOTIFY camSensChanged)
    Q_PROPERTY(qreal density READ density WRITE setDensity NOTIFY densityChanged)

public:
    explicit LyricsStage(QQuickItem *parent = nullptr);

    int preset() const { return m_preset; }
    void setPreset(int p);
    int count() const { return m_count; }
    void setCount(int c);
    qreal time() const { return m_time; }
    void setTime(qreal t);
    qreal mouseX() const { return m_mouseX; }
    void setMouseX(qreal v);
    qreal mouseY() const { return m_mouseY; }
    void setMouseY(qreal v);

    qreal audioLow() const { return m_audio[0]; }
    void setAudioLow(qreal v);
    qreal audioMid() const { return m_audio[1]; }
    void setAudioMid(qreal v);
    qreal audioHigh() const { return m_audio[2]; }
    void setAudioHigh(qreal v);
    qreal audioAir() const { return m_audio[3]; }
    void setAudioAir(qreal v);
    qreal audioLevel() const { return m_audio[4]; }
    void setAudioLevel(qreal v);

    QColor color1() const { return m_color1; }
    void setColor1(const QColor &c);
    QColor color2() const { return m_color2; }
    void setColor2(const QColor &c);

    qreal audioGain() const { return m_audioGain; }
    void setAudioGain(qreal v);
    qreal camDist() const { return m_camDist; }
    void setCamDist(qreal v);
    qreal camSens() const { return m_camSens; }
    void setCamSens(qreal v);
    qreal density() const { return m_density; }
    void setDensity(qreal v);

signals:
    void presetChanged();
    void countChanged();
    void timeChanged();
    void mouseXChanged();
    void mouseYChanged();
    void audioLowChanged();
    void audioMidChanged();
    void audioHighChanged();
    void audioAirChanged();
    void audioLevelChanged();
    void color1Changed();
    void color2Changed();
    void audioGainChanged();
    void camDistChanged();
    void camSensChanged();
    void densityChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) override;
    void geometryChange(const QRectF &newG, const QRectF &oldG) override;

private:
    int m_preset = 0;
    int m_count = 14000;
    int m_geoCount = -1;
    qreal m_time = 0.0;
    qreal m_mouseX = 0.0;
    qreal m_mouseY = 0.0;
    qreal m_audio[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
    qreal m_audioGain = 1.0;
    qreal m_camDist = 9.0;
    qreal m_camSens = 1.0;
    qreal m_density = 1.0;
    QColor m_color1 = QColor::fromString("#00ee66");
    QColor m_color2 = QColor::fromString("#00b1ee");
};

#endif // LYRICSSTAGE_H
