// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// AccountManager — 在线音乐账号登录态管理（网易云 / 酷狗）
//
// 设计目标（与 NeriPlayer 的"账号即能力"思路一致）：
//  - 不构建公共云端服务，用户用自己账号通过官方 App 扫码授权
//  - 登录态（Cookie / Token）只保存在本机，请求都带上用户自己的身份
//  - 退出登录即删除本地登录态
#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QNetworkCookieJar>
#include <QObject>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkReply;

#include <QAtomicInt>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

class QJSEngine;

class ApiHelper;

class AccountManager : public QObject
{
    Q_OBJECT
    // QML 单例：QML 侧按类型名访问（如 AccountManager.neteaseLoggedIn）。
    // 只有这样 qmlcachegen 才能把相关绑定编译成 C++；经由对象属性间接访问会被判定为
    // shadowable base type 而拒绝编译（实测）。
    QML_ELEMENT
    QML_SINGLETON
    // 网易云
    Q_PROPERTY(bool neteaseLoggedIn READ isNeteaseLoggedIn NOTIFY neteaseLoginChanged)
    Q_PROPERTY(QString neteaseNickname READ neteaseNickname NOTIFY neteaseLoginChanged)
    Q_PROPERTY(QString neteaseAvatar READ neteaseAvatar NOTIFY neteaseLoginChanged)
    Q_PROPERTY(QString neteaseCookie READ neteaseCookie NOTIFY neteaseLoginChanged)
    Q_PROPERTY(QString neteaseQrText READ neteaseQrText NOTIFY neteaseQrChanged)
    Q_PROPERTY(int neteaseQrState READ neteaseQrState NOTIFY neteaseQrChanged)
    Q_PROPERTY(QString neteaseQrMessage READ neteaseQrMessage NOTIFY neteaseQrChanged)
    // 酷狗
    Q_PROPERTY(bool kugouLoggedIn READ isKugouLoggedIn NOTIFY kugouLoginChanged)
    Q_PROPERTY(QString kugouNickname READ kugouNickname NOTIFY kugouLoginChanged)
    Q_PROPERTY(QString kugouAvatar READ kugouAvatar NOTIFY kugouLoginChanged)
    Q_PROPERTY(QString kugouCookie READ kugouCookie NOTIFY kugouLoginChanged)
    Q_PROPERTY(QString kugouQrText READ kugouQrText NOTIFY kugouQrChanged)
    Q_PROPERTY(int kugouQrState READ kugouQrState NOTIFY kugouQrChanged)
    Q_PROPERTY(QString kugouQrMessage READ kugouQrMessage NOTIFY kugouQrChanged)

public:
    // QML 单例工厂。main.cpp 会提前调用一次以保证账号初始化时序；
    // 之后 QML 首次访问 AccountManager 时复用同一实例。
    static AccountManager *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
    {
        Q_UNUSED(scriptEngine)
        static AccountManager *instance = nullptr;
        if (!instance)
            instance = new AccountManager(qmlEngine);
        return instance;
    }

    // 二维码状态码（QML 显示用）
    enum QrState {
        QrWaiting = 0,   // 等待扫码
        QrScanned = 1,   // 已扫码，待确认
        QrSuccess = 2,   // 登录成功
        QrExpired = 3,   // 二维码过期
        QrError = 4      // 网络/其他错误
    };
    Q_ENUM(QrState)

    // 不能给 parent 默认值：否则引擎会走"默认构造"分支而不调用 create()，
    // 导致 main.cpp 里提前创建的实例与 QML 侧看到的实例不是同一个（详见 cpp/AppModels.h 说明）
    explicit AccountManager(QObject *parent);
    ~AccountManager() override;

    bool isNeteaseLoggedIn() const { return m_neteaseLoggedIn; }
    // 供在线接口层使用：始终带客户端身份（稳定 deviceId + os=pc），未登录时只带身份
    QString neteaseApiCookie() const { return mergeNeteaseIdentity(m_neteaseCookie); }
    QString neteaseNickname() const { return m_neteaseNickname; }
    QString neteaseAvatar() const { return m_neteaseAvatar; }
    QString neteaseCookie() const { return m_neteaseCookie; }
    QString neteaseQrText() const { return m_neteaseQrText; }
    int neteaseQrState() const { return m_neteaseQrState; }
    QString neteaseQrMessage() const { return m_neteaseQrMessage; }

    bool isKugouLoggedIn() const { return m_kugouLoggedIn; }
    QString kugouNickname() const { return m_kugouNickname; }
    QString kugouAvatar() const { return m_kugouAvatar; }
    QString kugouCookie() const { return m_kugouCookie; }
    QString kugouMid() const { return m_kugouMid; }
    QString kugouDfid() const { return m_kugouDfid; }
    QString kugouQrText() const { return m_kugouQrText; }
    int kugouQrState() const { return m_kugouQrState; }
    QString kugouQrMessage() const { return m_kugouQrMessage; }

    Q_INVOKABLE void loadPersisted(); // 启动时恢复登录态

public slots:
    void startNeteaseQrLogin();
    void cancelNeteaseQrLogin();
    void logoutNetease();

    void startKugouQrLogin();
    void cancelKugouQrLogin();
    void logoutKugou();

    // 浏览器登录（内嵌 WebView）完成后，由 QML 回填 Cookie / 用户信息
    Q_INVOKABLE void setNeteaseBrowserCookie(const QString &cookie, const QString &nickname,
                                             const QString &avatar);
    Q_INVOKABLE void setKugouBrowserCookie(const QString &cookie, const QString &nickname,
                                           const QString &avatar);

    // 备用登录入口：扫码被网易云风控拦截（手机端提示"环境异常"）时使用
    Q_INVOKABLE void loginNeteaseWithCookie(const QString &cookie); // 粘贴含 MUSIC_U 的 Cookie
    Q_INVOKABLE void sendNeteaseCaptcha(const QString &phone);      // 发送短信验证码
    Q_INVOKABLE void loginNeteaseWithCellphone(const QString &phone, const QString &captcha);

signals:
    void neteaseLoginChanged();
    void kugouLoginChanged();
    void neteaseQrChanged();
    void kugouQrChanged();
    // type: 0 普通 / 1 成功 / 2 警告 / 3 错误（对接主窗口气泡提示）
    void message(const QString &text, int type);

private:
    void kugouGet(const QString &baseUrl, const QString &path, const QJsonObject &customParams);
    void pollNetease();
    void pollKugou();

    void onNeteaseProfile(QNetworkReply *reply);

    // 网易云二维码登录（基于 QCloudMusicApi 的 login_qr_* 接口，后台线程执行避免卡 UI）
    void neteaseFetchQrWorker();
    void neteasePollWorker(const QString &key);
    Q_INVOKABLE void onNeteaseQrFetched(const QString &unikey, const QString &qrurl);
    Q_INVOKABLE void onNeteasePollResult(int code, const QString &cookie,
                                        const QString &nickname, const QString &msg);
    Q_INVOKABLE void onNeteaseFetchError(const QString &msg);
    void storeNeteaseCookieString(const QString &cookieStr);
    // 客户端身份：稳定 deviceId + os=pc（与 weapi 的 PC UA、网页扫码 type=1 自洽）。
    // SDK 默认每次启动随机生成 deviceId、且默认 iPhone App 身份，容易被判为异常环境。
    QString ensureNeteaseDeviceId();
    QString mergeNeteaseIdentity(const QString &cookie) const;
    void verifyNeteaseLogin(); // 用账号信息接口校验当前 cookie 是否有效
    void onKugouKey(QNetworkReply *reply);
    void onKugouPoll(QNetworkReply *reply);

    void writeNeteaseCookiesToJar(const QString &cookie);
    void clearLoginSettings(const QString &group);
    void persistNetease();
    void persistKugou();
    void loadNetease();
    void loadKugou();
    void setNeteaseQr(int state, const QString &msg, const QString &qrText = QString());
    void setKugouQr(int state, const QString &msg, const QString &qrText = QString());

    // ---- 状态 ----
    bool m_neteaseLoggedIn = false;
    bool m_kugouLoggedIn = false;
    QString m_neteaseNickname;
    QString m_neteaseAvatar;
    QString m_kugouNickname;
    QString m_kugouAvatar;
    QString m_neteaseCookie;
    QString m_kugouCookie;

    // 网易云客户端身份与扫码登录状态控制
    QString m_neteaseDeviceId;        // 持久化的设备号（Account.ini: Netease/deviceId）
    int m_neteasePollFails = 0;       // 连续轮询失败次数（风控时及时停止，避免刷分）
    qint64 m_neteaseQrDeadlineMs = 0; // 二维码整体有效期
    bool m_neteaseCookieVerifying = false; // 正在校验手工填入的 Cookie

    QString m_neteaseQrText;
    int m_neteaseQrState = QrWaiting;
    QString m_neteaseQrMessage;
    QString m_kugouQrText;
    int m_kugouQrState = QrWaiting;
    QString m_kugouQrMessage;

    // ---- 网络 ----
    QNetworkAccessManager *m_nam = nullptr;
    QNetworkCookieJar *m_jar = nullptr;
    QTimer *m_neteasePollTimer = nullptr;
    QTimer *m_kugouPollTimer = nullptr;
    QString m_neteaseUnikey;
    QString m_kugouKey;

    // 网易云登录走 QCloudMusicApi（login_qr_* 接口），由其内部维护 cookie
    ApiHelper *m_api = nullptr;
    QAtomicInt m_neteaseBusy{0};      // 防止并发调用阻塞的 invoke
    QAtomicInt m_neteaseCancelled{0}; // 取消/退出登录时置位

    // 酷狗设备信息
    QString m_kugouGuid;
    QString m_kugouMid;
    QString m_kugouDfid;

    QString m_configPath;
};

#endif // ACCOUNTMANAGER_H
