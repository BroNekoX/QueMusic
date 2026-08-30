// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "ColorExtractor.h"
#include "../meshgradient/MeshGradientItem.h"

#include <QBuffer>
#include <QNetworkReply>
#include <QtConcurrent>

#include <algorithm>

namespace {

constexpr int kColorCount = 3;
constexpr int kSampleSize = 48; // 固定采样，控制遍历量
constexpr int kSimilarDistance = 120;
constexpr int kSimilarDistanceSq = kSimilarDistance * kSimilarDistance;

// 封面无有效主色（纯灰/纯白）时的兜底配色
QVector<QColor> defaultColors()
{
    return { QColor(0x00, 0xEE, 0x66), QColor(0x00, 0xB1, 0xEE), QColor(0x9D, 0x4E, 0xDD) };
}

} // namespace

ColorExtractor::ColorExtractor(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    m_cache.setMaxCost(128);
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &ColorExtractor::onImageDownloaded);
    ensureDefaultRenderUrl();
}

// 默认封面处理成 data URI，作为无封面时的兜底渲染图
void ColorExtractor::ensureDefaultRenderUrl()
{
    QImage defaultImage(QStringLiteral(":/QueMusic/resources/app/musicpic.png"));
    if (defaultImage.isNull())
        return;

    const QImage render = MeshGradientItem::processCoverImage(defaultImage);
    if (!render.isNull())
        m_renderUrl = encodeDataUri(render);
}

QUrl ColorExtractor::imageSource() const
{
    return m_imageSource;
}

void ColorExtractor::setImageSource(const QUrl &source)
{
    if (m_imageSource == source)
        return;
    m_imageSource = source;
    emit imageSourceChanged();
    extractColorsFromUrl(source);
}

QVector<QColor> ColorExtractor::dominantColors() const
{
    return m_dominantColors;
}

QUrl ColorExtractor::renderUrl() const
{
    return m_renderUrl;
}

bool ColorExtractor::busy() const
{
    return m_busy;
}

void ColorExtractor::extractColors()
{
    extractColorsFromUrl(m_imageSource);
}

void ColorExtractor::extractColorsFromUrl(const QUrl &url)
{
    m_imageSource = url;

    if (url.isLocalFile()) {
        const QString path = url.toLocalFile();
        runTask(url.toString(), [path]() {
            QImage image;
            image.load(path);
            return extract(image);
        });
    } else if (!url.isEmpty()) {
        m_networkManager->get(QNetworkRequest(url));
    }
}

// 直接吃封面像素，省掉写临时文件再解码的往返
void ColorExtractor::extractColorsFromImage(const QVariant &image)
{
    QImage source;
    if (image.userType() == QMetaType::QImage) {
        source = image.value<QImage>();
    } else if (image.canConvert<QByteArray>()) {
        source.loadFromData(image.toByteArray());
    }
    if (source.isNull())
        return;

    const QImage key = source.convertToFormat(QImage::Format_ARGB32);
    const QString cacheKey = QStringLiteral("px:%1")
        .arg(qHashBits(key.constBits(), size_t(key.sizeInBytes())), 0, 16);

    runTask(cacheKey, [source]() { return extract(source); });
}

void ColorExtractor::onImageDownloaded(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    const QByteArray data = reply->readAll();
    const QString cacheKey = reply->url().toString();
    // 网络图解码也放后台
    runTask(cacheKey, [data]() {
        QImage image;
        image.loadFromData(data);
        return extract(image);
    });
}

void ColorExtractor::runTask(const QString &cacheKey, const std::function<ExtractionResult()> &task)
{
    if (cacheKey.isEmpty())
        return;

    if (ExtractionResult *cached = m_cache.object(cacheKey)) {
        applyResult(*cached);
        return;
    }

    const int generation = ++m_generation;
    setBusy(true);

    auto *watcher = new QFutureWatcher<ExtractionResult>(this);
    connect(watcher, &QFutureWatcher<ExtractionResult>::finished, this,
            [this, watcher, generation, cacheKey]() {
                const ExtractionResult result = watcher->result();
                watcher->deleteLater();

                // 期间又切歌，丢弃过期结果
                if (generation != m_generation)
                    return;

                if (!result.colors.isEmpty() || result.renderUrl.isValid())
                    m_cache.insert(cacheKey, new ExtractionResult(result));
                applyResult(result);
                setBusy(false);
            });
    watcher->setFuture(QtConcurrent::run(task));
}

void ColorExtractor::applyResult(const ExtractionResult &result)
{
    m_dominantColors = result.colors;
    emit colorsExtracted(m_dominantColors);

    QStringList colorStrings;
    colorStrings.reserve(m_dominantColors.size());
    for (const QColor &color : m_dominantColors)
        colorStrings.append(color.name());
    emit colorsExtractedAsString(colorStrings);

    if (result.renderUrl.isValid() && result.renderUrl != m_renderUrl) {
        m_renderUrl = result.renderUrl;
        emit renderUrlChanged();
    }
}

void ColorExtractor::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

// 工作线程：取色 + 生成渲染图
ExtractionResult ColorExtractor::extract(const QImage &image)
{
    ExtractionResult result;
    if (image.isNull()) {
        result.colors = defaultColors();
        return result;
    }

    result.colors = computeDominantColors(image, kColorCount);

    const QImage render = MeshGradientItem::processCoverImage(image);
    if (!render.isNull())
        result.renderUrl = encodeDataUri(render);

    return result;
}

QVector<QColor> ColorExtractor::computeDominantColors(const QImage &image, int count)
{
    if (image.isNull() || count <= 0)
        return defaultColors();

    // 先降采样再统计
    const QImage sampled = (image.width() > kSampleSize || image.height() > kSampleSize)
        ? image.scaled(kSampleSize, kSampleSize, Qt::KeepAspectRatio, Qt::FastTransformation)
        : image;
    const QImage rgb = sampled.format() == QImage::Format_RGB32
        ? sampled
        : sampled.convertToFormat(QImage::Format_RGB32);

    struct Bucket
    {
        quint64 r = 0;
        quint64 g = 0;
        quint64 b = 0;
        quint32 n = 0;
    };

    // 量化打包成 15 位整型键，比拼接 QString 快一个数量级
    QHash<quint32, Bucket> buckets;
    buckets.reserve(512);

    for (int y = 0; y < rgb.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(rgb.constScanLine(y));
        for (int x = 0; x < rgb.width(); ++x) {
            const QRgb pixel = line[x];
            const int r = qRed(pixel);
            const int g = qGreen(pixel);
            const int b = qBlue(pixel);

            if (r + g + b > 690) // 平均亮度 > 230，近白
                continue;

            const int maxVal = qMax(r, qMax(g, b));
            const int minVal = qMin(r, qMin(g, b));
            if (maxVal - minVal < 5) // 近灰色，对主题色无贡献
                continue;

            const quint32 key = (quint32(r >> 3) << 10) | (quint32(g >> 3) << 5)
                              | quint32(b >> 3);
            Bucket &bucket = buckets[key];
            bucket.r += r;
            bucket.g += g;
            bucket.b += b;
            ++bucket.n;
        }
    }

    if (buckets.isEmpty())
        return defaultColors();

    QVector<QPair<quint32, QColor>> ranked;
    ranked.reserve(buckets.size());
    for (auto it = buckets.constBegin(); it != buckets.constEnd(); ++it) {
        const Bucket &bucket = it.value();
        ranked.append({ bucket.n, QColor(int(bucket.r / bucket.n),
                                        int(bucket.g / bucket.n),
                                        int(bucket.b / bucket.n)) });
    }
    std::sort(ranked.begin(), ranked.end(),
              [](const QPair<quint32, QColor> &a, const QPair<quint32, QColor> &b) {
                  return a.first > b.first;
              });

    // 依次挑出互不相似的颜色并提亮
    QVector<QColor> result;
    result.reserve(count);
    for (const QPair<quint32, QColor> &entry : ranked) {
        if (result.size() >= count)
            break;

        const QColor &color = entry.second;
        bool similar = false;
        for (const QColor &selected : result) {
            const int dr = color.red() - selected.red();
            const int dg = color.green() - selected.green();
            const int db = color.blue() - selected.blue();
            if (dr * dr + dg * dg + db * db < kSimilarDistanceSq) { // 平方距离，省开方
                similar = true;
                break;
            }
        }
        if (similar)
            continue;

        const QColor hsv = color.toHsv();
        result.append(QColor::fromHsv(hsv.hue(),
                                      qMin(255, hsv.saturation() + 42),
                                      qBound(30, hsv.value() - 32, 200)));
    }

    return result.isEmpty() ? defaultColors() : result;
}

QUrl ColorExtractor::encodeDataUri(const QImage &image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();

    return QUrl(QStringLiteral("data:image/png;base64,")
                + QString::fromLatin1(bytes.toBase64()));
}
