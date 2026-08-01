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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

class ColorExtractor : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QUrl imageSource READ imageSource WRITE setImageSource NOTIFY imageSourceChanged)
	Q_PROPERTY(QVector<QColor> dominantColors READ dominantColors NOTIFY colorsExtracted)

	
public:
	explicit ColorExtractor(QObject *parent = nullptr);
	
	QUrl imageSource() const;
	void setImageSource(const QUrl &source);
	
	QVector<QColor> dominantColors() const;
	
	Q_INVOKABLE void extractColors();
	Q_INVOKABLE void extractColorsFromUrl(const QUrl &url);
	
	signals:
	void imageSourceChanged();
	void colorsExtracted(const QVector<QColor> &colors);
	void colorsExtractedAsString(const QStringList &colors);

private slots:
    void onImageDownloaded(QNetworkReply *reply);

	
private:
    double calculateColorDistance(const QColor &c1, const QColor &c2);
	QUrl m_imageSource;
	QVector<QColor> m_dominantColors;
	QNetworkAccessManager *m_networkManager;
	QVector<QColor> getDominantColors(const QImage &image, int count = 3);
};

#endif // COLOREXTRACTOR_H

