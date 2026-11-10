// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
// 弹簧动画器：把目标属性用阻尼弹簧推到 to。错峰、融合等编排在 QML，这里只管怎么动。
// 每帧挂 QQuickWindow::afterAnimating 解一次运动方程，位置与速度都是状态量，
// 换目标时保留速度自然弯过去。duration 换算成角频率，bounce 换算成阻尼比。
#pragma once

#include <QElapsedTimer>
#include <QMetaProperty>
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class SpringAnimator : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QQuickItem *target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString property READ property WRITE setProperty NOTIFY propertyChanged)
    Q_PROPERTY(double from READ from WRITE setFrom NOTIFY fromChanged)
    Q_PROPERTY(double to READ to WRITE setTo NOTIFY toChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration NOTIFY durationChanged)
    Q_PROPERTY(int delay READ delay WRITE setDelay NOTIFY delayChanged)
    Q_PROPERTY(double bounce READ bounce WRITE setBounce NOTIFY bounceChanged)
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(double value READ value NOTIFY valueChanged)

public:
    explicit SpringAnimator(QObject *parent = nullptr);

    QQuickItem *target() const { return m_target; }
    void setTarget(QQuickItem *target);

    QString property() const { return m_property; }
    void setProperty(const QString &name);

    double from() const { return m_from; }
    void setFrom(double value);

    double to() const { return m_to; }
    void setTo(double value);

    int duration() const { return m_duration; }
    void setDuration(int ms);

    int delay() const { return m_delay; }
    void setDelay(int ms);

    double bounce() const { return m_bounce; }
    void setBounce(double value);

    bool autoStart() const { return m_autoStart; }
    void setAutoStart(bool on);

    bool running() const { return m_running; }
    void setRunning(bool on);

    double value() const { return m_value; }

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void jumpTo(double value);

Q_SIGNALS:
    void targetChanged();
    void propertyChanged();
    void fromChanged();
    void toChanged();
    void durationChanged();
    void delayChanged();
    void bounceChanged();
    void autoStartChanged();
    void runningChanged();
    void valueChanged();
    void finished();

private:
    void beginMotion();
    void tick();
    void step(double dt);
    void attachWindow();
    void onWindowChanged();
    void detach();
    void finish();
    bool resolveProperty();
    double readProperty() const;
    void writeProperty(double value);

    QPointer<QQuickItem> m_target;
    QPointer<QQuickWindow> m_window;
    QString m_property;
    QMetaProperty m_metaProperty;

    double m_from = 0.0;
    bool m_fromSet = false;
    double m_to = 0.0;
    double m_value = 0.0;
    double m_v = 0.0;      // 弹簧速度
    double m_peak = 1.0;   // 本段离目标的最大偏移，用于相对静止判定

    int m_duration = 460;
    int m_delay = 0;
    double m_bounce = 0.3;
    bool m_autoStart = true;
    bool m_running = false;

    double m_omega = 17.0; // start() 里由 duration 换算
    double m_zeta = 0.85;  // start() 里由 bounce 换算

    QTimer m_delayTimer;
    QElapsedTimer m_clock;
    qint64 m_lastNs = 0;
};
