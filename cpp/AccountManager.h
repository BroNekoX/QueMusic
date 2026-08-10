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

class AccountManager : public QObject
{
    Q_OBJECT
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
    // 二维码状态码（QML 显示用）
    enum QrState {
        QrWaiting = 0,   // 等待扫码
        QrScanned = 1,   // 已扫码，待确认
        QrSuccess = 2,   // 登录成功
        QrExpired = 3,   // 二维码过期
        QrError = 4      // 网络/其他错误
    };
    Q_ENUM(QrState)

    explicit AccountManager(QObject *parent = nullptr);
    ~AccountManager() override;

    bool isNeteaseLoggedIn() const { return m_neteaseLoggedIn; }
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

signals:
    void neteaseLoginChanged();
    void kugouLoginChanged();
    void neteaseQrChanged();
    void kugouQrChanged();
    // type: 0 普通 / 1 成功 / 2 警告 / 3 错误（对接主窗口气泡提示）
    void message(const QString &text, int type);

private:
    void weapiPost(const QString &path, const QJsonObject &json);
    void kugouGet(const QString &baseUrl, const QString &path, const QJsonObject &customParams);
    void pollNetease();
    void pollKugou();

    void onNeteaseUnikey(QNetworkReply *reply);
    void onNeteasePoll(QNetworkReply *reply);
    void onNeteaseProfile(QNetworkReply *reply);
    void onKugouKey(QNetworkReply *reply);
    void onKugouPoll(QNetworkReply *reply);

    QString buildNeteaseCookieString() const;
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

    // 酷狗设备信息
    QString m_kugouGuid;
    QString m_kugouMid;
    QString m_kugouDfid;

    QString m_configPath;
};

#endif // ACCOUNTMANAGER_H
