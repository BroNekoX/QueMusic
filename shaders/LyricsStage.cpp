// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

#include "LyricsStage.h"

#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGMaterialShader>
#include <QSGRendererInterface>

namespace {

// std140 布局（须与 shaders/stage.vert 的 buf 完全一致）：
// qt_Matrix(64) + qt_Opacity(4) + 填充(12) = 80
// + vec4 uColor(16) + vec4 uColor2(16) + vec2 uSize(8) + vec2 uMouse(8)
// + float uTime(4) + float uLow..uLevel(20) + float uPreset(4)
// + float uAudio/uCount/uCamDist/uCamSens/uDensity(20) = 176
constexpr int kStageUniformSize = 176;
constexpr int kBase = 80;

class StageShader : public QSGMaterialShader
{
public:
    StageShader()
    {
        setShaderFileName(VertexStage, QStringLiteral(":/shaders/stage.vert.qsb"));
        setShaderFileName(FragmentStage, QStringLiteral(":/shaders/stage.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        auto *mat = static_cast<StageMaterial *>(newMaterial);
        QByteArray *buf = state.uniformData();
        Q_ASSERT(buf->size() >= kStageUniformSize);
        if (state.isMatrixDirty())
            memcpy(buf->data(), state.combinedMatrix().constData(), 64);
        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            memcpy(buf->data() + 64, &opacity, 4);
        }
        char *p = buf->data() + kBase;
        memcpy(p + 0, mat->colors, 32);        // uColor, uColor2
        memcpy(p + 32, mat->size, 8);          // uSize
        memcpy(p + 40, mat->mouse, 8);         // uMouse
        memcpy(p + 48, &mat->time, 4);         // uTime
        memcpy(p + 52, mat->audio, 20);        // uLow / uMid / uHigh / uAir / uLevel
        const float presetF = float(mat->preset);
        memcpy(p + 72, &presetF, 4);           // uPreset
        memcpy(p + 76, mat->params, 20);       // uAudio / uCount / uCamDist / uCamSens / uDensity
        return true;
    }
};

const QSGGeometry::AttributeSet &stageAttributes()
{
    // 单个属性：aId = (点序号, 角序号)
    static const QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true)
    };
    static const QSGGeometry::AttributeSet set = {1, 2 * int(sizeof(float)), attrs};
    return set;
}

QSGGeometry *makeStageGeometry(int count)
{
    auto *g = new QSGGeometry(stageAttributes(), count * 6);   // 每点 2 个三角形
    g->setDrawingMode(QSGGeometry::DrawTriangles);
    g->setVertexDataPattern(QSGGeometry::StaticPattern);
    float *v = reinterpret_cast<float *>(g->vertexData());
    int k = 0;
    for (int i = 0; i < count; ++i) {
        for (int c = 0; c < 6; ++c) {
            v[k++] = float(i);
            v[k++] = float(c);
        }
    }
    g->markVertexDataDirty();
    return g;
}

} // namespace

StageMaterial::StageMaterial()
{
    setFlag(Blending, true);        // 加色发光（片元输出 alpha=0 + 预乘颜色）
}

QSGMaterialType *StageMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *StageMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new StageShader;
}

int StageMaterial::compare(const QSGMaterial *other) const
{
    const auto *o = static_cast<const StageMaterial *>(other);
    if (preset != o->preset)
        return preset - o->preset;
    return count - o->count;
}

LyricsStage::LyricsStage(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

void LyricsStage::setPreset(int p)
{
    if (m_preset == p)
        return;
    m_preset = p;
    emit presetChanged();
    update();
}

void LyricsStage::setCount(int c)
{
    c = qMax(16, c);
    if (m_count == c)
        return;
    m_count = c;
    emit countChanged();
    update();
}

void LyricsStage::setTime(qreal t)
{
    if (qFuzzyCompare(m_time, t))
        return;
    m_time = t;
    update();
}

void LyricsStage::setMouseX(qreal v)
{
    if (qFuzzyCompare(m_mouseX, v))
        return;
    m_mouseX = v;
    update();
}

void LyricsStage::setMouseY(qreal v)
{
    if (qFuzzyCompare(m_mouseY, v))
        return;
    m_mouseY = v;
    update();
}

void LyricsStage::setAudioLow(qreal v)
{
    m_audio[0] = v;
    update();
}

void LyricsStage::setAudioMid(qreal v)
{
    m_audio[1] = v;
    update();
}

void LyricsStage::setAudioHigh(qreal v)
{
    m_audio[2] = v;
    update();
}

void LyricsStage::setAudioAir(qreal v)
{
    m_audio[3] = v;
    update();
}

void LyricsStage::setAudioLevel(qreal v)
{
    m_audio[4] = v;
    update();
}

void LyricsStage::setColor1(const QColor &c)
{
    if (m_color1 == c)
        return;
    m_color1 = c;
    emit color1Changed();
    update();
}

void LyricsStage::setColor2(const QColor &c)
{
    if (m_color2 == c)
        return;
    m_color2 = c;
    emit color2Changed();
    update();
}

void LyricsStage::setAudioGain(qreal v)
{
    m_audioGain = v;
    update();
}

void LyricsStage::setCamDist(qreal v)
{
    m_camDist = v;
    update();
}

void LyricsStage::setCamSens(qreal v)
{
    m_camSens = v;
    update();
}

void LyricsStage::setDensity(qreal v)
{
    m_density = v;
    update();
}

void LyricsStage::geometryChange(const QRectF &newG, const QRectF &oldG)
{
    QQuickItem::geometryChange(newG, oldG);
    if (newG.size() != oldG.size())
        update();
}

QSGNode *LyricsStage::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    auto *node = static_cast<QSGGeometryNode *>(old);
    if (!node) {
        node = new QSGGeometryNode;
        node->setFlag(QSGNode::OwnsGeometry, true);
        node->setMaterial(new StageMaterial);
        node->setFlag(QSGNode::OwnsMaterial, true);
    }
    if (!node->geometry() || m_geoCount != m_count) {
        node->setGeometry(makeStageGeometry(m_count));   // 旧几何由 OwnsGeometry 释放
        m_geoCount = m_count;
    }

    auto *mat = static_cast<StageMaterial *>(node->material());
    mat->preset = m_preset;
    mat->count = m_count;
    mat->time = float(m_time);
    mat->mouse[0] = float(m_mouseX);
    mat->mouse[1] = float(m_mouseY);
    mat->size[0] = float(width());
    mat->size[1] = float(height());
    for (int i = 0; i < 5; ++i)
        mat->audio[i] = float(m_audio[i]);

    const float c1[4] = {float(m_color1.redF()), float(m_color1.greenF()), float(m_color1.blueF()), 1.0f};
    const float c2[4] = {float(m_color2.redF()), float(m_color2.greenF()), float(m_color2.blueF()), 1.0f};
    memcpy(mat->colors, c1, 16);
    memcpy(mat->colors + 4, c2, 16);

    mat->params[0] = float(m_audioGain);
    mat->params[1] = float(m_count);
    mat->params[2] = float(m_camDist);
    mat->params[3] = float(m_camSens);
    mat->params[4] = float(m_density);

    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}
