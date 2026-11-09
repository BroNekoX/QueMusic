// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "UpdateChecker.h"

#include "AppSettings.h"
#include "WarningBuffer.h"

#include <QCoreApplication>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QQmlEngine>
#include <QUrl>

namespace {

// 对应原 QML autoUpdateTimer：interval 3000
constexpr int kDeferredDelayMs = 3000;

// 对应 JS parseInt 的十进制前缀语义：取字符串开头的连续 ASCII 数字转 int。
// 与 QML 的 parseInt(substring(0,3)) 保持一致（如 "80x" → 80，QString::toInt
// 则会失败）；无数字前缀时 ok=false 表达 NaN（NaN 参与比较恒 false，调用方
// 跳过，等价）。刻意不实现 parseInt 的 "0x" 十六进制分支（版本码文件不会是
// 十六进制）。入参为 trimmed().left(3) 的前缀，至多 3 位数字，无溢出之虞。
bool parseIntPrefix(const QString &text, int *value)
{
    bool ok = false;
    int result = 0;
    for (const QChar c : text) {
        const char16_t u = c.unicode();
        if (u < u'0' || u > u'9')
            break;
        result = result * 10 + int(u - u'0');
        ok = true;
    }
    if (value)
        *value = result;
    return ok;
}

} // namespace

UpdateChecker *UpdateChecker::s_instance = nullptr;

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_versionCode(localVersionCode())
{
    m_deferTimer.setInterval(kDeferredDelayMs);
    m_deferTimer.setSingleShot(true);
    connect(&m_deferTimer, &QTimer::timeout, this, &UpdateChecker::checkNow);
}

UpdateChecker::~UpdateChecker()
{
    if (s_instance == this)
        s_instance = nullptr;
}

// 单例工厂：静态实例守卫 + C++ 所有权，QML 侧不可接管（与 LogManager 同一模式）
UpdateChecker *UpdateChecker::create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(scriptEngine)
    static UpdateChecker *instance = nullptr;
    if (!instance) {
        instance = new UpdateChecker(QCoreApplication::instance());
        QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    }
    return instance;
}

void UpdateChecker::startDeferredCheck()
{
    // 开关在启动时判断一次（等价原 QML Qt.callLater 里
    // if(Config.autoUpdate) autoUpdateTimer.start()）。
    // AppSettings::instance() 未创建（self-check 等场景）时直接跳过。
    AppSettings *settings = AppSettings::instance();
    if (!settings || !settings->autoUpdate())
        return;
    m_deferTimer.start();
}

void UpdateChecker::checkNow()
{
    // QNAM 必须在主线程：本单例由 main.cpp / QML 引擎在主线程创建，成员对象同线程。
    // Qt6 的 QNetworkAccessManager 默认跟随重定向，无需额外设置。
    QNetworkRequest request(QUrl(QStringLiteral("https://raw.githubusercontent.com/BroNekoX/QueMusic/main/doc/updater.txt")));
    QNetworkReply *reply = m_nam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        // 原 QML 仅在 readyState===DONE && status===200 时解析，其余静默忽略
        if (reply->error() != QNetworkReply::NoError)
            return;

        // 原 QML：parseInt(responseText.trim().substring(0, 3))
        int remote = 0;
        if (!parseIntPrefix(QString::fromUtf8(reply->readAll()).trimmed().left(3),
                            &remote)
            || remote <= m_versionCode)
            return;

        // 原 QML：mainWarn.tiped("发现新版本 v" + remote + "，请在设置-关于中查看", 1)
        // 结果进常驻层警告缓存；WarningBuffer 未创建（self-check 等场景）时容错跳过
        if (WarningBuffer *buffer = WarningBuffer::instance()) {
            const QString text =
                QStringLiteral("发现新版本 v%1，请在设置-关于中查看").arg(remote);
            buffer->push(text, 1);
        }
    });
}

// 本地版本码：applicationVersion("0.7.2") 去 '.' 后左取 3 位转 int → 72。
// 对应 components/Options.qml 的 readonly property int versionCode: 72，
// 此处从 applicationVersion 动态计算，不硬编码。
int UpdateChecker::localVersionCode()
{
    QString version = QCoreApplication::applicationVersion();
    version.remove(QLatin1Char('.'));
    bool ok = false;
    const int code = version.left(3).toInt(&ok);
    return ok ? code : 0;
}
