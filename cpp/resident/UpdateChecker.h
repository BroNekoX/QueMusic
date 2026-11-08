// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// 静默更新检查：把 main.qml 的 silentUpdateCheck() + autoUpdateTimer 移植到
// 纯 C++ 常驻层，行为与 QML 保持一致：
//   - GET https://raw.githubusercontent.com/BroNekoX/QueMusic/main/doc/updater.txt
//     （Qt6 QNetworkAccessManager 默认跟随重定向）
//   - 响应体去首尾空白后取前 3 字符转整数（对应 JS parseInt），与本地版本码比较，
//     更大则提示"发现新版本 v<remote>，请在设置-关于中查看"（type 1）
//   - 本地版本码：applicationVersion（main.cpp 设 "0.7.2"）去 '.' 后左取 3 位
//     转 int（→ 72），动态计算不硬编码（对应 components/Options.qml versionCode）
//   - 网络失败 / 非 200 / 非数字响应：静默忽略（与 QML 只在 DONE && 200 时处理一致）
// 检查结果统一进 WarningBuffer（实时 warnedArrived 弹出 + 场景重建时重放）；
// WarningBuffer 未创建（self-check 等场景）时容错跳过。
//
// 单例模式仿 cpp/LogManager.h / WarningBuffer：create 工厂 + CppOwnership +
// QML_ELEMENT/QML_SINGLETON，QML 侧按类型名 UpdateChecker 访问。
class UpdateChecker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // QML 单例工厂。集成阶段 main.cpp 提前调用一次；之后 QML 首次访问
    // UpdateChecker 时复用同一实例（必须主线程创建：内含 QNetworkAccessManager）。
    static UpdateChecker *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine);

    // C++ 消费端入口；未提前创建时返回 nullptr
    static UpdateChecker *instance() { return s_instance; }

    // 不能给 parent 默认值：否则引擎不会调用 create()，会出现两个实例
    // （同 cpp/AppModels.h / LogManager 的说明）
    explicit UpdateChecker(QObject *parent);
    ~UpdateChecker() override;

    // 启动延迟检查：3 秒单发定时器到点后 checkNow()（对应 autoUpdateTimer 3000ms）。
    // 开关判断放在本函数内部（等价原 QML 启动时 if(Config.autoUpdate) timer.start()），
    // 由集成方在启动阶段调用；刻意不 connect AppSettings::autoUpdateChanged ——
    // QML 原逻辑也不在运行中重挂定时器（设置页改动后下次启动才生效），
    // 设置页手动"检查更新"将来直接调 checkNow() 即可。
    Q_INVOKABLE void startDeferredCheck();

    // 立即检查（设置页"检查更新"可复用）
    Q_INVOKABLE void checkNow();

private:
    static int localVersionCode();

    static UpdateChecker *s_instance;

    QNetworkAccessManager m_nam;   // 成员对象：随本类销毁（同 Playback 的 QTimer 成员风格）
    QTimer m_deferTimer;
    int m_versionCode = 0;
};

#endif // UPDATECHECKER_H
