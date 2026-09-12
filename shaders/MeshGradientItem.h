// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#ifndef MESHGRADIENTITEM_H
#define MESHGRADIENTITEM_H

#include <QColor>
#include <QElapsedTimer>
#include <QImage>
#include <QNetworkAccessManager>
#include <QQuickItem>
#include <QSGMaterial>
#include <QTimer>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>

class QSGTexture;

// algorithm 决定着色器：0 = Fluid，1 = Classic(扭曲网格)
class BackgroundMaterial : public QSGMaterial
{
public:
    explicit BackgroundMaterial(int alg);

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;
    int compare(const QSGMaterial *other) const override;

    int algorithm;
    float time;
    float resolution[2];
    QColor colors[4];
    QSGTexture *texture;
};

// 多算法流体背景：统一输入三个主色 + 封面（内部降采样为低清纹理）
class MeshGradientItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl coverUrl READ coverUrl WRITE setCoverUrl NOTIFY coverUrlChanged)
    Q_PROPERTY(QColor color1 READ color1 WRITE setColor1 NOTIFY color1Changed)
    Q_PROPERTY(QColor color2 READ color2 WRITE setColor2 NOTIFY color2Changed)
    Q_PROPERTY(QColor color3 READ color3 WRITE setColor3 NOTIFY color3Changed)
    Q_PROPERTY(int algorithm READ algorithm WRITE setAlgorithm NOTIFY algorithmChanged)
    Q_PROPERTY(bool animating READ animating WRITE setAnimating NOTIFY animatingChanged)
    Q_PROPERTY(qreal flowSpeed READ flowSpeed WRITE setFlowSpeed NOTIFY flowSpeedChanged)

public:
    explicit MeshGradientItem(QQuickItem *parent = nullptr);

    QUrl coverUrl() const { return m_coverUrl; }
    void setCoverUrl(const QUrl &url);

    QColor color1() const { return m_colors[0]; }
    void setColor1(const QColor &c);
    QColor color2() const { return m_colors[1]; }
    void setColor2(const QColor &c);
    QColor color3() const { return m_colors[2]; }
    void setColor3(const QColor &c);

    int algorithm() const { return m_algorithm; }
    void setAlgorithm(int a);
    bool animating() const { return m_animating; }
    void setAnimating(bool a);
    qreal flowSpeed() const { return m_flowSpeed; }
    void setFlowSpeed(qreal s);

signals:
    void coverUrlChanged();
    void color1Changed();
    void color2Changed();
    void color3Changed();
    void algorithmChanged();
    void animatingChanged();
    void flowSpeedChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;

private:
    bool setColorAt(int index, const QColor &c);
    void loadCover(const QUrl &url);
    void applyCover(const QImage &img);
    void syncTimer();

    QUrl m_coverUrl;
    QColor m_colors[4] {Qt::black, Qt::black, Qt::black, Qt::black};
    int m_algorithm = 0;
    bool m_animating = true;
    qreal m_flowSpeed = 1.0;
    float m_time = 0.0f;
    QSizeF m_geoSize;
    int m_geoAlgorithm = -1;
    QImage m_coverImage;
    QSGTexture *m_texture = nullptr;
    QNetworkAccessManager *m_network;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
};

#endif // MESHGRADIENTITEM_H
