// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2024-2026 QueMusic Contributors
// Copyright (c) 2022-2024 AMLL Contributors
//
// MeshGradientItem.h
//
// 本文件是 AMLL (Apple Music Like Lyrics) 的 Mesh Gradient 背景渲染器的
// C++/Qt 移植与改写版本，衍生自以下 AGPL-3.0 原始实现：
//   https://github.com/amll-dev/applemusic-like-lyrics
//   - src/renderer/mesh/MeshGradientRenderer.ts （渲染主逻辑）
//   - src/renderer/mesh/bhpmesh.ts （Bicubic Hermite Patch Mesh）
//   - src/renderer/mesh/cp-presets.ts （控制点预设）
//   - src/renderer/mesh/cp-generate.ts （控制点生成与平滑）
//
// 采用 Qt Quick Scene Graph 自定义材质（QSGMaterial + QSGGeometryNode）方案，
// 与 AMLL 的 Bicubic Hermite Patch Mesh 效果保持一致。
//
// 主要修改内容：
//   - 将 TypeScript 控制点/网格数据结构改写为 C++ 结构体与 QVector
//   - 新增 coverUrl/volume/flowSpeed/animating/subDivisions 等 QML 可绑定属性
//   - 材质改为 QSGMaterial 体系，统一由 Qt RHI 渲染
//
// 依据 GNU Affero General Public License v3.0 发布，全文见：
//   https://www.gnu.org/licenses/agpl-3.0.txt
#ifndef MESHGRADIENTITEM_H
#define MESHGRADIENTITEM_H

#include <QQuickItem>
#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QSGGeometryNode>
#include <QSGTexture>
#include <QSGGeometry>
#include <QElapsedTimer>
#include <QTimer>
#include <QImage>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QVector>
#include <QPointer>

// ---------------------------------------------------------------------------
// 控制点配置（对应 AMLL cp-presets.ts 的 ControlPointConf / ControlPointPreset）
// ---------------------------------------------------------------------------
struct ControlPointConf
{
    int cx = 0;
    int cy = 0;
    float x = 0.0f;    // 位置 x（NDC，-1..1）
    float y = 0.0f;    // 位置 y（NDC，-1..1）
    float ur = 0.0f;   // u 切向旋转角（度）
    float vr = 0.0f;   // v 切向旋转角（度）
    float up = 1.0f;   // u 切向缩放
    float vp = 1.0f;   // v 切向缩放
};

struct ControlPointPreset
{
    int width = 0;
    int height = 0;
    QVector<ControlPointConf> conf;
};

// ---------------------------------------------------------------------------
// 材质（渲染线程使用）
// ---------------------------------------------------------------------------
class MeshGradientMaterial : public QSGMaterial
{
public:
    MeshGradientMaterial()
    {
        setFlag(Blending, true);
    }

    QSGMaterialType *type() const override
    {
        static QSGMaterialType type;
        return &type;
    }

    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;

    int compare(const QSGMaterial *other) const override
    {
        const auto *o = static_cast<const MeshGradientMaterial *>(other);
        if (texture != o->texture)
            return texture < o->texture ? -1 : 1;
        if (!qFuzzyCompare(alpha, o->alpha))
            return alpha < o->alpha ? -1 : 1;
        if (!qFuzzyCompare(volume, o->volume))
            return volume < o->volume ? -1 : 1;
        return 0;
    }

    // 每帧更新的 Uniform 数据
    QSGTexture *texture = nullptr;
    float aspect   = 1.0f;
    float volume   = 0.0f;
    float alpha    = 1.0f;
    float sinAngle = 0.0f;
    float cosAngle = 1.0f;
    float time     = 0.0f;
};

// ---------------------------------------------------------------------------
// 渲染状态（对应 AMLL 的 MeshState）
// ---------------------------------------------------------------------------
struct MeshState
{
    QSGGeometryNode *node = nullptr;
    MeshGradientMaterial *material = nullptr;
    QSGTexture *texture = nullptr;
    float alpha = 0.0f;   // 淡入淡出过渡用
};

// ---------------------------------------------------------------------------
// 网格渐变渲染 Item
// ---------------------------------------------------------------------------
class MeshGradientItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QUrl coverUrl READ coverUrl WRITE setCoverUrl NOTIFY coverUrlChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qreal flowSpeed READ flowSpeed WRITE setFlowSpeed NOTIFY flowSpeedChanged)
    Q_PROPERTY(bool animating READ animating WRITE setAnimating NOTIFY animatingChanged)
    Q_PROPERTY(int subDivisions READ subDivisions WRITE setSubDivisions NOTIFY subDivisionsChanged)
    // 网格渐变主色（对应 AMLL 控制点颜色，QML 绑定封面的 coverColor）
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1 NOTIFY color1Changed)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2 NOTIFY color2Changed)
    Q_PROPERTY(QColor color3 READ color3 WRITE setColor3 NOTIFY color3Changed)

public:
    explicit MeshGradientItem(QQuickItem *parent = nullptr);
    ~MeshGradientItem() override;

    QUrl coverUrl() const { return m_coverUrl; }
    void setCoverUrl(const QUrl &url);

    qreal volume() const { return m_volume; }
    void setVolume(qreal v);

    qreal flowSpeed() const { return m_flowSpeed; }
    void setFlowSpeed(qreal s);

    bool animating() const { return m_animating; }
    void setAnimating(bool a);

    int subDivisions() const { return m_subDivisions; }
    void setSubDivisions(int s);

    QColor color1() const { return m_color1; }
    void setColor1(const QColor &c);
    QColor color2() const { return m_color2; }
    void setColor2(const QColor &c);
    QColor color3() const { return m_color3; }
    void setColor3(const QColor &c);

signals:
    void coverUrlChanged();
    void volumeChanged();
    void flowSpeedChanged();
    void animatingChanged();
    void subDivisionsChanged();
    void color1Changed();
    void color2Changed();
    void color3Changed();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void releaseResources() override;

private:
    void loadCover();
    void handleCoverDownloaded(const QByteArray &data, bool fromNetwork);
    void onCoverProcessed(const QImage &processed);

    // -- 控制点 / 网格生成 --
    static ControlPointPreset choosePreset();
    static ControlPointPreset generateControlPoints(int width, int height);
    static void smoothifyControlPoints(QVector<ControlPointConf> &conf, int w, int h,
                                       int iterations, float factor, float factorIterationModifier);
    QSGGeometry *buildGeometry(const ControlPointPreset &preset, float width, float height);

    // -- 封面处理 --
    static QImage processCoverImage(const QImage &src);
    static void blurImage(QImage &img, int radius, int iterations);

    // -- 渲染辅助 --
    void createMeshState(const QImage &processed);
    void destroyMeshState(MeshState *state);
    void syncAlpha(qint64 deltaMs);

    QUrl m_coverUrl;
    qreal m_volume = 0.0;
    qreal m_smoothedVolume = 0.0;
    qreal m_flowSpeed = 1.0;
    bool m_animating = true;
    int m_subDivisions = 24;
    QColor m_color1 = QColor(0, 238, 102);   // 对应 QueMusic 默认主色
    QColor m_color2 = QColor(0, 177, 238);   // 对应 QueMusic 默认辅色
    QColor m_color3 = QColor(157, 78, 221);  // 对应 QueMusic 默认第三色

    QVector<MeshState *> m_states;      // 渲染线程状态（在 updatePaintNode 同步点访问）
    QSGNode *m_root = nullptr;          // 场景图根容器（子节点为各 mesh）

    QElapsedTimer m_timer;
    qint64 m_lastFrameTime = -1;
    qreal m_time = 0.0;
    bool m_timerStarted = false;

    QNetworkAccessManager *m_net = nullptr;
    QPointer<QQuickWindow> m_window;

    QTimer *m_animTimer = nullptr;      // 持续动画驱动（每 16ms 请求重绘）
    bool m_hasCover = false;
    bool m_pendingCover = false;        // 有待处理的封面
    QImage m_pendingImage;              // 已处理完成的封面
    QImage m_lastImage;                 // 最近一次成功处理的封面（尺寸变化时重建用）
};

#endif // MESHGRADIENTITEM_H
