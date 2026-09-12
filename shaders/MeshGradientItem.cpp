// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "MeshGradientItem.h"

#include <QNetworkReply>
#include <QQuickWindow>
#include <QRandomGenerator>
#include <QSGGeometryNode>
#include <QSGTexture>
#include <QtConcurrent>

#include <cmath>

namespace {

// std140：mat4 + qt_Opacity + time + vec2 + vec4[4]
constexpr int kUniformSize = 144;

// 可分离盒式模糊（采样坐标钳制，边缘亮度不变）
void boxBlur(QImage &img, int radius, int iterations)
{
    const int w = img.width(), h = img.height();
    const int cnt = radius * 2 + 1;
    QImage buf(w, h, QImage::Format_RGBA8888);
    for (int it = 0; it < iterations; ++it) {
        for (int y = 0; y < h; ++y) {
            const uchar *src = img.constScanLine(y);
            uchar *dst = buf.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int r = 0, g = 0, b = 0;
                for (int k = -radius; k <= radius; ++k) {
                    const int xx = qBound(0, x + k, w - 1);
                    const uchar *p = src + xx * 4;
                    r += p[0]; g += p[1]; b += p[2];
                }
                uchar *o = dst + x * 4;
                o[0] = uchar(r / cnt); o[1] = uchar(g / cnt); o[2] = uchar(b / cnt); o[3] = 255;
            }
        }
        for (int y = 0; y < h; ++y) {
            uchar *dst = img.scanLine(y);
            for (int x = 0; x < w; ++x) {
                int r = 0, g = 0, b = 0;
                for (int k = -radius; k <= radius; ++k) {
                    const int yy = qBound(0, y + k, h - 1);
                    const uchar *p = buf.constScanLine(yy) + x * 4;
                    r += p[0]; g += p[1]; b += p[2];
                }
                uchar *o = dst + x * 4;
                o[0] = uchar(r / cnt); o[1] = uchar(g / cnt); o[2] = uchar(b / cnt); o[3] = 255;
            }
        }
    }
}

QImage decodeCover(QImage img)
{
    if (img.isNull())
        return {};
    img = img.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
              .convertToFormat(QImage::Format_RGBA8888);

    // 透明/半透明像素（圆角封面等）合成到全图均值色，避免出现黑边
    double sr = 0, sg = 0, sb = 0, sw = 0;
    int minA = 255;
    for (int y = 0; y < 32; ++y) {
        const uchar *line = img.constScanLine(y);
        for (int x = 0; x < 32; ++x) {
            const uchar *p = line + x * 4;
            const double a = p[3] / 255.0;
            sr += p[0] * a; sg += p[1] * a; sb += p[2] * a; sw += a;
            minA = qMin(minA, int(p[3]));
        }
    }
    if (minA < 255) {
        const double mr = sw > 0 ? sr / sw : 0.0;
        const double mg = sw > 0 ? sg / sw : 0.0;
        const double mb = sw > 0 ? sb / sw : 0.0;
        for (int y = 0; y < 32; ++y) {
            uchar *line = img.scanLine(y);
            for (int x = 0; x < 32; ++x) {
                uchar *p = line + x * 4;
                const double a = p[3] / 255.0;
                p[0] = uchar(mr + (p[0] - mr) * a);
                p[1] = uchar(mg + (p[1] - mg) * a);
                p[2] = uchar(mb + (p[2] - mb) * a);
                p[3] = 255;
            }
        }
    }

    // 先强压缩对比度让边缘向均值收敛（重复时过渡平滑），再扩饱和度、拉对比度
    constexpr float kPreContrast = 0.5f, kSat = 2.6f, kContrast = 1.4f, kBright = 0.8f;
    for (int y = 0; y < 32; ++y) {
        uchar *line = img.scanLine(y);
        for (int x = 0; x < 32; ++x) {
            uchar *p = line + x * 4;
            float r = p[0], g = p[1], b = p[2];
            r = (r - 128.f) * kPreContrast + 128.f;
            g = (g - 128.f) * kPreContrast + 128.f;
            b = (b - 128.f) * kPreContrast + 128.f;
            const float gray = r * 0.3f + g * 0.59f + b * 0.11f;
            r = gray + (r - gray) * kSat;
            g = gray + (g - gray) * kSat;
            b = gray + (b - gray) * kSat;
            r = (r - 128.f) * kContrast + 128.f;
            g = (g - 128.f) * kContrast + 128.f;
            b = (b - 128.f) * kContrast + 128.f;
            r *= kBright; g *= kBright; b *= kBright;
            p[0] = uchar(qBound(0, int(r + 0.5f), 255));
            p[1] = uchar(qBound(0, int(g + 0.5f), 255));
            p[2] = uchar(qBound(0, int(b + 0.5f), 255));
            p[3] = 255;
        }
    }

    boxBlur(img, 2, 4);
    return img;
}

class BackgroundShader : public QSGMaterialShader
{
public:
    explicit BackgroundShader(int algorithm)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/shaders/background.vert.qsb"));
        setShaderFileName(FragmentStage,
                          algorithm == 0 ? QStringLiteral(":/shaders/fluid.frag.qsb")
                                         : QStringLiteral(":/shaders/classic.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        auto *mat = static_cast<BackgroundMaterial *>(newMaterial);
        QByteArray *buf = state.uniformData();
        Q_ASSERT(buf->size() >= kUniformSize);
        if (state.isMatrixDirty())
            memcpy(buf->data(), state.combinedMatrix().constData(), 64);
        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            memcpy(buf->data() + 64, &opacity, 4);
        }
        memcpy(buf->data() + 68, &mat->time, 4);
        memcpy(buf->data() + 72, mat->resolution, 8);
        for (int i = 0; i < 4; ++i) {
            const QColor &c = mat->colors[i];
            const float v[4] = { float(c.redF()), float(c.greenF()), float(c.blueF()), 1.0f };
            memcpy(buf->data() + 80 + 16 * i, v, 16);
        }
        return true;
    }

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *) override
    {
        if (binding != 1)
            return;
        auto *mat = static_cast<BackgroundMaterial *>(newMaterial);
        if (!mat->texture)
            return;
        *texture = mat->texture;
        mat->texture->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    }
};

} // namespace

BackgroundMaterial::BackgroundMaterial(int alg)
    : algorithm(alg)
    , time(0.0f)
    , resolution{1.0f, 1.0f}
    , texture(nullptr)
{
    setFlag(Blending, false);
}

QSGMaterialType *BackgroundMaterial::type() const
{
    static QSGMaterialType fluidType;
    static QSGMaterialType classicType;
    return algorithm == 0 ? &fluidType : &classicType;
}

QSGMaterialShader *BackgroundMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new BackgroundShader(algorithm);
}

int BackgroundMaterial::compare(const QSGMaterial *other) const
{
    const auto *o = static_cast<const BackgroundMaterial *>(other);
    if (algorithm != o->algorithm)
        return algorithm - o->algorithm;
    if (texture != o->texture)
        return texture < o->texture ? -1 : 1;
    for (int i = 0; i < 4; ++i) {
        const QRgb a = colors[i].rgba();
        const QRgb b = o->colors[i].rgba();
        if (a != b)
            return a < b ? -1 : 1;
    }
    return 0;
}

MeshGradientItem::MeshGradientItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_network(new QNetworkAccessManager(this))
{
    setFlag(ItemHasContents, true);
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        m_time += float(m_elapsed.restart()) / 1000.0f * float(m_flowSpeed);
        update();
    });
    connect(m_network, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return;
        const QByteArray data = reply->readAll();
        QtConcurrent::run([data] { return decodeCover(QImage::fromData(data)); })
            .then(this, [this](const QImage &img) { applyCover(img); });
    });
    connect(this, &QQuickItem::windowChanged, this, [this] { syncTimer(); });
}

void MeshGradientItem::setCoverUrl(const QUrl &url)
{
    if (url == m_coverUrl)
        return;
    m_coverUrl = url;
    emit coverUrlChanged();
    loadCover(url);
}

void MeshGradientItem::loadCover(const QUrl &url)
{
    const QString scheme = url.scheme();
    if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
        m_network->get(QNetworkRequest(url));
        return;
    }
    QtConcurrent::run([url] {
        if (url.scheme() == QLatin1String("data")) {
            const QByteArray raw = QByteArray::fromBase64(
                url.toString().section(QLatin1String("base64,"), 1).toUtf8());
            return decodeCover(QImage::fromData(raw));
        }
        QString path = url.isLocalFile() ? url.toLocalFile() : url.toString();
        if (path.startsWith(QLatin1String("qrc:")))
            path.remove(0, 3);
        return decodeCover(QImage(path));
    }).then(this, [this](const QImage &img) { applyCover(img); });
}

void MeshGradientItem::applyCover(const QImage &img)
{
    if (img.isNull())
        return;
    m_coverImage = img;
    m_geoSize = QSizeF(); // 切歌重掷地形
    update();
}

bool MeshGradientItem::setColorAt(int index, const QColor &c)
{
    if (c == m_colors[index])
        return false;
    m_colors[index] = c;
    update();
    return true;
}

void MeshGradientItem::setColor1(const QColor &c)
{
    if (setColorAt(0, c))
        emit color1Changed();
}

void MeshGradientItem::setColor2(const QColor &c)
{
    if (setColorAt(1, c))
        emit color2Changed();
}

void MeshGradientItem::setColor3(const QColor &c)
{
    if (setColorAt(2, c))
        emit color3Changed();
}

void MeshGradientItem::setAlgorithm(int a)
{
    if (a == m_algorithm)
        return;
    m_algorithm = a;
    emit algorithmChanged();
    update();
}

void MeshGradientItem::setAnimating(bool a)
{
    if (a == m_animating)
        return;
    m_animating = a;
    emit animatingChanged();
    syncTimer();
}

void MeshGradientItem::setFlowSpeed(qreal s)
{
    if (s == m_flowSpeed)
        return;
    m_flowSpeed = s;
    emit flowSpeedChanged();
}

void MeshGradientItem::itemChange(ItemChange change, const ItemChangeData &data)
{
    if (change == ItemVisibleHasChanged)
        syncTimer();
    QQuickItem::itemChange(change, data);
}

void MeshGradientItem::syncTimer()
{
    const bool run = m_animating && isVisible() && window();
    if (run && !m_timer.isActive()) {
        m_elapsed.restart();
        m_timer.start();
    } else if (!run && m_timer.isActive()) {
        m_timer.stop();
    }
}

namespace {

QSGGeometry *makeQuadGeometry()
{
    auto *geo = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
    geo->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    geo->setVertexDataPattern(QSGGeometry::DynamicPattern);
    return geo;
}

// 地形参数：p[4] = 悬崖位置相位，r[12] = 模态/悬崖幅值（r[3]、r[4] 暂空置）
struct TerrainParams
{
    float p[4];
    float r[12];
};

// 海选出的地形预设池，每次切歌随机取一组（不与上一次重复）
const TerrainParams kTerrainPresets[] = {
    {{4.67f, 2.70f, 4.24f, 1.59f},
     {-0.62f, 0.70f, -0.12f, 0.41f, 0.49f, -0.48f, 0.41f, 0.75f, -0.55f, -0.91f, -0.37f, 0.87f}},
    {{5.36f, 5.68f, 5.32f, 2.82f},
     {-0.70f, 0.39f, -0.09f, -0.81f, 0.72f, 0.61f, 0.98f, -0.53f, 0.16f, 0.67f, 0.87f, 0.01f}},
    {{1.47f, 0.65f, 2.70f, 5.10f},
     {0.65f, 0.65f, 0.98f, 0.72f, 0.92f, 0.20f, -0.15f, 0.82f, -0.49f, -0.58f, -0.87f, -0.68f}},
    {{6.21f, 3.60f, 0.28f, 3.30f},
     {-0.96f, 0.24f, -0.69f, 0.37f, 0.20f, 0.32f, 0.48f, -0.82f, 0.30f, 0.40f, 0.49f, -0.43f}},
    {{0.59f, 1.55f, 1.01f, 0.91f},
     {-0.27f, 0.78f, -0.70f, 0.47f, -0.26f, -0.61f, -0.68f, 0.49f, -0.41f, -0.18f, -0.82f, -0.79f}},
};
constexpr int kTerrainPresetCount = int(sizeof(kTerrainPresets) / sizeof(kTerrainPresets[0]));

// 自研扭曲网格：基底方格 + 驻波模态（山坡）与 tanh 折叠面（悬崖），
// 模态在四边天然为零，几何上不漏空。每次调用随机选一个地形预设。
QSGGeometry *makeWarpGeometry(float width, float height)
{
    constexpr int kCells = 28;
    constexpr int kVerts = kCells + 1;

    static int lastPreset = -1;
    int presetIdx = QRandomGenerator::global()->bounded(kTerrainPresetCount);
    if (presetIdx == lastPreset && kTerrainPresetCount > 1)
        presetIdx = (presetIdx + 1 + QRandomGenerator::global()->bounded(kTerrainPresetCount - 1)) % kTerrainPresetCount;
    lastPreset = presetIdx;
    const float *p = kTerrainPresets[presetIdx].p;
    const float *r = kTerrainPresets[presetIdx].r;

    QSGGeometry *geo = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(),
                                       kVerts * kVerts, kCells * kCells * 6,
                                       QSGGeometry::UnsignedShortType);
    geo->setDrawingMode(QSGGeometry::DrawTriangles);
    geo->setVertexDataPattern(QSGGeometry::StaticPattern);

    quint16 *idx = geo->indexDataAsUShort();
    int i = 0;
    for (int y = 0; y < kCells; ++y) {
        for (int x = 0; x < kCells; ++x) {
            const quint16 a = quint16(y * kVerts + x);
            idx[i++] = a;
            idx[i++] = a + kVerts;
            idx[i++] = a + 1;
            idx[i++] = a + 1;
            idx[i++] = a + kVerts;
            idx[i++] = a + kVerts + 1;
        }
    }

    QSGGeometry::TexturedPoint2D *vp = geo->vertexDataAsTexturedPoint2D();
    constexpr float kPi = 3.14159265f;
    for (int yi = 0; yi < kVerts; ++yi) {
        const float v = float(yi) / kCells;
        for (int xi = 0; xi < kVerts; ++xi) {
            const float u = float(xi) / kCells;
            // 钉边驻波模态：sin(nπu)·sin(mπv) 在四条边天然为零，无边缘折叠带
            const float m1 = std::sin(kPi * u) * std::sin(kPi * v);
            const float m2 = std::sin(2.0f * kPi * u) * std::sin(kPi * v);
            const float m3 = std::sin(kPi * u) * std::sin(2.0f * kPi * v);
            const float m4 = std::sin(2.0f * kPi * u) * std::sin(2.0f * kPi * v);
            // 山坡（驻波模态随机叠加）+ 悬崖（tanh 折叠面，中心包络向边缘渐隐）
            const float dx = 0.12f * (r[0] * m1 + r[1] * m2 + r[2] * m4)
                           + 0.10f * r[8] * std::tanh(6.0f * (v - 0.5f - 0.30f * std::sin(p[0]))) * m1
                           + 0.06f * r[9] * std::tanh(6.0f * ((u + v) * 0.7f - 0.7f - 0.33f * std::sin(p[1]))) * m1;
            const float dy = 0.12f * (r[5] * m1 + r[6] * m3 + r[7] * m4)
                           + 0.12f * r[10] * std::tanh(6.0f * (v - 0.5f - 0.30f * std::sin(p[2]))) * m1
                           + 0.06f * r[11] * std::tanh(6.0f * ((u - v) * 0.7f - 0.7f + 0.33f * std::sin(p[3]))) * m1;
            vp->set((u + dx) * width, (v + dy) * height, u, v);
            ++vp;
        }
    }
    return geo;
}

} // namespace

QSGNode *MeshGradientItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    auto *node = static_cast<QSGGeometryNode *>(old);
    if (!node) {
        node = new QSGGeometryNode;
        node->setFlag(QSGNode::OwnsGeometry, true);
        node->setMaterial(new BackgroundMaterial(m_algorithm));
        node->setFlag(QSGNode::OwnsMaterial, true);
    } else if (static_cast<BackgroundMaterial *>(node->material())->algorithm != m_algorithm) {
        node->setMaterial(new BackgroundMaterial(m_algorithm));
    }
    auto *mat = static_cast<BackgroundMaterial *>(node->material());

    // 纹理属性只需在创建时设置一次
    const auto setupTexture = [this](QSGTexture *tex) {
        tex->setFiltering(QSGTexture::Linear);
        // 镜像重复：UV 旋转出界按正反正反取样，处处连续
        tex->setHorizontalWrapMode(QSGTexture::MirroredRepeat);
        tex->setVerticalWrapMode(QSGTexture::MirroredRepeat);
        return tex;
    };
    if (!m_coverImage.isNull()) {
        delete m_texture;
        m_texture = setupTexture(window()->createTextureFromImage(m_coverImage));
        m_coverImage = QImage();
    }
    if (!m_texture) {
        QImage fallback(4, 4, QImage::Format_RGB32);
        fallback.fill(m_colors[0]);
        m_texture = setupTexture(window()->createTextureFromImage(fallback));
    }

    for (int i = 0; i < 3; ++i)
        mat->colors[i] = m_colors[i];
    mat->colors[3] = m_colors[2].darker(200);
    mat->texture = m_texture;
    mat->time = m_time;
    mat->resolution[0] = float(width());
    mat->resolution[1] = float(height());
    node->markDirty(QSGNode::DirtyMaterial);

    const QRectF r = boundingRect();
    const bool sizeValid = r.width() > 1.0 && r.height() > 1.0;
    if (sizeValid && (!node->geometry() || m_geoAlgorithm != m_algorithm || m_geoSize != r.size())) {
        node->setGeometry(m_algorithm == 0
                              ? makeQuadGeometry()
                              : makeWarpGeometry(float(r.width()), float(r.height())));
        m_geoAlgorithm = m_algorithm;
        m_geoSize = r.size();
        node->markDirty(QSGNode::DirtyGeometry);
    }

    if (m_algorithm == 0 && node->geometry()) {
        // Fluid：铺满屏幕的平面，形变完全在片元着色器
        const float w = float(r.width());
        const float h = float(r.height());
        QSGGeometry::TexturedPoint2D *vp = node->geometry()->vertexDataAsTexturedPoint2D();
        vp[0].set(0, h, 0, 1);
        vp[1].set(0, 0, 0, 0);
        vp[2].set(w, h, 1, 1);
        vp[3].set(w, 0, 1, 0);
        node->markDirty(QSGNode::DirtyGeometry);
    }
    return node;
}
