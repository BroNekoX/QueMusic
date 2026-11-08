// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef ENGINEHOST_H
#define ENGINEHOST_H

#include <QCoreApplication>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlApplicationEngine;
class QQuickWindow;

// QML 引擎宿主：主界面引擎/窗口的创建、销毁与"轻量模式"状态机。
//
// 进程架构：常驻层单例（AppSettings/Playback/MusicApiService/SessionController/
// SystemTrayManager/SmtcController/桌面部件窗口）随进程存活；主界面 QML 场景
// 由本类按需装配/卸载。轻量模式下引擎与主窗口整体销毁，仅保留后台播放核心；
// 托盘恢复（restore）时重新装配，QML 单例（Options/Style 等）随新引擎重建。
//
// QWindowKit 约束（销毁协议的依据）：
//   - QuickWindowAgent::setup 不接受 null 且每实例仅可 setup 一次
//     （ThirdParty/qwindowkit/src/quick/quickwindowagent.cpp），不支持解绑；
//   - 其 context 内部全为 QPointer，windowAgent 对象析构时会一并移除 winId
//     事件过滤器与平台原生过滤器。因此解绑只能"先销毁 windowAgent 对象，
//     再销毁窗口"，顺序由 main.qml 的 performTeardown() 收尾保证。
//
// 单例模式仿 cpp/LogManager.h：create 工厂 + CppOwnership + QML_ELEMENT/
// QML_SINGLETON，QML 侧按类型名 EngineHost 访问，C++ 侧用 instance()。
class EngineHost : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // 轻量模式是否生效：Inactive（引擎已卸载）与 Destroying（销毁协议执行中）
    // 均视为轻量态，供 UI/调试判断
    Q_PROPERTY(bool lightweightActive READ lightweightActive NOTIFY lightweightActiveChanged FINAL)

public:
    // QML 单例工厂。main.cpp 在引擎装配前提前调用一次（且必须在
    // SessionController::create 之后，构造函数要接它的 windowHiddenToTray）；
    // 之后 QML 首次访问 EngineHost 时复用同一实例。
    static EngineHost *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
    {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(scriptEngine)
        static EngineHost *instance = nullptr;
        if (!instance) {
            instance = new EngineHost(QCoreApplication::instance());
            QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
        }
        return instance;
    }

    // C++ 消费端入口；未提前创建时返回 nullptr
    static EngineHost *instance() { return s_instance; }

    // 不能给 parent 默认值：否则引擎不会调用 create()，会出现两个实例
    // （同 cpp/LogManager.h 的说明）
    explicit EngineHost(QObject *parent);
    ~EngineHost() override;

    // 引擎状态机（全程仅允许 GUI 线程操作；托盘信号/定时器/QML 调用都
    // 经主线程事件循环投递，天然满足）：
    //   Inactive ──createEngine()──▶ Creating ──load 成功──▶ Active
    //   Active   ──destroyEngine()─▶ Destroying ──延迟删除──▶ Inactive
    // Creating/Destroying 是同步调用帧内的过渡态：createEngine/destroyEngine
    // 内部不旋转事件循环，外部触发无法落在过渡态；但 createEngine 的
    // engine.load 等路径可能处理部分事件，仍做守卫——restore/activateNow
    // 对过渡态直接忽略或经 QTimer::singleShot(0) 排队重试（见实现注释）。
    enum EngineState {
        Active,      // QML 引擎与主窗口存活
        Destroying,  // 销毁协议执行中（轻量模式已生效）
        Inactive,    // 引擎已卸载，仅常驻层存活（轻量模式）
        Creating     // 引擎装配中
    };
    Q_ENUM(EngineState)

    // 记录配置目录（createEngine 里 setContextProperty("configDir", ...) 用）。
    // 必须在首次 createEngine() 之前调用。
    void init(const QString &configPath);

    // 装配 QML 引擎并加载 main.qml（原 main.cpp 引擎装配逻辑整体迁入）。
    // 返回 false 表示装配/加载失败（残缺引擎已回收，状态回 Inactive）。
    Q_INVOKABLE bool createEngine();

    // 销毁协议（顺序固定，实现见 .cpp）：
    //   1. invokeMethod(root, "performTeardown") —— QML 侧解绑 Options 引用、
    //      复位 MusicApi 会话态、销毁 windowAgent；
    //   2. root->close() + deleteLater（main.qml onClosing 由 teardownActive
    //      守卫直接放行，不再走 toClosing/quitApp）；
    //   3. engine deleteLater；成员指针清空（QPointer 防悬空）。
    Q_INVOKABLE void destroyEngine();

    // 主窗口（engine.load 后才有；轻量模式下为 null）。托盘"恢复/退出"接线用。
    Q_INVOKABLE QQuickWindow *mainWindow() const;

    bool lightweightActive() const;

signals:
    void lightweightActiveChanged();

private:
    static EngineHost *s_instance;

    QString m_configPath;
    // 引擎无 parent：由本类负责 deleteLater 回收（轻量模式卸载/进程收尾）
    QQmlApplicationEngine *m_engine = nullptr;
    // QPointer：窗口销毁后自动置空，防止悬空访问
    QPointer<QQuickWindow> m_rootWindow;
    EngineState m_state = Inactive;
};

#endif // ENGINEHOST_H
