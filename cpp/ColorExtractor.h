// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COLOREXTRACTOR_H
#define COLOREXTRACTOR_H

#include <QObject>
#include <QColor>
#include <QImage>
#include <QVector>
#include <QUrl>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QtQmlIntegration/qqmlintegration.h>

class ColorExtractor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl imageSource READ imageSource WRITE setImageSource NOTIFY imageSourceChanged)
    Q_PROPERTY(QVector<QColor> dominantColors READ dominantColors NOTIFY colorsExtracted)
    Q_PROPERTY(QUrl renderUrl READ renderUrl NOTIFY renderUrlChanged)

public:
    explicit ColorExtractor(QObject *parent = nullptr);

    QUrl imageSource() const;
    void setImageSource(const QUrl &source);

    QVector<QColor> dominantColors() const;
    // 封面经 32x32 处理后的 data URI（供 MeshGradient 直接使用）
    QUrl renderUrl() const;

    Q_INVOKABLE void extractColors();
    Q_INVOKABLE void extractColorsFromUrl(const QUrl &url);

signals:
    void imageSourceChanged();
    void colorsExtracted(const QVector<QColor> &colors);
    void colorsExtractedAsString(const QStringList &colors);
    void renderUrlChanged();

private slots:
    void onImageDownloaded(QNetworkReply *reply);

private:
    double calculateColorDistance(const QColor &c1, const QColor &c2);
    QVector<QColor> getDominantColors(const QImage &image, int count = 3);
    // 统一处理：计算主色、发信号、缓存渲染图
    void emitExtractedColors(const QImage &image, const QUrl &cacheKey);
    // 处理封面并缓存，生成 data URI
    void processAndCacheRenderImage(const QUrl &key, const QImage &image);
    QUrl encodeDataUri(const QImage &image);
    // 预制默认渲染图（无封面时兜底）
    void ensureDefaultRenderUrl();

    QUrl m_imageSource;
    QVector<QColor> m_dominantColors;
    QNetworkAccessManager *m_networkManager;
    QHash<QUrl, QImage> m_renderCache;
    QUrl m_renderUrl;
};

#endif // COLOREXTRACTOR_H

