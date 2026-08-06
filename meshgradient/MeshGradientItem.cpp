// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2024-2026 QueMusic Contributors
// Copyright (c) 2022-2024 AMLL Contributors
//
// MeshGradientItem.cpp
//
// 本文件是 AMLL (Apple Music Like Lyrics) 的 Mesh Gradient 背景渲染器的
// C++/Qt 移植与改写版本，衍生自以下 AGPL-3.0 原始实现：
//   https://github.com/amll-dev/applemusic-like-lyrics
//   - src/renderer/mesh/MeshGradientRenderer.ts （渲染主逻辑）
//   - src/renderer/mesh/mesh.vert.glsl / mesh.frag.glsl （着色器）
//   - src/renderer/mesh/bhpmesh.ts （Bicubic Hermite Patch Mesh）
//   - src/renderer/mesh/cp-presets.ts （控制点预设）
//   - src/renderer/mesh/cp-generate.ts （控制点生成与平滑）
//
// 主要修改内容：
//   - 将 TypeScript/Web 渲染器移植为 Qt Quick Scene Graph 自定义材质
//     （QSGMaterial + QSGGeometryNode），适配 Qt 6 / C++17
//   - 封面网络/本地加载与 32x32 后处理（对比度/饱和度/亮度/模糊）以 Qt 实现
//   - 以 QTimer 驱动动画循环替代 requestAnimationFrame
//   - 着色器改写为 GLSL 440，经 qsb 编译供 Qt RHI（D3D11/OpenGL 等）使用
//   - 修复：纹理改用 Linear 过滤 + ClampToEdge，消除放大马赛克
//   - 修复：补调 commitTextureOperations()，保证纹理数据上传 GPU
//   - 移除音量（volume）对渲染的硬依赖，适配 QueMusic 播放器场景
//
// 依据 GNU Affero General Public License v3.0 发布，全文见：
//   https://www.gnu.org/licenses/agpl-3.0.txt
#include "MeshGradientItem.h"

#include <QQuickWindow>
#include <QQuickItem>
#include <QSGRendererInterface>
#include <QNetworkReply>
#include <QPainter>
#include <QRadialGradient>
#include <QtConcurrent>
#include <QRandomGenerator>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

// 着色器（对应 AMLL 的 mesh.vert.glsl / mesh.frag.glsl）
namespace {

class MeshGradientShader : public QSGMaterialShader
{
public:
    MeshGradientShader()
    {
        setShaderFileName(VertexStage, QStringLiteral(":/shaders/meshgradient/shaders/meshgradient.vert.qsb"));
        setShaderFileName(FragmentStage, QStringLiteral(":/shaders/meshgradient/shaders/meshgradient.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override
    {
        QByteArray *buf = state.uniformData();
        Q_ASSERT(buf->size() >= 92);

        // mat4 qt_Matrix (0..64)
        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data(), m.constData(), 64);
        }
        // float qt_Opacity (64)
        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            memcpy(buf->data() + 64, &opacity, 4);
        }
        // 自定义 Uniform（每帧随动画/音量变化，无条件更新）：
        // u_aspect(68) u_volume(72) u_alpha(76) u_sinAngle(80) u_cosAngle(84) u_time(88)
        auto *mat = static_cast<MeshGradientMaterial *>(newMaterial);
        memcpy(buf->data() + 68, &mat->aspect, 4);
        memcpy(buf->data() + 72, &mat->volume, 4);
        memcpy(buf->data() + 76, &mat->alpha, 4);
        memcpy(buf->data() + 80, &mat->sinAngle, 4);
        memcpy(buf->data() + 84, &mat->cosAngle, 4);
        memcpy(buf->data() + 88, &mat->time, 4);
        return true;
    }

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *) override
    {
        auto *mat = static_cast<MeshGradientMaterial *>(newMaterial);
        if (binding == 1 && mat->texture) {
            *texture = mat->texture;
            mat->texture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
        }
    }
};

} // namespace

QSGMaterialShader *MeshGradientMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new MeshGradientShader;
}

// 封面图像处理（移植自 AMLL setAlbum：32x32 + 对比度/饱和度/亮度 + 模糊）
void MeshGradientItem::blurImage(QImage &img, int radius, int iterations)
{
    if (img.isNull() || radius <= 0)
        return;
    const int w = img.width();
    const int h = img.height();
    QImage tmp = img;
    QImage tmp2;

    for (int it = 0; it < iterations; ++it) {
        // 水平方向
        for (int y = 0; y < h; ++y) {
            const uchar *src = img.constScanLine(y);
            uchar *dst = tmp.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int r = 0, g = 0, b = 0, a = 0, cnt = 0;
                for (int k = -radius; k <= radius; ++k) {
                    const int xx = qBound(0, x + k, w - 1);
                    const uchar *p = src + xx * 4;
                    r += p[0]; g += p[1]; b += p[2]; a += p[3];
                    ++cnt;
                }
                uchar *d = dst + x * 4;
                d[0] = uchar(r / cnt);
                d[1] = uchar(g / cnt);
                d[2] = uchar(b / cnt);
                d[3] = uchar(a / cnt);
            }
        }
        // 垂直方向
        tmp2 = tmp;
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                int r = 0, g = 0, b = 0, a = 0, cnt = 0;
                for (int k = -radius; k <= radius; ++k) {
                    const int yy = qBound(0, y + k, h - 1);
                    const uchar *p = tmp.constScanLine(yy) + x * 4;
                    r += p[0]; g += p[1]; b += p[2]; a += p[3];
                    ++cnt;
                }
                uchar *d = tmp2.scanLine(y) + x * 4;
                d[0] = uchar(r / cnt);
                d[1] = uchar(g / cnt);
                d[2] = uchar(b / cnt);
                d[3] = uchar(a / cnt);
            }
        }
        img = tmp2;
        tmp = img;
    }
}

QImage MeshGradientItem::processCoverImage(const QImage &src)
{
    if (src.isNull())
        return QImage();
    QImage img = src.convertToFormat(QImage::Format_RGBA8888);
    // 与 AMLL 原版一致：32x32，模糊后颜色分布最接近原版
    img = img.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // 合并对比度/饱和度/亮度的处理（对应 AMLL 的像素循环）
    for (int y = 0; y < img.height(); ++y) {
        uchar *line = img.scanLine(y);
        for (int x = 0; x < img.width(); ++x) {
            uchar *p = line + x * 4;
            float r = p[0];
            float g = p[1];
            float b = p[2];

            // contrast 0.4
            r = (r - 128.0f) * 0.4f + 128.0f;
            g = (g - 128.0f) * 0.4f + 128.0f;
            b = (b - 128.0f) * 0.4f + 128.0f;

            // saturate 3.0
            const float gray = r * 0.3f + g * 0.59f + b * 0.11f;
            r = gray * -2.0f + r * 3.0f;
            g = gray * -2.0f + g * 3.0f;
            b = gray * -2.0f + b * 3.0f;

            // contrast 1.7
            r = (r - 128.0f) * 1.7f + 128.0f;
            g = (g - 128.0f) * 1.7f + 128.0f;
            b = (b - 128.0f) * 1.7f + 128.0f;

            // brightness 0.75
            r *= 0.75f;
            g *= 0.75f;
            b *= 0.75f;

            p[0] = uchar(qBound(0, int(r + 0.5f), 255));
            p[1] = uchar(qBound(0, int(g + 0.5f), 255));
            p[2] = uchar(qBound(0, int(b + 0.5f), 255));
        }
    }

    blurImage(img, 2, 4);
    return img;
}

// 默认兜底渐变图：用三个主题色绘制 32x32 彩色渐变。
// 当 coverUrl 为空/无效时作为 MeshGradient 纹理使用，保证背景永不透明。
QImage MeshGradientItem::defaultGradientImage(const QColor &c1, const QColor &c2, const QColor &c3)
{
    QImage img(32, 32, QImage::Format_RGBA8888);
    img.fill(Qt::black);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    // 三个径向光斑叠加，模拟封面处理后的柔和色彩分布
    QRadialGradient g1(10, 8, 20);
    g1.setColorAt(0.0, c1);
    g1.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(0, 0, 32, 32, g1);

    QRadialGradient g2(24, 22, 18);
    g2.setColorAt(0.0, c2);
    g2.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(0, 0, 32, 32, g2);

    QRadialGradient g3(16, 28, 14);
    g3.setColorAt(0.0, c3);
    g3.setColorAt(1.0, QColor(0, 0, 0, 0));
    p.fillRect(0, 0, 32, 32, g3);

    p.end();

    // 轻微模糊让光斑融合更自然
    blurImage(img, 2, 2);
    return img;
}

// MeshGradientItem 实现（网格生成 buildGeometry 见文件末尾）
MeshGradientItem::MeshGradientItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    m_net = new QNetworkAccessManager(this);

    // 持续动画驱动：QTimer 每 33ms 请求重绘，确保动画循环稳定运行
    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(33);
    connect(m_animTimer, &QTimer::timeout, this, &MeshGradientItem::update);
    if (m_animating)
        m_animTimer->start();
}

MeshGradientItem::~MeshGradientItem()
{
    // QSG 节点由场景图在 item 销毁时回收；这里仅释放纹理与状态外壳
    for (MeshState *s : std::as_const(m_states)) {
        if (s->texture)
            s->texture->deleteLater();
        delete s;
    }
    m_states.clear();
    m_root = nullptr;
}

void MeshGradientItem::setCoverUrl(const QUrl &url)
{
    if (m_coverUrl == url)
        return;
    m_coverUrl = url;
    emit coverUrlChanged();
    loadCover();
}

void MeshGradientItem::setVolume(qreal v)
{
    if (qFuzzyCompare(m_volume, v))
        return;
    m_volume = v;
    emit volumeChanged();
}

void MeshGradientItem::setFlowSpeed(qreal s)
{
    if (qFuzzyCompare(m_flowSpeed, s))
        return;
    m_flowSpeed = s;
    emit flowSpeedChanged();
}

void MeshGradientItem::setAnimating(bool a)
{
    if (m_animating == a)
        return;
    m_animating = a;
    emit animatingChanged();
    if (m_animTimer) {
        if (m_animating)
            m_animTimer->start();
        else
            m_animTimer->stop();
    }
    update();
}

void MeshGradientItem::setSubDivisions(int s)
{
    const int v = qBound(2, s, 64);
    if (m_subDivisions == v)
        return;
    m_subDivisions = v;
    emit subDivisionsChanged();
    update();
}

void MeshGradientItem::setColor1(const QColor &c)
{
    if (m_color1 == c)
        return;
    m_color1 = c;
    emit color1Changed();
    update();
}

void MeshGradientItem::setColor2(const QColor &c)
{
    if (m_color2 == c)
        return;
    m_color2 = c;
    emit color2Changed();
    update();
}

void MeshGradientItem::setColor3(const QColor &c)
{
    if (m_color3 == c)
        return;
    m_color3 = c;
    emit color3Changed();
    update();
}

// 封面加载
void MeshGradientItem::loadCover()
{
    const QUrl url = m_coverUrl;
    if (url.isEmpty()) {
        // 无封面/无歌曲：用主题色渐变兜底，保证背景不透明。
        // 直接走处理完成路径，等价于"已就绪的默认封面"。
        const QImage fallback = MeshGradientItem::defaultGradientImage(m_color1, m_color2, m_color3);
        if (!fallback.isNull()) {
            m_lastImage = fallback;
            m_pendingImage = fallback;
            m_pendingCover = true;
            m_hasCover = true;
            update();
            return;
        }
        m_hasCover = false;
        m_pendingCover = false;
        update();
        return;
    }

    if (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https")) {
        QNetworkRequest req(url);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *reply = m_net->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                m_hasCover = false;
                m_pendingCover = false;
                update();
                return;
            }
            handleCoverDownloaded(reply->readAll(), true);
        });
    } else if (url.scheme() == QLatin1String("data")) {
        // data URI：ColorExtractor 已调用 processCoverImage 处理好的 32x32 渲染图，
        // 直接解码作为最终纹理使用，不再走下载/后台处理链路（避免二次处理）。
        // 32x32 PNG 解码为微秒级，同步执行即可。
        const QString s = url.toString();
        const int comma = s.indexOf(QLatin1Char(','));
        if (comma >= 0) {
            const QByteArray payload = s.mid(comma + 1).toLatin1();
            const QByteArray raw = QByteArray::fromBase64(payload);
            QImage img;
            img.loadFromData(raw);
            if (!img.isNull()) {
                onCoverProcessed(img);
                return;
            }
        }
        m_hasCover = false;
        m_pendingCover = false;
        update();
    } else {
        QString path;
        if (url.scheme() == QLatin1String("qrc"))
            path = QStringLiteral(":") + url.path();
        else
            path = url.toLocalFile();
        handleCoverDownloaded(path.toUtf8(), false);
    }
}

void MeshGradientItem::handleCoverDownloaded(const QByteArray &data, bool fromNetwork)
{
    QPointer<MeshGradientItem> self(this);
    QtConcurrent::run([self, data, fromNetwork]() {
        QImage raw;
        if (fromNetwork)
            raw.loadFromData(data);
        else
            raw.load(QString::fromUtf8(data));
        const QImage processed = MeshGradientItem::processCoverImage(raw);
        if (!self)
            return;
        QMetaObject::invokeMethod(self, [self, processed]() {
            if (self)
                self->onCoverProcessed(processed);
        }, Qt::QueuedConnection);
    });
}

void MeshGradientItem::onCoverProcessed(const QImage &processed)
{
    if (processed.isNull()) {
        m_hasCover = false;
        m_pendingCover = false;
    } else {
        m_lastImage = processed;   // 保存最近封面，供尺寸变化时重建
        m_pendingImage = processed;
        m_pendingCover = true;
        m_hasCover = true;
    }
    update();
}

// 渲染状态管理
void MeshGradientItem::createMeshState(const QImage &processed)
{
    if (!m_window || !m_root || processed.isNull())
        return;

    // 需要有效尺寸：几何顶点要转换为 item 本地坐标
    const qreal iw = boundingRect().width();
    const qreal ih = boundingRect().height();
    if (iw <= 1.0 || ih <= 1.0)
        return;

    QSGTexture *tex = m_window->createTextureFromImage(processed);
    if (!tex)
        return;
    // 线性过滤：32x32 纹理放大到屏幕时平滑过渡，避免纹素硬块
    tex->setFiltering(QSGTexture::Linear);
    // MirroredRepeat 会镜像重复纹理 -> 每个镜像单元渐变方向相反（"每块对角反向"马赛克）。
    // ClampToEdge 让超出部分钳制到边缘，渐变方向一致。
    tex->setHorizontalWrapMode(QSGTexture::ClampToEdge);
    tex->setVerticalWrapMode(QSGTexture::ClampToEdge);

    const ControlPointPreset preset = choosePreset();
    QSGGeometry *geo = buildGeometry(preset, float(iw), float(ih));
    if (!geo)
        return;

    auto *node = new QSGGeometryNode;
    node->setGeometry(geo);
    node->setFlag(QSGNode::OwnsGeometry);

    auto *mat = new MeshGradientMaterial;
    mat->texture = tex;
    node->setMaterial(mat);
    node->setFlag(QSGNode::OwnsMaterial);

    auto *state = new MeshState;
    state->node = node;
    state->material = mat;
    state->texture = tex;
    // 第一个 mesh 直接完全显示（alpha=1），后续切歌的 mesh 才从 0 淡入，
    // 避免 animating=false（静态模式）时永远透明
    state->alpha = m_states.isEmpty() ? 1.0f : 0.0f;
    m_states.append(state);
    m_root->appendChildNode(node);
}

void MeshGradientItem::destroyMeshState(MeshState *state)
{
    if (!state)
        return;
    if (state->node) {
        if (m_root)
            m_root->removeChildNode(state->node);
        delete state->node; // OwnsGeometry / OwnsMaterial 一并释放
    }
    if (state->texture)
        state->texture->deleteLater();
    delete state;
}

// 对应 AMLL onRedraw 中的 alpha 过渡逻辑
void MeshGradientItem::syncAlpha(qint64 deltaMs)
{
    const float deltaFactor = float(deltaMs) / 500.0f;

    if (!m_hasCover) {
        // 无封面：全部淡出
        for (int i = m_states.size() - 1; i >= 0; --i) {
            MeshState *s = m_states[i];
            s->alpha = qMax(-0.1f, s->alpha - deltaFactor);
            if (s->alpha <= -0.1f) {
                destroyMeshState(s);
                m_states.remove(i);
            }
        }
    } else if (!m_states.isEmpty()) {
        MeshState *latest = m_states.last();
        if (latest->alpha >= 1.1f) {
            // 新的已完全淡入，清理旧状态
            while (m_states.size() > 1)
                destroyMeshState(m_states.takeFirst());
        } else {
            latest->alpha = qMin(1.1f, latest->alpha + deltaFactor);
        }
    }
}

// 场景图同步
QSGNode *MeshGradientItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode;
    if (!root)
        root = new QSGNode;
    m_root = root;

    QQuickWindow *w = window();
    if (!w)
        return root;
    m_window = w;

    // 时间推进（对应 AMLL onTick）
    if (!m_timerStarted) {
        m_timer.start();
        m_timerStarted = true;
        m_lastFrameTime = -1;
    }
    const qint64 now = m_timer.elapsed();
    qreal delta = (m_lastFrameTime < 0) ? 16.0 : qreal(now - m_lastFrameTime);
    if (delta > 100.0)
        delta = 100.0; // 防止窗口隐藏/切后台后帧时间跳变
    m_lastFrameTime = now;
    if (m_animating)
        m_time += delta * m_flowSpeed;

    // 音量平滑（对应 AMLL smoothedVolume)
    const qreal lerp = qMin(1.0, delta / 100.0);
    m_smoothedVolume += (m_volume - m_smoothedVolume) * lerp;

    // 新封面就绪 -> 创建新网格状态
    if (m_pendingCover) {
        createMeshState(m_pendingImage);
        m_pendingCover = false;
        m_pendingImage = QImage();
    }

    // alpha 过渡
    syncAlpha(qint64(delta));

    // 更新材质 uniform（对应 AMLL onRedraw 的 uniform 设置）
    const float aspect = (boundingRect().height() > 1.0)
                             ? float(boundingRect().width() / boundingRect().height()) : 1.0f;
    // 慢速旋转（与原版接近）：angle = (time/10000 + volume) * 2
    const float uTime = float(m_time / 1000.0);
    const float angle = (uTime * 0.1f + float(m_smoothedVolume)) * 2.0f;
    const float sinA = std::sin(angle);
    const float cosA = std::cos(angle);
    const float vol = float(m_smoothedVolume);
    const float secs = uTime; // 秒，供 shader 做 sin 波流动

    for (MeshState *s : std::as_const(m_states)) {
        MeshGradientMaterial *mat = s->material;
        mat->aspect = aspect;
        mat->volume = vol;
        mat->alpha = s->alpha;
        mat->sinAngle = sinA;
        mat->cosAngle = cosA;
        mat->time = secs;
        s->node->markDirty(QSGNode::DirtyMaterial);
    }

    // 持续动画驱动：请求下一帧
    if (m_animating && !m_states.isEmpty())
        w->update();

    return root;
}

void MeshGradientItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    // 尺寸变化时：网格几何基于 item 尺寸生成，需要重建
    // 清空旧 states（含节点/纹理），用保存的封面图重建，避免"只渲染一块、其他透明"
    const bool sizeChanged = !qFuzzyCompare(newGeometry.width(), oldGeometry.width())
                             || !qFuzzyCompare(newGeometry.height(), oldGeometry.height());
    if (sizeChanged) {
        for (MeshState *s : std::as_const(m_states)) {
            if (s->node && m_root)
                m_root->removeChildNode(s->node);
            delete s->node;
            if (s->texture)
                s->texture->deleteLater();
            delete s;
        }
        m_states.clear();
        if (m_hasCover && !m_lastImage.isNull()) {
            m_pendingImage = m_lastImage;
            m_pendingCover = true;
        }
    }
    update();
}

void MeshGradientItem::releaseResources()
{
    // 文档说明 releaseResources 在 GUI 线程调用，不能直接删除渲染线程对象；
    // node 由场景图在 item 移除时自动回收，这里只释放纹理与状态外壳
    for (MeshState *s : std::as_const(m_states)) {
        if (s->texture)
            s->texture->deleteLater();
        delete s;
    }
    m_states.clear();
    m_root = nullptr;
}

// 控制点预设与数学工具（匿名命名空间）
namespace {

ControlPointConf cp(int cx, int cy, float x, float y,
                    float ur = 0.0f, float vr = 0.0f,
                    float up = 1.0f, float vp = 1.0f)
{
    ControlPointConf c;
    c.cx = cx; c.cy = cy;
    c.x = x; c.y = y;
    c.ur = ur; c.vr = vr;
    c.up = up; c.vp = vp;
    return c;
}

ControlPointPreset makePreset(int w, int h, const QVector<ControlPointConf> &conf)
{
    ControlPointPreset p;
    p.width = w;
    p.height = h;
    p.conf = conf;
    return p;
}

// 预设 0：5x5
const ControlPointPreset kPreset0 = makePreset(5, 5, {
                                                         cp(0,0,-1,-1), cp(1,0,-0.5,-1), cp(2,0,0,-1), cp(3,0,0.5,-1), cp(4,0,1,-1),
                                                         cp(0,1,-1,-0.5), cp(1,1,-0.5,-0.5),
                                                         cp(2,1,-0.0052029684413368305,-0.6131420587090777),
                                                         cp(3,1,0.5884227308309977,-0.3990805107556692), cp(4,1,1,-0.5),
                                                         cp(0,2,-1,0), cp(1,2,-0.4210024670505933,-0.11895058380429502),
                                                         cp(2,2,-0.1019613423315412,-0.023812118047224606,0,-47,0.629f,0.849f),
                                                         cp(3,2,0.40275125660925437,-0.06345314544600389), cp(4,2,1,0),
                                                         cp(0,3,-1,0.5), cp(1,3,0.06801958477287173,0.5205913248960121,-31,-45),
                                                         cp(2,3,0.21446469120128908,0.29331610114301043,6,-56,0.566f,1.321f),
                                                         cp(3,3,0.5,0.5), cp(4,3,1,0.5),
                                                         cp(0,4,-1,1), cp(1,4,-0.31378372841550195,1),
                                                         cp(2,4,0.26153633255328046,1), cp(3,4,0.5,1), cp(4,4,1,1),
                                                     });

// 预设 1：4x4（横屏推荐）
const ControlPointPreset kPreset1 = makePreset(5, 5, {
                                                         cp(0,0,-1,-1), cp(1,0,-0.5,-1), cp(2,0,0,-1), cp(3,0,0.5,-1), cp(4,0,1,-1),
                                                         cp(0,1,-1,-0.5), cp(1,1,-0.5,-0.5),
                                                         cp(2,1,-0.0052029684413368305,-0.6131420587090777),
                                                         cp(3,1,0.5884227308309977,-0.3990805107556692), cp(4,1,1,-0.5),
                                                         cp(0,2,-1,0), cp(1,2,-0.4210024670505933,-0.11895058380429502),
                                                         cp(2,2,-0.1019613423315412,-0.023812118047224606,0,-47,0.629f,0.849f),
                                                         cp(3,2,0.40275125660925437,-0.06345314544600389), cp(4,2,1,0),
                                                         cp(0,3,-1,0.5), cp(1,3,0.06801958477287173,0.5205913248960121,-31,-45),
                                                         cp(2,3,0.21446469120128908,0.29331610114301043,6,-56,0.566f,1.321f),
                                                         cp(3,3,0.5,0.5), cp(4,3,1,0.5),
                                                         cp(0,4,-1,1), cp(1,4,-0.31378372841550195,1),
                                                         cp(2,4,0.26153633255328046,1), cp(3,4,0.5,1), cp(4,4,1,1),
                                                     });

    /*makePreset(4, 4, {
                                                         cp(0,0,-1,-1), cp(1,0,-0.33333333333333337f,-1), cp(2,0,0.33333333333333326f,-1), cp(3,0,1,-1),
                                                         cp(0,1,-1,-0.04495399932657351f),
                                                         cp(1,1,-0.24056117520129328f,-0.22465999020104f),
                                                         cp(2,1,0.334758885767489f,-0.00531297192779423f),
                                                         cp(3,1,0.9989920470678106f,-0.3382976020775408f,8,0,0.566f,1.792f),
                                                         cp(0,2,-1,0.33333333333333326f),
                                                         cp(1,2,-0.3425497314639411f,-0.000027501607956947893f),
                                                         cp(2,2,0.3321437945812673f,0.1981776353859399f),
                                                         cp(3,2,1,0.0766118180296832f),
                                                         cp(0,3,-1,1), cp(1,3,-0.33333333333333337f,1),
                                                         cp(2,3,0.33333333333333326f,1), cp(3,3,1,1),
                                                     });*/

// 预设 2：4x4
const ControlPointPreset kPreset2 = makePreset(4, 4, {
                                                         cp(0,0,-1,-1,0,0,1,2.075f), cp(1,0,-0.33333333333333337f,-1), cp(2,0,0.33333333333333326f,-1), cp(3,0,1,-1),
                                                         cp(0,1,-1,-0.4545779491139603f), cp(1,1,-0.33333333333333337f,-0.33333333333333337f),
                                                         cp(2,1,0.0889403142626457f,-0.6025711180694033f,-32,45),
                                                         cp(3,1,1,-0.33333333333333337f),
                                                         cp(0,2,-1,-0.07402408608567845f,1,0,1,0.094f),
                                                         cp(1,2,-0.2719422694359541f,0.09775369930903222f,25,-18,1.321f,0),
                                                         cp(2,2,0.19877414408395877f,0.4307383294587789f,48,-40,0.755f,0.975f),
                                                         cp(3,2,1,0.33333333333333326f,-37,0),
                                                         cp(0,3,-1,1), cp(1,3,-0.33333333333333337f,1),
                                                         cp(2,3,0.5125850864305672f,1,-20,-18,0,1.604f), cp(3,3,1,1),
                                                     });

// 预设 3：5x5
const ControlPointPreset kPreset3 = makePreset(5, 5, {
                                                         cp(0,0,-1,-1), cp(1,0,-0.4501953125f,-1,0,55,1,2.075f),
                                                         cp(2,0,0.1953125f,-1), cp(3,0,0.4580078125f,-1,0,-25), cp(4,0,1,-1),
                                                         cp(0,1,-1,-0.2514475377525607f,-16,0,2.327f,0.943f),
                                                         cp(1,1,-0.55859375f,-0.6609325945787148f,47,0,2.358f,0.377f),
                                                         cp(2,1,0.232421875f,-0.5244375756366635f,-66,-25,1.855f,1.164f),
                                                         cp(3,1,0.685546875f,-0.3753706470552125f), cp(4,1,1,-0.6699125300354287f),
                                                         cp(0,2,-1,0.035910396862284255f), cp(1,2,-0.4921875f,0.005378616309457018f,90,23,1,1.981f),
                                                         cp(2,2,0.021484375f,-0.1365043639066228f,0,42), cp(3,2,0.4765625f,0.05925822904974043f,-30,0,1.95f,0.44f),
                                                         cp(4,2,1,0.251428847823418f),
                                                         cp(0,3,-1,0.6968336464764276f,-68,0,1,0.786f),
                                                         cp(1,3,-0.6904296875f,0.5890744209958608f,-68,0),
                                                         cp(2,3,0.1845703125f,0.3879238667654693f,61,0), cp(3,3,0.60546875f,0.4633553246018661f,-47,-59,0.849f,1.73f),
                                                         cp(4,3,1,0.6214021886400309f,-33,0,0.377f,1.604f),
                                                         cp(0,4,-1,1), cp(1,4,-0.5,1,0,-73), cp(2,4,-0.3271484375f,1,0,-24,0.314f,2.704f),
                                                         cp(3,4,0.5,1), cp(4,4,1,1),
                                                     });

// 预设 4：5x5
const ControlPointPreset kPreset4 = makePreset(5, 5, {
                                                         cp(0,0,-1,-1), cp(1,0,-0.6393f,-1,0,0,1,2.3884f), cp(2,0,0,-1), cp(3,0,0.5f,-1), cp(4,0,1,-1),
                                                         cp(0,1,-1,-0.2301f), cp(1,1,-0.6934f,-0.331f,0,-0.7188f,1,1.063f),
                                                         cp(2,1,-0.0082f,-0.6814f,-0.2583f,0,1.0964f,1),
                                                         cp(3,1,0.5836f,-0.531f,0.7029f,0,1.5466f,1), cp(4,1,1,-0.6407f),
                                                         cp(0,2,-1,0.2973f,0,0,1.8352f,1), cp(1,2,-0.4082f,0.0602f),
                                                         cp(2,2,-0.1803f,-0.3646f,-0.2998f,0,1.1513f,1),
                                                         cp(3,2,0.477f,-0.1027f,0.8903f,-0.1882f,1.0807f,0.8551f), cp(4,2,1,-0.2973f),
                                                         cp(0,3,-1,0.7628f,0,0,2.3868f,1), cp(1,3,-0.2525f,0.4814f,-0.8406f,-1.6199f,1.4093f,1.2215f),
                                                         cp(2,3,0.3607f,0.2814f,-1.0713f,-0.0529f,1.0025f,0.7611f),
                                                         cp(3,3,0.4885f,0.623f,0,0.8184f,1,1.2876f), cp(4,3,1,0.5f),
                                                         cp(0,4,-1,1), cp(1,4,-0.4033f,1), cp(2,4,0.2672f,1), cp(3,4,0.5967f,1), cp(4,4,1,1),
                                                     });

// 预设 5：5x5
const ControlPointPreset kPreset5 = makePreset(5, 5, {
                                                         cp(0,0,-1,-1), cp(1,0,-0.2197f,-1), cp(2,0,0.0197f,-1), cp(3,0,0.8033f,-1), cp(4,0,1,-1),
                                                         cp(0,1,-1,-0.5451f), cp(1,1,-0.4885f,-0.4035f,-1.0246f,-0.2268f,1.1936f,0.8005f),
                                                         cp(2,1,-0.1213f,-0.2867f,0,-0.6981f,1,0.809f),
                                                         cp(3,1,0.3246f,-0.5628f,0,-1.2188f,1,1.044f), cp(4,1,1,-0.3292f),
                                                         cp(0,2,-1,0.1416f), cp(1,2,-0.341f,-0.0142f,0,-0.4004f,1,1.1293f),
                                                         cp(2,2,-0.0393f,-0.023f,0.2915f,-0.373f,1.044f,0.9879f),
                                                         cp(3,2,0.3148f,-0.0673f,-0.7853f,-0.8962f,1.4709f,1.0247f), cp(4,2,1,0.1912f),
                                                         cp(0,3,-1,0.5f), cp(1,3,-0.2689f,0.2743f,0.3404f,-0.5248f,1.0184f,0.4391f),
                                                         cp(2,3,0.0721f,0.269f,0.5302f,0.1244f,0.6723f,0.3225f),
                                                         cp(3,3,0.4148f,0.3894f,-0.6977f,-0.6783f,0.8094f,0.9247f), cp(4,3,1,0.446f),
                                                         cp(0,4,-1,1), cp(1,4,-0.7311f,1), cp(2,4,0.323f,1), cp(3,4,0.6393f,1), cp(4,4,1,1),
                                                     });

const ControlPointPreset *kPresets[] = {
    &kPreset0, &kPreset1, &kPreset2, &kPreset3, &kPreset4, &kPreset5
};
const int kPresetCount = 6;

// 小工具：随机数（0..1）
inline double rand01() { return QRandomGenerator::global()->generateDouble(); }
inline double randomRange(double min, double max) { return rand01() * (max - min) + min; }
inline double clampD(double x, double min, double max) { return qBound(min, x, max); }
inline double smoothstepD(double e0, double e1, double x)
{
    const double t = clampD((x - e0) / (e1 - e0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}
inline double fractD(double x) { return x - std::floor(x); }

double noise2(double x, double y)
{
    return fractD(std::sin(x * 12.9898 + y * 78.233) * 43758.5453);
}

double smoothNoise(double x, double y)
{
    const double x0 = std::floor(x);
    const double y0 = std::floor(y);
    const double x1 = x0 + 1.0;
    const double y1 = y0 + 1.0;
    const double xf = x - x0;
    const double yf = y - y0;
    const double u = xf * xf * (3.0 - 2.0 * xf);
    const double v = yf * yf * (3.0 - 2.0 * yf);
    const double n00 = noise2(x0, y0);
    const double n10 = noise2(x1, y0);
    const double n01 = noise2(x0, y1);
    const double n11 = noise2(x1, y1);
    const double nx0 = n00 * (1.0 - u) + n10 * u;
    const double nx1 = n01 * (1.0 - u) + n11 * u;
    return nx0 * (1.0 - v) + nx1 * v;
}

// 数值梯度并归一化
void computeNoiseGradient(double x, double y, double &dx, double &dy)
{
    const double eps = 0.001;
    const double n1 = smoothNoise(x + eps, y);
    const double n2 = smoothNoise(x - eps, y);
    const double n3 = smoothNoise(x, y + eps);
    const double n4 = smoothNoise(x, y - eps);
    dx = (n1 - n2) / (2.0 * eps);
    dy = (n3 - n4) / (2.0 * eps);
    const double len = std::sqrt(dx * dx + dy * dy);
    if (len > 1e-9) { dx /= len; dy /= len; }
}

// 4x4 矩阵（列主序，与 gl-matrix 一致）
struct Mat4 { float m[16]; };

Mat4 matMul(const Mat4 &a, const Mat4 &b)
{
    Mat4 r;
    for (int c = 0; c < 4; ++c) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
                sum += a.m[k * 4 + row] * b.m[c * 4 + k];
            r.m[c * 4 + row] = sum;
        }
    }
    return r;
}

Mat4 matTranspose(const Mat4 &a)
{
    Mat4 r;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r.m[j * 4 + i] = a.m[i * 4 + j];
    return r;
}

void vec4Transform(float out[4], const Mat4 &m, const float v[4])
{
    for (int row = 0; row < 4; ++row) {
        out[row] = m.m[0 * 4 + row] * v[0] + m.m[1 * 4 + row] * v[1]
                   + m.m[2 * 4 + row] * v[2] + m.m[3 * 4 + row] * v[3];
    }
}

// 双三次 Hermite 基矩阵（列主序，对应 AMLL 的 H = Mat4.fromValues(...)）
const Mat4 kHermiteH = {{
    2.0f, -2.0f, 1.0f, 1.0f,
    -3.0f, 3.0f, -2.0f, -1.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f
}};
const Mat4 kHermiteHT = matTranspose(kHermiteH);

} // namespace

// 平滑控制点（移植自 cp-generate.ts smoothifyControlPoints）
void MeshGradientItem::smoothifyControlPoints(QVector<ControlPointConf> &conf,
                                              int w, int h, int iterations,
                                              float factor, float factorIterationModifier)
{
    QVector<QVector<ControlPointConf>> grid;
    grid.resize(h);
    for (int j = 0; j < h; ++j) {
        grid[j].resize(w);
        for (int i = 0; i < w; ++i)
            grid[j][i] = conf[j * w + i];
    }

    const int kernel[3][3] = { {1, 2, 1}, {2, 4, 2}, {1, 2, 1} };
    const float kernelSum = 16.0f;
    float f = factor;

    for (int iter = 0; iter < iterations; ++iter) {
        QVector<QVector<ControlPointConf>> newGrid;
        newGrid.resize(h);
        for (int j = 0; j < h; ++j) {
            newGrid[j].resize(w);
            for (int i = 0; i < w; ++i) {
                if (i == 0 || i == w - 1 || j == 0 || j == h - 1) {
                    newGrid[j][i] = grid[j][i];
                    continue;
                }
                float sumX = 0, sumY = 0, sumUR = 0, sumVR = 0, sumUP = 0, sumVP = 0;
                for (int dj = -1; dj <= 1; ++dj) {
                    for (int di = -1; di <= 1; ++di) {
                        const float weight = kernel[dj + 1][di + 1];
                        const ControlPointConf &nb = grid[j + dj][i + di];
                        sumX  += nb.x  * weight;
                        sumY  += nb.y  * weight;
                        sumUR += nb.ur * weight;
                        sumVR += nb.vr * weight;
                        sumUP += nb.up * weight;
                        sumVP += nb.vp * weight;
                    }
                }
                const float avgX  = sumX  / kernelSum;
                const float avgY  = sumY  / kernelSum;
                const float avgUR = sumUR / kernelSum;
                const float avgVR = sumVR / kernelSum;
                const float avgUP = sumUP / kernelSum;
                const float avgVP = sumVP / kernelSum;

                const ControlPointConf &cur = grid[j][i];
                ControlPointConf n;
                n.cx = i; n.cy = j;
                n.x  = cur.x  * (1.0f - f) + avgX  * f;
                n.y  = cur.y  * (1.0f - f) + avgY  * f;
                n.ur = cur.ur * (1.0f - f) + avgUR * f;
                n.vr = cur.vr * (1.0f - f) + avgVR * f;
                n.up = cur.up * (1.0f - f) + avgUP * f;
                n.vp = cur.vp * (1.0f - f) + avgVP * f;
                newGrid[j][i] = n;
            }
        }
        grid = newGrid;
        f = float(qMin(1.0, qMax(f + factorIterationModifier, 0.0)));
    }

    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            conf[j * w + i] = grid[j][i];
}

// 随机控制点生成（移植自 cp-generate.ts generateControlPoints）
ControlPointPreset MeshGradientItem::generateControlPoints(int width, int height)
{
    const double variationFraction = randomRange(0.4, 0.6);
    const double normalOffset = randomRange(0.3, 0.6);
    const double blendFactor = 0.8;
    const int smoothIters = int(std::floor(randomRange(3.0, 5.0)));
    const double smoothFactor = randomRange(0.2, 0.3);
    const double smoothModifier = randomRange(-0.1, -0.05);

    const int w = width;
    const int h = height;
    QVector<ControlPointConf> conf;
    conf.reserve(w * h);
    const double dx = (w == 1) ? 0.0 : 2.0 / (w - 1);
    const double dy = (h == 1) ? 0.0 : 2.0 / (h - 1);

    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            const double baseX = (w == 1 ? 0.0 : i / double(w - 1)) * 2.0 - 1.0;
            const double baseY = (h == 1 ? 0.0 : j / double(h - 1)) * 2.0 - 1.0;

            const bool isBorder = (i == 0 || i == w - 1 || j == 0 || j == h - 1);
            const double pertX = isBorder ? 0.0
                                          : randomRange(-variationFraction * dx, variationFraction * dx);
            const double pertY = isBorder ? 0.0
                                          : randomRange(-variationFraction * dy, variationFraction * dy);
            double x = baseX + pertX;
            double y = baseY + pertY;

            const double ur = isBorder ? 0.0 : randomRange(-60.0, 60.0);
            const double vr = isBorder ? 0.0 : randomRange(-60.0, 60.0);
            const double up = isBorder ? 1.0 : randomRange(0.8, 1.2);
            const double vp = isBorder ? 1.0 : randomRange(0.8, 1.2);

            if (!isBorder) {
                const double uNorm = (baseX + 1.0) / 2.0;
                const double vNorm = (baseY + 1.0) / 2.0;
                double nx = 0, ny = 0;
                computeNoiseGradient(uNorm, vNorm, nx, ny);
                double offsetX = nx * normalOffset;
                double offsetY = ny * normalOffset;
                const double distToBorder = std::min({uNorm, 1.0 - uNorm, vNorm, 1.0 - vNorm});
                const double weight = smoothstepD(0.0, 1.0, distToBorder);
                offsetX *= weight;
                offsetY *= weight;
                x = x * (1.0 - blendFactor) + (x + offsetX) * blendFactor;
                y = y * (1.0 - blendFactor) + (y + offsetY) * blendFactor;
            }

            conf.append(cp(i, j, float(x), float(y),
                           float(ur), float(vr), float(up), float(vp)));
        }
    }

    smoothifyControlPoints(conf, w, h, smoothIters,
                           float(smoothFactor), float(smoothModifier));

    ControlPointPreset p;
    p.width = w;
    p.height = h;
    p.conf = conf;
    return p;
}

// 选择控制点预设（80% 用预设，20% 随机生成）
ControlPointPreset MeshGradientItem::choosePreset()
{
    if (rand01() > 0.8) {
        qDebug() << "idx:" << 6;
        return generateControlPoints(6, 6);
    }
    const int idx = int(rand01() * kPresetCount);
    qDebug() << "idx:" << idx;
    return *kPresets[qBound(0, idx, kPresetCount - 1)];
}

// 网格生成（移植自 AMLL BHPMesh::updateMesh 双三次 Hermite 插值）
QSGGeometry *MeshGradientItem::buildGeometry(const ControlPointPreset &preset,
                                             float width, float height)
{
    if (width <= 0.0f || height <= 0.0f)
        return nullptr;
    const float halfW = width * 0.5f;
    const float halfH = height * 0.5f;
    // 窗口/Item 宽高比（对应 AMLL 顶点着色器的 u_aspect）
    const float aspect = (height > 1.0f) ? (width / height) : 1.0f;
    const int subDiv = qMax(2, m_subDivisions);

    const int cpW = preset.width;
    const int cpH = preset.height;
    const int vertexWidth = (cpW - 1) * subDiv;
    const int vertexHeight = (cpH - 1) * subDiv;
    const int vertexCount = vertexWidth * vertexHeight;
    const int indexCount = (vertexWidth - 1) * (vertexHeight - 1) * 6;

    static QSGGeometry::Attribute attrs[] = {
        QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),
        QSGGeometry::Attribute::create(1, 3, QSGGeometry::FloatType, false),
        QSGGeometry::Attribute::create(2, 2, QSGGeometry::FloatType, false),
    };
    // AttributeSet 成员顺序：{ count, stride, attributes }
    static QSGGeometry::AttributeSet set = { 3, int(7 * sizeof(float)), attrs };

    QSGGeometry *geo = new QSGGeometry(set, vertexCount, indexCount,
                                       QSGGeometry::UnsignedShortType);
    geo->setDrawingMode(QSGGeometry::DrawTriangles);
    float *vertices = static_cast<float *>(geo->vertexData());
    quint16 *indices = static_cast<quint16 *>(geo->indexData());

    // 控制点 -> location / tangent（对应 AMLL ControlPoint）
    const int cpCount = cpW * cpH;
    QVector<float> locX(cpCount), locY(cpCount);
    QVector<float> uTanX(cpCount), uTanY(cpCount);
    QVector<float> vTanX(cpCount), vTanY(cpCount);
    const float uPower = 2.0f / (cpW - 1);
    const float vPower = 2.0f / (cpH - 1);
    // 切线强度：与 AMLL 原版一致（满强度 = uPower * cp.up / vPower * cp.vp），
    // 形变更明显，流动曲线更接近原版（若出现 Hermite 过冲/翻转可适当回调）
    for (int i = 0; i < cpCount; ++i) {
        const ControlPointConf &c = preset.conf.at(i);
        locX[i] = c.x;
        locY[i] = c.y;
        const float uRot = qDegreesToRadians(c.ur);
        const float vRot = qDegreesToRadians(c.vr);
        const float us = uPower * c.up;
        const float vs = vPower * c.vp;
        uTanX[i] = std::cos(uRot) * us;
        uTanY[i] = std::sin(uRot) * us;
        vTanX[i] = -std::sin(vRot) * vs;
        vTanY[i] = std::cos(vRot) * vs;
    }

    // 预计算 u/v 幂次（对应 AMLL normPowers）
    const int subDivM1 = subDiv - 1;
    const float invSubDivM1 = 1.0f / subDivM1;
    const float invTW = 1.0f / (subDivM1 * (cpH - 1));
    const float invTH = 1.0f / (subDivM1 * (cpW - 1));
    QVector<float> normPowers(subDiv * 4);
    for (int i = 0; i < subDiv; ++i) {
        const float norm = i * invSubDivM1;
        normPowers[i * 4 + 0] = norm * norm * norm;
        normPowers[i * 4 + 1] = norm * norm;
        normPowers[i * 4 + 2] = norm;
        normPowers[i * 4 + 3] = 1.0f;
    }

    const auto cpIdx = [cpW](int x, int y) { return x + y * cpW; };

    Mat4 tempM, accX, accY;
    float ux[4], uy[4];

    for (int x = 0; x < cpW - 1; ++x) {
        for (int y = 0; y < cpH - 1; ++y) {
            const int i00 = cpIdx(x, y);
            const int i01 = cpIdx(x, y + 1);
            const int i10 = cpIdx(x + 1, y);
            const int i11 = cpIdx(x + 1, y + 1);

            // x 轴系数
            {
                Mat4 &M = tempM;
                M.m[0] = locX[i00];   M.m[1] = locX[i01];
                M.m[2] = vTanX[i00];  M.m[3] = vTanX[i01];
                M.m[4] = locX[i10];   M.m[5] = locX[i11];
                M.m[6] = vTanX[i10];  M.m[7] = vTanX[i11];
                M.m[8] = uTanX[i00];  M.m[9] = uTanX[i01];
                M.m[10] = 0;          M.m[11] = 0;
                M.m[12] = uTanX[i10]; M.m[13] = uTanX[i11];
                M.m[14] = 0;          M.m[15] = 0;
                // Acc = H_T * M^T * H
                const Mat4 mt = matTranspose(M);
                const Mat4 t1 = matMul(mt, kHermiteH);
                accX = matMul(kHermiteHT, t1);
            }
            // y 轴系数
            {
                Mat4 &M = tempM;
                M.m[0] = locY[i00];   M.m[1] = locY[i01];
                M.m[2] = vTanY[i00];  M.m[3] = vTanY[i01];
                M.m[4] = locY[i10];   M.m[5] = locY[i11];
                M.m[6] = vTanY[i10];  M.m[7] = vTanY[i11];
                M.m[8] = uTanY[i00];  M.m[9] = uTanY[i01];
                M.m[10] = 0;          M.m[11] = 0;
                M.m[12] = uTanY[i10]; M.m[13] = uTanY[i11];
                M.m[14] = 0;          M.m[15] = 0;
                const Mat4 mt = matTranspose(M);
                const Mat4 t1 = matMul(mt, kHermiteH);
                accY = matMul(kHermiteHT, t1);
            }

            const float sX = float(x) / (cpW - 1);
            const float sY = float(y) / (cpH - 1);
            const int baseVx = y * subDiv;
            const int baseVy = x * subDiv;

            for (int u = 0; u < subDiv; ++u) {
                const int vxOffset = baseVx + u;
                const float *np = normPowers.constData() + u * 4;
                const float uVec[4] = { np[0], np[1], np[2], np[3] };
                vec4Transform(ux, accX, uVec);
                vec4Transform(uy, accY, uVec);
                // 独立 UV（与原版一致）：uvY 递减方向
                const float uvY = 1.0f - sY - u * invTW;

                for (int v = 0; v < subDiv; ++v) {
                    const int vy = baseVy + v;
                    const float *vp = normPowers.constData() + v * 4;
                    const float px = vp[0] * ux[0] + vp[1] * ux[1]
                                     + vp[2] * ux[2] + vp[3] * ux[3];
                    const float py = vp[0] * uy[0] + vp[1] * uy[1]
                                     + vp[2] * uy[2] + vp[3] * uy[3];
                    const float uvX = sX + v * invTH;

                    const int vi = (vxOffset + vy * vertexWidth) * 7;
                    // NDC (-1..1) -> item 本地坐标 (0..width, 0..height)
                    // 对应 AMLL 顶点着色器的 aspect 补偿：
                    //   if (u_aspect > 1.0) pos.y *= u_aspect; else pos.x /= u_aspect;
                    // 使控制点网格在屏幕空间保持各向同性（正方形网格单元），
                    // 渐变形状不受窗口宽高比拉伸，更接近 AMLL 原版观感
                    float nx = px;
                    float ny = py;
                    if (aspect > 1.0f) {
                        ny = py * aspect;
                    } else if (aspect > 0.0f) {
                        nx = px / aspect;
                    }
                    vertices[vi + 0] = (nx + 1.0f) * halfW;
                    vertices[vi + 1] = (ny + 1.0f) * halfH;
                    // 顶点颜色：AMLL 原版为白色 (1,1,1)，颜色完全来自封面纹理，
                    // 靠高细分网格的 Hermite 插值保证几何平滑（无马赛克）
                    vertices[vi + 2] = 1.0f;
                    vertices[vi + 3] = 1.0f;
                    vertices[vi + 4] = 1.0f;
                    vertices[vi + 5] = uvX;
                    vertices[vi + 6] = uvY;
                }
            }
        }
    }

    // 索引表（对应 AMLL Mesh::resize）
    int idxPos = 0;
    for (int y = 0; y < vertexHeight - 1; ++y) {
        for (int x = 0; x < vertexWidth - 1; ++x) {
            const quint16 v0 = quint16(y * vertexWidth + x);
            const quint16 v1 = quint16(y * vertexWidth + x + 1);
            const quint16 v2 = quint16((y + 1) * vertexWidth + x);
            const quint16 v3 = quint16((y + 1) * vertexWidth + x + 1);
            indices[idxPos++] = v0;
            indices[idxPos++] = v1;
            indices[idxPos++] = v2;
            indices[idxPos++] = v1;
            indices[idxPos++] = v3;
            indices[idxPos++] = v2;
        }
    }

    return geo;
}
