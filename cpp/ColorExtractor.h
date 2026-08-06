// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
// ColorExtractor.h
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
#include <QEventLoop>

class ColorExtractor : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QUrl imageSource READ imageSource WRITE setImageSource NOTIFY imageSourceChanged)
	Q_PROPERTY(QVector<QColor> dominantColors READ dominantColors NOTIFY colorsExtracted)
	Q_PROPERTY(QUrl renderUrl READ renderUrl NOTIFY renderUrlChanged)

	
public:
	explicit ColorExtractor(QObject *parent = nullptr);
	
	QUrl imageSource() const;
	void setImageSource(const QUrl &source);
	
	QVector<QColor> dominantColors() const;
	// 当前封面经过 32x32 处理后的 data URI（供 MeshGradient 直接使用，避免重复下载/处理）
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
	QUrl m_imageSource;
	QVector<QColor> m_dominantColors;
	QNetworkAccessManager *m_networkManager;
	QVector<QColor> getDominantColors(const QImage &image, int count = 3);

	// 封面渲染图缓存：key=原始封面 URL，value=32x32 处理结果。
	// 同一封面只处理一次，后续直接复用并生成 data URI。
	QHash<QUrl, QImage> m_renderCache;
	QUrl m_renderUrl;
	// 处理封面并缓存、生成 data URI、发出 renderUrlChanged
	void processAndCacheRenderImage(const QUrl &key, const QImage &image);
	// 预制默认渲染图（无歌曲/封面时兜底，避免 MeshGradient 背景透明）。
	// 构造时把主项目默认封面图处理为 32x32 data URI 作为 m_renderUrl 初始值。
	void ensureDefaultRenderUrl();
};

#endif // COLOREXTRACTOR_H

