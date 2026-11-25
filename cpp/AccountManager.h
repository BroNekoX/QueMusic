// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// AccountManager — 在线音乐账号登录态管理
//
#ifndef ACCOUNTMANAGER_H
#define ACCOUNTMANAGER_H

#include <QNetworkCookieJar>
#include <QObject>
#include <QThreadPool>
#include <QTimer>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

#include <QAtomicInt>
#include <QCoreApplication>
#include <QQmlEngine>
#include <QtQml/qqmlregistration.h>

class QJSEngine;

class ApiHelper;

class AccountManager : public QObject
{
    Q_OBJECT
    // QML 单例：必须按类型名访问，qmlcachegen 才能编译相关绑定
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool neteaseLoggedIn READ isNeteaseLoggedIn NOTIFY neteaseLoginChanged FINAL)
    Q_PROPERTY(QString neteaseNickname READ neteaseNickname NOTIFY neteaseLoginChanged FINAL)
    Q_PROPERTY(QString neteaseAvatar READ neteaseAvatar NOTIFY neteaseLoginChanged FINAL)
    Q_PROPERTY(QString neteaseCookie READ neteaseCookie NOTIFY neteaseLoginChanged FINAL)
    Q_PROPERTY(QString neteaseQrText READ neteaseQrText NOTIFY neteaseQrChanged FINAL)
    Q_PROPERTY(int neteaseQrState READ neteaseQrState NOTIFY neteaseQrChanged FINAL)
    Q_PROPERTY(QString neteaseQrMessage READ neteaseQrMessage NOTIFY neteaseQrChanged FINAL)
    // 账号安全风险拦截时网易云返回的验证页（需用官方 App 扫码完成验证）
    Q_PROPERTY(QString neteaseVerifyUrl READ neteaseVerifyUrl NOTIFY neteaseQrChanged FINAL)
    Q_PROPERTY(bool kugouLoggedIn READ isKugouLoggedIn NOTIFY kugouLoginChanged FINAL)
    Q_PROPERTY(QString kugouNickname READ kugouNickname NOTIFY kugouLoginChanged FINAL)
    Q_PROPERTY(QString kugouAvatar READ kugouAvatar NOTIFY kugouLoginChanged FINAL)
    Q_PROPERTY(QString kugouCookie READ kugouCookie NOTIFY kugouLoginChanged FINAL)
    Q_PROPERTY(QString kugouQrText READ kugouQrText NOTIFY kugouQrChanged FINAL)
    Q_PROPERTY(int kugouQrState READ kugouQrState NOTIFY kugouQrChanged FINAL)
    Q_PROPERTY(QString kugouQrMessage READ kugouQrMessage NOTIFY kugouQrChanged FINAL)
    Q_PROPERTY(bool spotifyLoggedIn READ isSpotifyLoggedIn NOTIFY spotifyLoginChanged FINAL)
    Q_PROPERTY(QString spotifyNickname READ spotifyNickname NOTIFY spotifyLoginChanged FINAL)
    Q_PROPERTY(QString spotifyAvatar READ spotifyAvatar NOTIFY spotifyLoginChanged FINAL)
    Q_PROPERTY(QString spotifyLoginMessage READ spotifyLoginMessage NOTIFY spotifyLoginChanged FINAL)
    Q_PROPERTY(bool appleLoggedIn READ isAppleLoggedIn NOTIFY appleLoginChanged FINAL)
    Q_PROPERTY(QString appleNickname READ appleNickname NOTIFY appleLoginChanged FINAL)
    Q_PROPERTY(QString appleAvatar READ appleAvatar NOTIFY appleLoginChanged FINAL)
    Q_PROPERTY(QString appleLoginMessage READ appleLoginMessage NOTIFY appleLoginChanged FINAL)
    Q_PROPERTY(bool bilibiliLoggedIn READ isBilibiliLoggedIn NOTIFY bilibiliLoginChanged FINAL)
    Q_PROPERTY(QString bilibiliNickname READ bilibiliNickname NOTIFY bilibiliLoginChanged FINAL)
    Q_PROPERTY(QString bilibiliAvatar READ bilibiliAvatar NOTIFY bilibiliLoginChanged FINAL)
    Q_PROPERTY(QString bilibiliCookie READ bilibiliCookie NOTIFY bilibiliLoginChanged FINAL)
    Q_PROPERTY(QString bilibiliQrText READ bilibiliQrText NOTIFY bilibiliQrChanged FINAL)
    Q_PROPERTY(int bilibiliQrState READ bilibiliQrState NOTIFY bilibiliQrChanged FINAL)
    Q_PROPERTY(QString bilibiliQrMessage READ bilibiliQrMessage NOTIFY bilibiliQrChanged FINAL)
    Q_PROPERTY(bool qqLoggedIn READ isQQLoggedIn NOTIFY qqLoginChanged FINAL)
    Q_PROPERTY(QString qqNickname READ qqNickname NOTIFY qqLoginChanged FINAL)
    Q_PROPERTY(QString qqAvatar READ qqAvatar NOTIFY qqLoginChanged FINAL)
    Q_PROPERTY(QString qqLoginMessage READ qqLoginMessage NOTIFY qqLoginChanged FINAL)

public:
    // QML 单例工厂；main.cpp 提前调用一次保证初始化时序
    static AccountManager *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
    {
        Q_UNUSED(qmlEngine)
        Q_UNUSED(scriptEngine)
        static AccountManager *instance = nullptr;
        if (!instance) {
            instance = new AccountManager(QCoreApplication::instance());
            QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
        }
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

    // 不能给 parent 默认值，否则不走 create()（见 cpp/AppModels.h）
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
    QString neteaseVerifyUrl() const { return m_neteaseVerifyUrl; }

    bool isKugouLoggedIn() const { return m_kugouLoggedIn; }
    QString kugouNickname() const { return m_kugouNickname; }
    QString kugouAvatar() const { return m_kugouAvatar; }
    QString kugouCookie() const { return m_kugouCookie; }
    QString kugouMid() const { return m_kugouMid; }
    QString kugouDfid() const { return m_kugouDfid; }
    QString kugouQrText() const { return m_kugouQrText; }
    int kugouQrState() const { return m_kugouQrState; }
    QString kugouQrMessage() const { return m_kugouQrMessage; }

    bool isSpotifyLoggedIn() const { return m_spotifyLoggedIn; }
    QString spotifyNickname() const { return m_spotifyNickname; }
    QString spotifyAvatar() const { return m_spotifyAvatar; }
    QString spotifyCookie() const { return m_spotifyCookie; }
    QString spotifyAccessToken() const { return m_spotifyToken; }
    QString spotifyLoginMessage() const { return m_spotifyLoginMessage; }
    bool isAppleLoggedIn() const { return m_appleLoggedIn; }
    QString appleNickname() const { return m_appleNickname; }
    QString appleAvatar() const { return m_appleAvatar; }
    QString appleLoginMessage() const { return m_appleLoginMessage; }
    QString appleUserToken() const { return m_appleUserToken; }
    QString appleDeveloperToken() const { return m_appleDeveloperToken; }

    bool isBilibiliLoggedIn() const { return m_bilibiliLoggedIn; }
    QString bilibiliNickname() const { return m_bilibiliNickname; }
    QString bilibiliAvatar() const { return m_bilibiliAvatar; }
    QString bilibiliCookie() const { return m_bilibiliCookie; }
    QString bilibiliQrText() const { return m_bilibiliQrText; }
    int bilibiliQrState() const { return m_bilibiliQrState; }
    QString bilibiliQrMessage() const { return m_bilibiliQrMessage; }

    bool isQQLoggedIn() const { return m_qqLoggedIn; }
    QString qqNickname() const { return m_qqNickname; }
    QString qqAvatar() const { return m_qqAvatar; }
    QString qqCookie() const
    {
        if (m_qqEuIn.isEmpty() || m_qqCookie.contains(QLatin1String("euin=")))
            return m_qqCookie;
        return m_qqCookie + QStringLiteral("; euin=") + m_qqEuIn;
    }
    QString qqLoginMessage() const { return m_qqLoginMessage; }

    Q_INVOKABLE void loadPersisted(); // 启动时恢复登录态

public slots:
    void startNeteaseQrLogin();
    void cancelNeteaseQrLogin();
    void logoutNetease();
    Q_INVOKABLE void openNeteaseVerifyUrl();

    void startKugouQrLogin();
    void cancelKugouQrLogin();
    void logoutKugou();

    Q_INVOKABLE void startSpotifyBrowserLogin();
    Q_INVOKABLE void loginSpotifyWithCookie(const QString &input);
    Q_INVOKABLE void logoutSpotify();
    Q_INVOKABLE void startAppleBrowserLogin();
    Q_INVOKABLE void loginAppleWithCookie(const QString &input);
    Q_INVOKABLE void logoutApple();

    Q_INVOKABLE void startBilibiliQrLogin();
    Q_INVOKABLE void cancelBilibiliQrLogin();
    Q_INVOKABLE void loginBilibiliWithCookie(const QString &cookie);
    Q_INVOKABLE void logoutBilibili();

    Q_INVOKABLE void loginQQWithCookie(const QString &cookie);
    Q_INVOKABLE void requestQQSmsCode(const QString &phone);
    Q_INVOKABLE void loginQQWithCode(const QString &phone, const QString &code);
    Q_INVOKABLE void logoutQQ();

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
    void spotifyLoginChanged();
    void appleLoginChanged();
    void bilibiliLoginChanged();
    void bilibiliQrChanged();
    void qqLoginChanged();
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
    // 客户端身份：稳定 deviceId + os=pc；SDK 默认每进程随机 + iPhone 身份，易判异常
    QString ensureNeteaseDeviceId();
    // 设备匿名注册换 MUSIC_A（网易云的可信设备凭证）；阻塞调用，须在工作线程里用
    void ensureNeteaseAnonymousToken();
    QString mergeNeteaseIdentity(const QString &cookie) const;
    void verifyNeteaseLogin(); // 用账号信息接口校验当前 cookie 是否有效
    void onKugouKey(QNetworkReply *reply);
    void onKugouPoll(QNetworkReply *reply);

    void writeNeteaseCookiesToJar(const QString &cookie);
    void clearLoginSettings(const QString &group);
    void persistNetease();
    void persistKugou();
    void ensureKugouDevice(); // guid/dfid/mid：签名与取址都依赖，缺一不可
    void loadNetease();
    void loadKugou();
    void persistSpotify();
    void persistApple();
    void loadSpotify();
    void loadApple();
    void persistBilibili();
    void loadBilibili();
    void persistQQ();
    void loadQQ();
    void pollBilibili();
    void onBilibiliKey(QNetworkReply *reply);
    void onBilibiliPoll(QNetworkReply *reply);
    void setBilibiliQr(int state, const QString &msg, const QString &qrText = QString());
    void validateBilibiliCookie(const QString &cookie);
    void validateQQCookie(const QString &cookie);
    void postQQCgi(const QJsonObject &body, std::function<void(const QJsonObject &)> done);
    void finishQQLogin(const QJsonObject &data);
    void browserGet(const QUrl &url, const QByteArray &cookie,
                    const QList<QPair<QByteArray, QByteArray>> &headers,
                    const std::function<void(int, const QByteArray &)> &done);
    void validateSpotifyToken(const QString &token, const QString &cookie);
    void refreshSpotifyToken();
    void validateAppleToken(const QString &userToken, const QString &developerToken);
    void fetchAppleDeveloperToken(const std::function<void(const QString &)> &done);
    void setNeteaseQr(int state, const QString &msg, const QString &qrText = QString());
    void setKugouQr(int state, const QString &msg, const QString &qrText = QString());

    bool m_neteaseLoggedIn = false;
    bool m_kugouLoggedIn = false;
    bool m_spotifyLoggedIn = false;
    bool m_appleLoggedIn = false;
    QString m_neteaseNickname;
    QString m_neteaseAvatar;
    QString m_kugouNickname;
    QString m_kugouAvatar;
    QString m_neteaseCookie;
    QString m_kugouCookie;
    QString m_spotifyCookie;
    QString m_spotifyToken;
    QString m_spotifyNickname;
    QString m_spotifyAvatar;
    QString m_spotifyLoginMessage;
    QString m_appleUserToken;
    QString m_appleDeveloperToken;
    QString m_appleNickname;
    QString m_appleAvatar;
    QString m_appleLoginMessage;

    bool m_bilibiliLoggedIn = false;
    bool m_qqLoggedIn = false;
    QString m_bilibiliCookie;
    QString m_bilibiliNickname;
    QString m_bilibiliAvatar;
    QString m_bilibiliQrText;
    QString m_bilibiliQrMessage;
    QString m_bilibiliKey;
    int m_bilibiliQrState = QrWaiting;
    QTimer *m_bilibiliPollTimer = nullptr;
    QString m_qqCookie;
    QString m_qqNickname;
    QString m_qqEuIn;
    QString m_qqAvatar;
    QString m_qqUin;
    QString m_qqLoginMessage;

    // 网易云客户端身份与扫码登录状态控制
    QString m_neteaseDeviceId;        // 持久化的设备号（Account.ini: Netease/deviceId）
    QString m_neteaseAnonymousToken;  // MUSIC_A（Netease/anonymousToken）
    QAtomicInt m_neteaseAnonBusy{0};  // 注册去重
    int m_neteasePollFails = 0;       // 连续轮询失败次数（风控时及时停止，避免刷分）
    qint64 m_neteaseQrDeadlineMs = 0; // 二维码整体有效期
    bool m_neteaseCookieVerifying = false; // 正在校验手工填入的 Cookie

    QString m_neteaseQrText;
    int m_neteaseQrState = QrWaiting;
    QString m_neteaseQrMessage;
    QString m_neteaseVerifyUrl;
    QString m_kugouQrText;
    int m_kugouQrState = QrWaiting;
    QString m_kugouQrMessage;

    QNetworkAccessManager *m_nam = nullptr;
    QNetworkCookieJar *m_jar = nullptr;
    QTimer *m_neteasePollTimer = nullptr;
    QTimer *m_kugouPollTimer = nullptr;
    QTimer *m_spotifyTokenTimer = nullptr;
    QString m_neteaseUnikey;
    QString m_kugouKey;

    // 网易云登录走 QCloudMusicApi（login_qr_* 接口），由其内部维护 cookie
    ApiHelper *m_api = nullptr;
    QThreadPool m_pool;               // 登录任务专用池：析构等它即可，不必等全局池
    QAtomicInt m_neteaseBusy{0};      // 防止并发调用阻塞的 invoke
    QAtomicInt m_neteaseCancelled{0}; // 取消/退出登录时置位

    // 酷狗设备信息
    QString m_kugouGuid;
    QString m_kugouMid;
    QString m_kugouDfid;

    QString m_configPath;
};

#endif // ACCOUNTMANAGER_H
