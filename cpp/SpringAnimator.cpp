// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "SpringAnimator.h"

#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWindow>
#include <QVariant>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
// duration -> 角频率：让包络在约一个 duration 内衰减到可忽略。
constexpr double kAlpha = 8.0;
} // namespace

SpringAnimator::SpringAnimator(QObject *parent)
    : QObject(parent)
{
    m_delayTimer.setSingleShot(true);
    QObject::connect(&m_delayTimer, &QTimer::timeout, this, &SpringAnimator::beginMotion);
}

void SpringAnimator::setTarget(QQuickItem *target)
{
    if (m_target == target)
        return;
    if (m_target)
        QObject::disconnect(m_target, &QQuickItem::windowChanged, this, &SpringAnimator::onWindowChanged);
    m_target = target;
    if (m_target)
        QObject::connect(m_target, &QQuickItem::windowChanged, this, &SpringAnimator::onWindowChanged);
    attachWindow();
    Q_EMIT targetChanged();
}

void SpringAnimator::setProperty(const QString &name)
{
    if (m_property == name)
        return;
    m_property = name;
    resolveProperty();
    Q_EMIT propertyChanged();
}

void SpringAnimator::setFrom(double value)
{
    if (m_fromSet && m_from == value)
        return;
    m_from = value;
    m_fromSet = true;
    Q_EMIT fromChanged();
}

void SpringAnimator::setTo(double value)
{
    if (m_to == value)
        return;
    m_to = value;
    Q_EMIT toChanged();
    if (m_autoStart)
        start();
}

void SpringAnimator::setDuration(int ms)
{
    if (m_duration == ms)
        return;
    m_duration = ms;
    Q_EMIT durationChanged();
}

void SpringAnimator::setDelay(int ms)
{
    if (m_delay == ms)
        return;
    m_delay = ms;
    Q_EMIT delayChanged();
}

void SpringAnimator::setBounce(double value)
{
    if (m_bounce == value)
        return;
    m_bounce = value;
    Q_EMIT bounceChanged();
}

void SpringAnimator::setAutoStart(bool on)
{
    if (m_autoStart == on)
        return;
    m_autoStart = on;
    Q_EMIT autoStartChanged();
}

void SpringAnimator::setRunning(bool on)
{
    if (on) {
        if (!m_running)
            start();
    } else {
        stop();
    }
}

void SpringAnimator::start()
{
    resolveProperty();

    // 位置取当前值；速度是状态量，换目标时保留，只有显式 from 或属性被外部挪动才清零。
    double begin = m_fromSet ? m_from : readProperty();
    if (!std::isfinite(begin))
        begin = m_value;
    if (m_fromSet || std::abs(begin - m_value) > 0.5)
        m_v = 0.0;

    const double previous = m_value;
    m_value = begin;
    m_peak = std::max(1e-4, std::abs(m_value - m_to));

    if (m_duration <= 0) {
        jumpTo(m_to);
        return;
    }
    m_omega = kAlpha / (double(m_duration) / 1000.0);
    m_zeta = 1.0 - 0.5 * std::clamp(m_bounce, 0.0, 1.0);

    // 首个写入走 QQmlProperty 清掉可能存在的 QML 绑定；之后每帧用缓存的 QMetaProperty 直接写。
    if (m_metaProperty.isValid() && m_target) {
        const QQmlProperty binding(m_target.data(), m_property);
        if (binding.isValid())
            binding.write(QVariant(m_value));
    } else {
        writeProperty(m_value);
    }

    const bool wasRunning = m_running;
    m_running = true;
    if (!wasRunning)
        Q_EMIT runningChanged();
    if (m_value != previous)
        Q_EMIT valueChanged();

    m_delayTimer.stop();
    if (m_delay > 0) {
        m_delayTimer.start(m_delay); // 等待期间不写属性、不刷新
        return;
    }
    beginMotion();
}

void SpringAnimator::beginMotion()
{
    if (!m_running)
        return;
    m_clock.restart();
    m_lastNs = m_clock.nsecsElapsed();
    attachWindow();
}

void SpringAnimator::stop()
{
    m_delayTimer.stop();
    if (!m_running)
        return;
    detach();
    m_running = false;
    Q_EMIT runningChanged();
}

void SpringAnimator::jumpTo(double value)
{
    m_delayTimer.stop();
    if (m_to != value) {
        m_to = value;
        Q_EMIT toChanged();
    }
    const bool changed = m_value != value;
    m_value = value;
    m_v = 0.0;
    writeProperty(m_value);
    if (changed)
        Q_EMIT valueChanged();
    if (m_running) {
        detach();
        m_running = false;
        Q_EMIT runningChanged();
    }
}

void SpringAnimator::attachWindow()
{
    QQuickWindow *window = m_target ? m_target->window() : nullptr;
    if (m_window != window) {
        if (m_window)
            QObject::disconnect(m_window, &QQuickWindow::afterAnimating, this, &SpringAnimator::tick);
        m_window = window;
    }
    if (m_running && m_window) {
        // 幂等：start/stop 会反复进来，UniqueConnection 防止叠加。
        QObject::connect(m_window, &QQuickWindow::afterAnimating, this, &SpringAnimator::tick,
                         Qt::UniqueConnection);
        m_window->update();
    }
}

void SpringAnimator::onWindowChanged()
{
    attachWindow();
}

void SpringAnimator::detach()
{
    if (m_window)
        QObject::disconnect(m_window, &QQuickWindow::afterAnimating, this, &SpringAnimator::tick);
}

void SpringAnimator::tick()
{
    if (!m_running)
        return;

    if (m_clock.elapsed() > qint64(m_duration) * 3) { // 兜底上限
        m_value = m_to;
        m_v = 0.0;
        writeProperty(m_value);
        Q_EMIT valueChanged();
        finish();
        return;
    }

    const qint64 now = m_clock.nsecsElapsed();
    double dt = double(now - m_lastNs) / 1e9;
    m_lastNs = now;
    dt = std::clamp(dt, 0.0, 0.05); // 掉帧时不让一次积分跨太大
    if (dt <= 0.0) {
        if (m_window)
            m_window->update();
        return;
    }

    step(dt);
    writeProperty(m_value);
    Q_EMIT valueChanged();
    m_peak = std::max(m_peak, std::abs(m_value - m_to));

    // 相对静止判定：比例极小(0.01%)，收尾写入目标的跳变远小于一个像素。
    const double settle = std::max(1e-6, 1e-4 * m_peak);
    if (std::abs(m_value - m_to) < settle && std::abs(m_v) < settle * 20.0) {
        m_value = m_to;
        m_v = 0.0;
        writeProperty(m_value);
        Q_EMIT valueChanged();
        finish();
        return;
    }

    // 目标属性不一定可见，显式 update() 保证下一帧仍会走到 afterAnimating。
    if (m_window)
        m_window->update();
}

void SpringAnimator::step(double dt)
{
    const double eps = m_value - m_to;
    if (m_zeta >= 0.999) { // 临界阻尼解析解，无三角函数
        const double e = std::exp(-m_omega * dt);
        const double u = m_v + m_omega * eps;
        m_value = m_to + e * (eps + u * dt);
        m_v = e * (m_v - m_omega * u * dt);
        return;
    }

    // 欠阻尼解析解，无条件稳定、与帧率无关
    const double wd = m_omega * std::sqrt(1.0 - m_zeta * m_zeta);
    const double e = std::exp(-m_zeta * m_omega * dt);
    const double ct = std::cos(wd * dt);
    const double st = std::sin(wd * dt);
    const double b = (m_v + m_zeta * m_omega * eps) / wd;
    const double nextEps = e * (eps * ct + b * st);
    m_value = m_to + nextEps;
    m_v = -m_zeta * m_omega * nextEps + e * wd * (-eps * st + b * ct);
}

void SpringAnimator::finish()
{
    detach();
    m_running = false;
    Q_EMIT runningChanged();
    Q_EMIT finished();
}

bool SpringAnimator::resolveProperty()
{
    m_metaProperty = QMetaProperty();
    if (!m_target || m_property.isEmpty())
        return false;
    const QQmlProperty property(m_target.data(), m_property);
    if (!property.isValid() || !property.isWritable())
        return false;
    m_metaProperty = property.property();
    return m_metaProperty.isValid();
}

double SpringAnimator::readProperty() const
{
    if (!m_metaProperty.isValid() || !m_target)
        return std::numeric_limits<double>::quiet_NaN();
    bool ok = false;
    const double value = m_metaProperty.read(m_target.data()).toDouble(&ok);
    return ok ? value : std::numeric_limits<double>::quiet_NaN();
}

void SpringAnimator::writeProperty(double value)
{
    if (!m_metaProperty.isValid() || !m_target)
        return;
    m_metaProperty.write(m_target.data(), QVariant(value));
}
