// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors

#include "LyricsStage.h"

#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGMaterialShader>
#include <QSGRendererInterface>

namespace {

// 偏移须与 shaders/stage.vert 的 buf 一致，布局表见 LyricsStage.h
constexpr int kStageUniformSize = 192;
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
        memcpy(p + 0, mat->colors, 32);
        memcpy(p + 32, mat->size, 8);
        memcpy(p + 48, mat->cam, 16);
        memcpy(p + 64, mat->ctrl, 16);
        memcpy(p + 80, mat->audio, 16);
        memcpy(p + 96, mat->misc, 16);
        return true;
    }
};

const QSGGeometry::AttributeSet &stageAttributes()
{
    // aId = (点序号, 角序号, 柔边标记)
    static const QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::create(0, 3, QSGGeometry::FloatType, true)
    };
    static const QSGGeometry::AttributeSet set = {1, 3 * int(sizeof(float)), attrs};
    return set;
}

QSGGeometry *makeStageGeometry(int count, bool soft)
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
            v[k++] = soft ? 1.0f : 0.0f;
        }
    }
    g->markVertexDataDirty();
    return g;
}

} // namespace

StageMaterial::StageMaterial()
{
    setFlag(Blending, true);        // 加色发光
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

void LyricsStage::setCamTheta(qreal v)
{
    if (qFuzzyCompare(m_cam[0], v))
        return;
    m_cam[0] = v;
    emit camChanged();
    update();
}

void LyricsStage::setCamPhi(qreal v)
{
    if (qFuzzyCompare(m_cam[1], v))
        return;
    m_cam[1] = v;
    emit camChanged();
    update();
}

void LyricsStage::setCamRadius(qreal v)
{
    if (qFuzzyCompare(m_cam[2], v))
        return;
    m_cam[2] = v;
    emit camChanged();
    update();
}

void LyricsStage::setCamFocal(qreal v)
{
    if (qFuzzyCompare(m_cam[3], v))
        return;
    m_cam[3] = v;
    emit camChanged();
    update();
}

void LyricsStage::setCamTargetY(qreal v)
{
    if (qFuzzyCompare(m_camTargetY, v))
        return;
    m_camTargetY = v;
    emit camChanged();
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
    m_audioLevel = v;
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
    const bool soft = (m_preset == 5);                   // 预设 5 = 前景浮尘
    if (!node->geometry() || m_geoCount != m_count || m_geoPreset != m_preset) {
        node->setGeometry(makeStageGeometry(m_count, soft));   // 旧几何由 OwnsGeometry 释放
        m_geoCount = m_count;
        m_geoPreset = m_preset;
    }

    auto *mat = static_cast<StageMaterial *>(node->material());
    mat->preset = m_preset;
    mat->count = m_count;
    mat->size[0] = float(width());
    mat->size[1] = float(height());
    for (int i = 0; i < 4; ++i)
        mat->cam[i] = float(m_cam[i]);

    const float c1[4] = {float(m_color1.redF()), float(m_color1.greenF()), float(m_color1.blueF()), 1.0f};
    const float c2[4] = {float(m_color2.redF()), float(m_color2.greenF()), float(m_color2.blueF()), 1.0f};
    memcpy(mat->colors, c1, 16);
    memcpy(mat->colors + 4, c2, 16);

    mat->ctrl[0] = float(m_camTargetY);
    mat->ctrl[1] = float(m_time);
    mat->ctrl[2] = float(m_preset);
    mat->ctrl[3] = float(m_audioGain);
    for (int i = 0; i < 4; ++i)
        mat->audio[i] = float(m_audio[i]);
    mat->misc[0] = float(m_audioLevel);
    mat->misc[1] = float(m_count);
    mat->misc[2] = float(m_density);
    mat->misc[3] = 0.0f;

    node->markDirty(QSGNode::DirtyMaterial);
    return node;
}