// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COLOREXTRACTOR_H
#define COLOREXTRACTOR_H

#include <QCache>
#include <QColor>
#include <QFutureWatcher>
#include <QImage>
#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <QVariant>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

#include <functional>

// 取色结果（值类型，可跨线程）：封面主色
struct ExtractionResult
{
    QVector<QColor> colors;
};

// 封面取色全部在工作线程完成，切歌不阻塞 UI
class ColorExtractor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl imageSource READ imageSource WRITE setImageSource NOTIFY imageSourceChanged)
    Q_PROPERTY(QVector<QColor> dominantColors READ dominantColors NOTIFY colorsExtracted)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit ColorExtractor(QObject *parent = nullptr);

    QUrl imageSource() const;
    void setImageSource(const QUrl &source);
    QVector<QColor> dominantColors() const;
    bool busy() const;

    // 三个入口均立即返回，结果经信号回传
    Q_INVOKABLE void extractColorsFromUrl(const QUrl &url);
    Q_INVOKABLE void extractColorsFromImage(const QVariant &image);
    Q_INVOKABLE void extractColors();

signals:
    void imageSourceChanged();
    void colorsExtracted(const QVector<QColor> &colors);
    void colorsExtractedAsString(const QStringList &colors);
    void busyChanged();

private slots:
    void onImageDownloaded(QNetworkReply *reply);

private:
    // 按 cacheKey 派发后台取色，命中缓存直接复用
    void runTask(const QString &cacheKey, const std::function<ExtractionResult()> &task);
    void applyResult(const ExtractionResult &result);
    void setBusy(bool busy);

    static ExtractionResult extract(const QImage &image);
    static QVector<QColor> computeDominantColors(const QImage &image, int count);

    QUrl m_imageSource;
    QVector<QColor> m_dominantColors;
    QNetworkAccessManager *m_networkManager;
    QCache<QString, ExtractionResult> m_cache;
    int m_generation = 0; // 代次：丢弃切歌后的过期结果
    bool m_busy = false;
};

#endif // COLOREXTRACTOR_H
