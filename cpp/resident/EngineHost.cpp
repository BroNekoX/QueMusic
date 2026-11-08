// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "EngineHost.h"

#include <QDebug>
#include <QMetaObject>
#include <QUrl>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickWindow>
#include <QWKQuick/qwkquickglobal.h>

// qmltyperegistration 生成（Qt 构建产物）：注册 QueMusic 模块的 QML_ELEMENT 类型。
// 进程级只需调用一次（重复调用会重复注册），createEngine 内用静态 bool 守卫。
extern void qml_register_types_QueMusic();

EngineHost *EngineHost::s_instance = nullptr;

EngineHost::EngineHost(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
}

EngineHost::~EngineHost()
{
    // 进程收尾兜底：正常退出时若引擎仍存活（DeferredDelete 未及处理），
    // 直接同步删除——引擎析构会连带销毁根窗口（root object 归引擎所有）
    if (m_engine) {
        delete m_engine;
        m_engine = nullptr;
    }
    m_rootWindow = nullptr;
    if (s_instance == this)
        s_instance = nullptr;
}

void EngineHost::init(const QString &configPath)
{
    m_configPath = configPath;
}

// 装配 QML 引擎（原 main.cpp 引擎装配段整体迁入）：
//   new QQmlApplicationEngine → qml_register_types_QueMusic（一次性）→
//   addImportPath → QWK::registerTypes（内部自幂等）→ setContextProperty →
//   engine.load → rootObjects 取主窗口。
bool EngineHost::createEngine()
{
    // 状态守卫：仅 Inactive 可创建；Active 幂等返回成功；过渡态（理论上
    // 只会出现在 load 内部处理事件期间）拒绝重入
    if (m_state != Inactive) {
        qWarning() << "[EngineHost] createEngine 在非 Inactive 状态被调用，忽略"
                   << "（当前状态：" << m_state << "）";
        return m_state == Active;
    }
    m_state = Creating;

    m_engine = new QQmlApplicationEngine();

    // 进程级只注册一次 QML 类型（静态守卫；重建引擎时无需也无法重复注册）
    static bool typesRegistered = false;
    if (!typesRegistered) {
        qml_register_types_QueMusic();
        typesRegistered = true;
    }
    m_engine->addImportPath(QStringLiteral("qrc:/"));
    // QWindowKit 类型注册：qwkquickglobal.cpp 内部有 static once 守卫，
    // 进程级幂等，重建引擎时重复调用安全，无需外层再加守卫
    QWK::registerTypes(m_engine);

    // 注入 QML 上下文属性（各单例定义见 cpp/AppModels.h）
    m_engine->rootContext()->setContextProperty(QStringLiteral("configDir"), m_configPath);

    m_engine->load(QUrl(QStringLiteral("qrc:/QueMusic/main.qml")));

    // 主窗口（engine.load 后才存在）：QPointer 持有，窗口销毁后自动置空
    m_rootWindow = qobject_cast<QQuickWindow *>(m_engine->rootObjects().value(0));
    if (!m_rootWindow) {
        qWarning() << "[EngineHost] engine.load 后未取得主窗口，回收残缺引擎";
        m_engine->deleteLater();
        m_engine = nullptr;
        m_state = Inactive;
        return false;
    }

    m_state = Active;
    emit lightweightActiveChanged();
    return true;
}

// 销毁协议（顺序固定，全程 GUI 线程）：
//   1. performTeardown()：QML 侧解绑 Options 引用、复位 MusicApi 会话态、
//      最后 windowAgent.destroy()（QWindowKit 不支持解绑，只能先销毁
//      windowAgent 对象——其析构会移除 winId 事件过滤器与平台原生过滤器）；
//   2. 窗口 close + deleteLater（onClosing 由 main.qml teardownActive 守卫
//      直接放行，避免销毁路径误触 toClosing/quitApp）；
//   3. 引擎 deleteLater；成员指针清空（QPointer 防悬空）。
//   deleteLater 按入队顺序处理：窗口先于引擎销毁，引擎析构不再二次删除窗口。
void EngineHost::destroyEngine()
{
    // 状态保护：Inactive 无可销毁；Creating/Destroying 拒绝重入
    //（activateNow/定时器重复触发时在此被挡下）
    if (m_state != Active) {
        qWarning() << "[EngineHost] destroyEngine 在非 Active 状态被调用，忽略"
                   << "（当前状态：" << m_state << "）";
        return;
    }
    // 进入销毁协议：Destroying 即视为轻量态
    m_state = Destroying;
    emit lightweightActiveChanged();

    if (QQuickWindow *root = m_rootWindow.data()) {
        // 1. QML 侧清理与解绑（windowAgent.destroy() 在该函数末尾完成）。
        //    main.qml 未实现该钩子时 invokeMethod 返回 false，仅告警不中断。
        if (!QMetaObject::invokeMethod(root, "performTeardown"))
            qWarning() << "[EngineHost] main.qml 未实现 performTeardown()（销毁协议 QML 钩子缺失）";
        // 2. 窗口关闭 + 延迟删除
        root->close();
        root->deleteLater();
    }
    // 3. 引擎延迟删除
    if (m_engine)
        m_engine->deleteLater();
    m_engine = nullptr;
    m_rootWindow = nullptr;
    m_state = Inactive;
}

QQuickWindow *EngineHost::mainWindow() const
{
    return m_rootWindow.data();
}

bool EngineHost::lightweightActive() const
{
    return m_state == Inactive || m_state == Destroying;
}
