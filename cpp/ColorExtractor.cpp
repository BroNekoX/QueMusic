// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
// ColorExtractor.cpp
#include "ColorExtractor.h"
#include "MeshGradientItem.h"
#include <QPainter>
#include <QRgb>
#include <QMap>
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
    // 预制默认渲染图，保证 renderUrl 从创建起就非空，
    // 无歌曲/封面时 MeshGradient 背景不至于透明。
    ensureDefaultRenderUrl();
}

// 预制默认渲染图：把主项目默认封面图处理成 32x32 data URI 作为兜底。
// 之后任何 extractColorsFromUrl 成功都会覆盖 m_renderUrl；
// 若从未播放歌曲/封面 URL 无效，则一直使用这个预设值。
void ColorExtractor::ensureDefaultRenderUrl()
{
    if (!m_renderUrl.isEmpty())
        return;

    // 主项目默认封面（应用图标图），打包在 qrc 中
    QImage defaultImage(QStringLiteral(":/QueMusic/resources/app/musicpic.png"));
    if (defaultImage.isNull())
        return;

    QImage render = MeshGradientItem::processCoverImage(defaultImage);
    if (render.isNull())
        return;

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    render.save(&buffer, "PNG");
    buffer.close();
    m_renderUrl = QUrl(QStringLiteral("data:image/png;base64,")
                       + QString::fromLatin1(ba.toBase64()));
    // 构造阶段 QML 绑定尚未建立，无需 emit；getter 直接返回该预设值
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
		
		// 提取颜色
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
	if (m_imageSource.isEmpty()) {
		return;
	}
	
	QImage image(m_imageSource.toLocalFile());
    //image = m_imageSource.toLocalFile();
	if (image.isNull()) {
        qDebug() << "无法加载图片:" << m_imageSource;
	}
	
	m_dominantColors = getDominantColors(image, 3);
	emit colorsExtracted(m_dominantColors);
	
	QStringList colorStrings;
	for (const QColor &color : m_dominantColors) {
		colorStrings.append(color.name());
	}
	emit colorsExtractedAsString(colorStrings);

	// 渲染图缓存（MeshGradient 背景使用）
	processAndCacheRenderImage(m_imageSource, image);
}

void ColorExtractor::onImageDownloaded(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "下载图片失败:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QImage image;
    if (!image.loadFromData(data)) {
        qDebug() << "无法解码图片数据";
        return;
    }

    // 复用你已有的颜色提取逻辑
    m_dominantColors = getDominantColors(image, 3);
    emit colorsExtracted(m_dominantColors);

    // 字符串格式信号
    QStringList colorStrings;
    for (const QColor &color : m_dominantColors) {
        colorStrings.append(color.name());
    }
    emit colorsExtractedAsString(colorStrings);

    // 渲染图缓存（MeshGradient 背景使用）
    processAndCacheRenderImage(reply->url(), image);
}

void ColorExtractor::extractColorsFromUrl(const QUrl &url)
{
    m_imageSource = url;

    // 判断网络与本地环境，对应获取封面
    if (url.isLocalFile()) {
        extractColors();
    } else {
        QNetworkRequest request(url);
        m_networkManager->get(request);
    }
}

// 处理封面为 32x32 渲染图并缓存，生成 data URI 供 MeshGradient 直接使用。
// 同一封面 URL 只处理一次；MeshGradientItem 加载 data URI 时无需再下载/处理。
void ColorExtractor::processAndCacheRenderImage(const QUrl &key, const QImage &image)
{
    if (image.isNull())
        return;

    // 命中缓存则直接复用，避免重复的 32x32 缩放 + 像素处理 + 模糊
    QImage render = m_renderCache.value(key);
    if (render.isNull()) {
        render = MeshGradientItem::processCoverImage(image);
        if (render.isNull())
            return;
        m_renderCache.insert(key, render);
    }

    // 编码为 data URI（32x32 PNG，仅数 KB，完全内存化）
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    render.save(&buffer, "PNG");
    buffer.close();
    m_renderUrl = QUrl(QStringLiteral("data:image/png;base64,")
                       + QString::fromLatin1(ba.toBase64()));
    emit renderUrlChanged();
}

double ColorExtractor::calculateColorDistance(const QColor &c1, const QColor &c2)
{
    // 欧几里得距离
    int dr = c1.red() - c2.red();
    int dg = c1.green() - c2.green();
    int db = c1.blue() - c2.blue();

    return sqrt(dr*dr + dg*dg + db*db);
}


QVector<QColor> ColorExtractor::getDominantColors(const QImage &image, int count)
{
    if (image.isNull() || count <= 0) {
        return QVector<QColor>();
    }

    // 减少尺寸150*150
    QImage smallImage = image.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QHash<QString, QColor> colorMap;
    QHash<QString, int> colorCount;

    for (int y = 0; y < smallImage.height(); y++) {
        const QRgb *scanLine = reinterpret_cast<const QRgb*>(smallImage.scanLine(y));
        for (int x = 0; x < smallImage.width(); x++) {
            QRgb pixel = scanLine[x];

            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);

            // 过滤灰色和暗色
            int maxVal = qMax(r, qMax(g, b));
            int minVal = qMin(r, qMin(g, b));
            int brightness = (r + g + b) / 3;

            // 颜色范围限制
            if (brightness > 230) continue;
            if ((maxVal - minVal) < 5) continue; // 饱和度阈值

            // 量化颜色到32级
            int qR = (r >> 3) << 3;
            int qG = (g >> 3) << 3;
            int qB = (b >> 3) << 3;

            QString key = QString("%1-%2-%3").arg(qR).arg(qG).arg(qB);

            if (!colorMap.contains(key)) {
                // 存储原始颜色
                colorMap[key] = QColor(r, g, b);
                colorCount[key] = 1;
            } else {
                // 累加颜色值，最后求平均
                QColor current = colorMap[key];
                int count = colorCount[key];

                int newR = (current.red() * count + r) / (count + 1);
                int newG = (current.green() * count + g) / (count + 1);
                int newB = (current.blue() * count + b) / (count + 1);

                colorMap[key] = QColor(newR, newG, newB);
                colorCount[key]++;
            }
        }
    }

    if (colorMap.isEmpty()) {
        // 返回默认极光颜色
        return QVector<QColor>{
            QColor(0x00, 0xEE, 0x66),
            QColor(0x00, 0xB1, 0xEE),
            QColor(0x9D, 0x4E, 0xDD)
        };
    }

    // 按出现频率排序
    QVector<QPair<QString, int>> sortedColors;
    for (auto it = colorCount.begin(); it != colorCount.end(); ++it) {
        sortedColors.append(qMakePair(it.key(), it.value()));
    }

    std::sort(sortedColors.begin(), sortedColors.end(),
              [](const QPair<QString, int> &a, const QPair<QString, int> &b) {
                  return a.second > b.second;
              });


    // 选择前i个颜色，确保它们不相似
    QVector<QColor> result;
    for (int i = 0; i < sortedColors.size() && result.size() < count; i++) {
        QColor color = colorMap[sortedColors[i].first];

        bool isSimilar = false;
        for (const QColor &selected : result) {
            double dist = calculateColorDistance(color, selected);
            if (dist < 120.0) { // 相似度阈值
                isSimilar = true;
                break;
            }
        }

        // 相似处理
        if (!isSimilar) {
            QColor hsvColor = color.toHsv();
            int h = hsvColor.hue();
            int s = qMin(255, hsvColor.saturation() + 42); // 增加饱和度
            int v = qBound(30, hsvColor.value() - 32, 200);  // 限制亮度
            QColor vibrantColor = QColor::fromHsv(h, s, v);
            result.append(vibrantColor);
        }
    }

    // 如果没收集到足够颜色，补全


    return result;
}

