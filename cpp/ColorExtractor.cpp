// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "ColorExtractor.h"
#include "../meshgradient/MeshGradientItem.h"
#include <QRgb>
#include <QVector>
#include <QBuffer>
#include <algorithm>
#include <QDebug>
#include <QColor>

ColorExtractor::ColorExtractor(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &ColorExtractor::onImageDownloaded);
    ensureDefaultRenderUrl();
}

// 把默认封面图处理成 32x32 data URI
void ColorExtractor::ensureDefaultRenderUrl()
{
    if (!m_renderUrl.isEmpty())
        return;

    QImage defaultImage(QStringLiteral(":/QueMusic/resources/app/musicpic.png"));
    if (defaultImage.isNull())
        return;

    QImage render = MeshGradientItem::processCoverImage(defaultImage);
    if (render.isNull())
        return;

    m_renderUrl = encodeDataUri(render);
}

QUrl ColorExtractor::imageSource() const
{
    return m_imageSource;
}

void ColorExtractor::setImageSource(const QUrl &source)
{
    if (m_imageSource != source) {
        m_imageSource = source;
        emit imageSourceChanged();
        extractColorsFromUrl(source);
    }
}

QVector<QColor> ColorExtractor::dominantColors() const
{
    return m_dominantColors;
}

QUrl ColorExtractor::renderUrl() const
{
    return m_renderUrl;
}

void ColorExtractor::extractColors()
{
    if (m_imageSource.isEmpty())
        return;

    QImage image(m_imageSource.toLocalFile());
    if (image.isNull())
        qDebug() << "无法加载图片:" << m_imageSource;

    emitExtractedColors(image, m_imageSource);
}

void ColorExtractor::onImageDownloaded(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "下载图片失败:" << reply->errorString();
        return;
    }

    QImage image;
    if (!image.loadFromData(reply->readAll())) {
        qDebug() << "无法解码图片数据";
        return;
    }

    emitExtractedColors(image, reply->url());
}

void ColorExtractor::extractColorsFromUrl(const QUrl &url)
{
    m_imageSource = url;

    if (url.isLocalFile())
        extractColors();
    else
        m_networkManager->get(QNetworkRequest(url));
}

// 统一出口：计算主色、发信号、缓存渲染图
void ColorExtractor::emitExtractedColors(const QImage &image, const QUrl &cacheKey)
{
    m_dominantColors = getDominantColors(image, 3);
    emit colorsExtracted(m_dominantColors);

    QStringList colorStrings;
    for (const QColor &color : m_dominantColors)
        colorStrings.append(color.name());
    emit colorsExtractedAsString(colorStrings);

    processAndCacheRenderImage(cacheKey, image);
}

// 处理封面为 32x32 渲染图并缓存，生成 data URI 供 MeshGradient 直接使用
void ColorExtractor::processAndCacheRenderImage(const QUrl &key, const QImage &image)
{
    if (image.isNull())
        return;

    // 命中缓存则直接复用，避免重复处理
    QImage render = m_renderCache.value(key);
    if (render.isNull()) {
        render = MeshGradientItem::processCoverImage(image);
        if (render.isNull())
            return;
        m_renderCache.insert(key, render);
    }

    m_renderUrl = encodeDataUri(render);
    emit renderUrlChanged();
}

QUrl ColorExtractor::encodeDataUri(const QImage &image)
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    buffer.close();
    return QUrl(QStringLiteral("data:image/png;base64,")
                + QString::fromLatin1(ba.toBase64()));
}

double ColorExtractor::calculateColorDistance(const QColor &c1, const QColor &c2)
{
    // 欧几里得距离
    int dr = c1.red() - c2.red();
    int dg = c1.green() - c2.green();
    int db = c1.blue() - c2.blue();
    return sqrt(dr * dr + dg * dg + db * db);
}

QVector<QColor> ColorExtractor::getDominantColors(const QImage &image, int count)
{
    if (image.isNull() || count <= 0)
        return QVector<QColor>();

    // 缩小到 64x64 加速统计
    QImage smallImage = image.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QHash<QString, QColor> colorMap;
    QHash<QString, int> colorCount;

    for (int y = 0; y < smallImage.height(); y++) {
        const QRgb *scanLine = reinterpret_cast<const QRgb *>(smallImage.scanLine(y));
        for (int x = 0; x < smallImage.width(); x++) {
            QRgb pixel = scanLine[x];

            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            // 过滤过亮与灰色（低饱和度）
            int maxVal = qMax(r, qMax(g, b));
            int minVal = qMin(r, qMin(g, b));
            int brightness = (r + g + b) / 3;
            if (brightness > 230) continue;
            if ((maxVal - minVal) < 5) continue;

            // 量化颜色到 32 级，同类像素合并统计
            int qR = (r >> 3) << 3;
            int qG = (g >> 3) << 3;
            int qB = (b >> 3) << 3;
            QString key = QString("%1-%2-%3").arg(qR).arg(qG).arg(qB);

            if (!colorMap.contains(key)) {
                colorMap[key] = QColor(r, g, b);
                colorCount[key] = 1;
            } else {
                // 累加颜色值求平均，减少量化误差
                QColor current = colorMap[key];
                int cnt = colorCount[key];
                colorMap[key] = QColor(
                    (current.red() * cnt + r) / (cnt + 1),
                    (current.green() * cnt + g) / (cnt + 1),
                    (current.blue() * cnt + b) / (cnt + 1));
                colorCount[key]++;
            }
        }
    }

    if (colorMap.isEmpty()) {
        // 默认极光颜色
        return QVector<QColor>{
            QColor(0x00, 0xEE, 0x66),
            QColor(0x00, 0xB1, 0xEE),
            QColor(0x9D, 0x4E, 0xDD)
        };
    }

    // 按出现频率排序
    QVector<QPair<QString, int>> sortedColors;
    for (auto it = colorCount.begin(); it != colorCount.end(); ++it)
        sortedColors.append(qMakePair(it.key(), it.value()));

    std::sort(sortedColors.begin(), sortedColors.end(),
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                  return a.second > b.second;
              });

    // 按频率选取互不相似的颜色，并提亮为鲜艳色
    QVector<QColor> result;
    for (int i = 0; i < sortedColors.size() && result.size() < count; i++) {
        QColor color = colorMap[sortedColors[i].first];

        bool isSimilar = false;
        for (const QColor &selected : result) {
            if (calculateColorDistance(color, selected) < 120.0) {
                isSimilar = true;
                break;
            }
        }

        if (!isSimilar) {
            QColor hsvColor = color.toHsv();
            int h = hsvColor.hue();
            int s = qMin(255, hsvColor.saturation() + 42);
            int v = qBound(30, hsvColor.value() - 32, 200);
            result.append(QColor::fromHsv(h, s, v));
        }
    }

    return result;
}

