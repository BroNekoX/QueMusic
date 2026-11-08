// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "WarningBuffer.h"

#include "MusicApiService.h"
#include "playback/Playback.h"

#include <QCoreApplication>
#include <QQmlEngine>

WarningBuffer *WarningBuffer::s_instance = nullptr;

WarningBuffer::WarningBuffer(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    // 两个 warned 信号签名一致：void warned(const QString &text, int type)
    // （cpp/playback/Playback.h、api/MusicApiService.h）。
    // - Playback.h 没有静态 instance() 访问器，复用其 QML 单例工厂 Playback::create
    //   （幂等，已存在则返回同一实例）；
    // - MusicApiService::instance() 不做懒创建（未创建时返回 nullptr）：
    //   集成阶段 main.cpp 先建 api 再建本类，顺序无虞；未就绪时跳过连接。
    if (Playback *playback = Playback::create(nullptr, nullptr))
        connect(playback, &Playback::warned, this, &WarningBuffer::onWarned);
    if (MusicApiService *api = MusicApiService::instance())
        connect(api, &MusicApiService::warned, this, &WarningBuffer::onWarned);
}

WarningBuffer::~WarningBuffer()
{
    if (s_instance == this)
        s_instance = nullptr;
}

// 单例工厂：静态实例守卫 + C++ 所有权，QML 侧不可接管（与 LogManager 同一模式）
WarningBuffer *WarningBuffer::create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(scriptEngine)
    static WarningBuffer *instance = nullptr;
    if (!instance) {
        instance = new WarningBuffer(QCoreApplication::instance());
        QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    }
    return instance;
}

QVariantList WarningBuffer::recent() const
{
    QVariantList list;
    list.reserve(m_entries.size());
    for (const Entry &entry : m_entries) {
        list.append(QVariantMap{
            {QStringLiteral("text"), entry.text},
            {QStringLiteral("type"), entry.type}
        });
    }
    return list;
}

void WarningBuffer::push(const QString &text, int type)
{
    // 环形缓存：超出容量丢最旧的一条
    if (m_entries.size() >= kCapacity)
        m_entries.removeFirst();
    m_entries.append(Entry{text, type});
    emit warnedArrived(text, type);
}

void WarningBuffer::onWarned(const QString &text, int type)
{
    push(text, type);
}
