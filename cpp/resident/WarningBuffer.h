// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef WARNINGBUFFER_H
#define WARNINGBUFFER_H

#include <QObject>
#include <QVariantList>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// 警告环形缓存：常驻 C++ 层收集 Playback / MusicApiService 的 warned 警告。
// 背景：轻量模式下 QML 场景（QWarn 提示条）会被销毁，直接弹的提示会丢；
// 本类缓存最近 8 条（text + type），QML 场景重建时用 recent() 重放，
// 实时警告用 warnedArrived 信号监听（场景内 QWarn 可直接 connect）。
//
// 单例模式仿 cpp/LogManager.h：create 工厂 + CppOwnership + QML_ELEMENT/
// QML_SINGLETON，QML 侧按类型名 WarningBuffer 访问。另提供 instance()
// （仿 AppSettings::instance()）供 UpdateChecker 等 C++ 消费端判空访问。
class WarningBuffer : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // 环形缓存容量：最近 8 条
    static constexpr int kCapacity = 8;

    // QML 单例工厂。集成阶段 main.cpp 在 Playback::create / MusicApiService::create
    // 之后提前调用一次（构造函数会 connect 两者的 warned 信号）；之后 QML 首次
    // 访问 WarningBuffer 时复用同一实例。
    static WarningBuffer *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine);

    // C++ 消费端入口；未提前创建时返回 nullptr
    static WarningBuffer *instance() { return s_instance; }

    // 不能给 parent 默认值：否则引擎不会调用 create()，会出现两个实例
    // （同 cpp/AppModels.h / LogManager 的说明）
    explicit WarningBuffer(QObject *parent);
    ~WarningBuffer() override;

    // 重放数据源（QML 场景重建时调用）：按时间序（旧→新）返回最近警告，
    // 元素为 QVariantMap{text: QString, type: int}
    Q_INVOKABLE QVariantList recent() const;

    // 追加一条警告到环形缓存并广播 warnedArrived
    Q_INVOKABLE void push(const QString &text, int type);

signals:
    // 实时警告：集成后 QML 场景内的 QWarn 可直接监听弹提示
    // （签名与 Playback::warned / MusicApiService::warned 一致，仅改了信号名）
    void warnedArrived(const QString &text, int type);

private:
    void onWarned(const QString &text, int type);

    struct Entry {
        QString text;
        int type = 0;
    };

    static WarningBuffer *s_instance;

    QVector<Entry> m_entries;
};

#endif // WARNINGBUFFER_H
