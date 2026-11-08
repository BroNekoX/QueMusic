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

#include "AppSettings.h"
#include "SystemTrayManager.h"
#include "SessionController.h"

// qmltyperegistration 生成（Qt 构建产物）：注册 QueMusic 模块的 QML_ELEMENT 类型。
// 进程级只需调用一次（重复调用会重复注册），createEngine 内用静态 bool 守卫。
extern void qml_register_types_QueMusic();

EngineHost *EngineHost::s_instance = nullptr;

// 构造时机（main.cpp 保证）：SessionController::create / AppSettings::create /
// SystemTrayManager::create 均已先行，instance() 非空；若调用顺序变化，
// 下方判空会跳过接线（此时轻量模式自动进入不可用，手动入口仍有效）。
EngineHost::EngineHost(QObject *parent)
    : QObject(parent)
{
    s_instance = this;

    // 自动进入定时器：单发，到点执行 activateNow()（轻量模式下已无窗口，
    // 由状态机守卫忽略）
    m_autoTimer.setSingleShot(true);
    connect(&m_autoTimer, &QTimer::timeout, this, &EngineHost::activateNow);

    // 轻量模式自动进入触发源：窗口隐藏到托盘 → 按开关与延迟启动计时
    if (SessionController *session = SessionController::instance())
        connect(session, &SessionController::windowHiddenToTray,
                this, &EngineHost::restartAutoTimer);

    // 设置联动：开关关闭 → 停掉尚未触发的计时；延迟秒数变更 → 仅当计时已在
    // 跑（窗口已隐藏、倒计时中）才按新间隔重启——不能因改设置凭空启动计时，
    // 否则可见窗口会在延迟后被误销毁
    if (AppSettings *settings = AppSettings::instance()) {
        connect(settings, &AppSettings::lightweightAutoEnabledChanged, this, [this] {
            if (!AppSettings::instance()->lightweightAutoEnabled())
                m_autoTimer.stop();
        });
        connect(settings, &AppSettings::lightweightDelaySecondsChanged, this, [this] {
            if (m_autoTimer.isActive())
                m_autoTimer.start(qMax(1, AppSettings::instance()->lightweightDelaySeconds())
                                      * 1000);
        });
    }
}

// 窗口隐藏到托盘 → 按实时配置（重）启单发计时。
// start() 对同一信号重复到达（反复隐藏窗口）自动重启计时。
void EngineHost::restartAutoTimer()
{
    AppSettings *settings = AppSettings::instance();
    if (!settings || !settings->lightweightAutoEnabled())
        return;
    m_autoTimer.start(qMax(1, settings->lightweightDelaySeconds()) * 1000);
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
    // 销毁后无需再计时（自动进入已达成/被手动路径取代）
    m_autoTimer.stop();
}

// 立即进入轻量模式：与 WindowControls.hideToTray 同款隐藏三步（记录形态 →
// 隐藏 → 托盘气泡），随后执行销毁协议。
void EngineHost::activateNow()
{
    // 过渡态直接忽略：Creating/Destroying 期间不重复触发（销毁协议不可重入）；
    // Inactive（已是轻量模式）同样忽略——定时器残留触发即落在这里
    if (m_state != Active) {
        qWarning() << "[EngineHost] activateNow 在非 Active 状态被调用，忽略"
                   << "（当前状态：" << m_state << "）";
        return;
    }
    if (m_rootWindow && m_rootWindow->isVisible()) {
        if (SessionController *session = SessionController::instance())
            session->markHidden(m_rootWindow);
        m_rootWindow->hide();
        if (SystemTrayManager *tray = SystemTrayManager::instance())
            tray->notifyTrayHidden();
    }
    destroyEngine();
}

// 恢复主界面（托盘恢复路径统一入口）：轻量模式下重新装配引擎，Active 时
// 仅按记忆形态恢复窗口。
void EngineHost::restore()
{
    // 主动恢复视为用户取消待触发的自动轻量化计时（否则恢复后窗口会被
    // 到点的定时器再次隐藏）
    m_autoTimer.stop();

    switch (m_state) {
    case Active:
        if (SessionController *session = SessionController::instance())
            session->restoreWindow(m_rootWindow);
        break;
    case Inactive:
        createEngine();
        break;
    case Creating:
    case Destroying:
        // 过渡态排队重试：单发 0ms 重入一次，重试时销毁协议已收尾
        //（Inactive）或装配已完成（Active），不会再无限排队
        QTimer::singleShot(0, this, &EngineHost::restore);
        break;
    }
}

QQuickWindow *EngineHost::mainWindow() const
{
    return m_rootWindow.data();
}

bool EngineHost::lightweightActive() const
{
    return m_state == Inactive || m_state == Destroying;
}
