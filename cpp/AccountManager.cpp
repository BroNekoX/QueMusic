// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "AccountManager.h"
#include "KugouApi.h"
#include "apihelper.h"

#include <QCoreApplication>
#include <QtConcurrent>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkCookie>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QSettings>
#include <QUrlQuery>

namespace {
const char *kUa = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                  "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
} // namespace

AccountManager::AccountManager(QObject *parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    m_jar = m_nam->cookieJar(); // QNetworkAccessManager 自带 jar

    m_neteasePollTimer = new QTimer(this);
    // 4 秒一次：原 2 秒过于频繁，轮询过密会累积风控评分
    m_neteasePollTimer->setInterval(4000);
    connect(m_neteasePollTimer, &QTimer::timeout, this, &AccountManager::pollNetease);

    // 网易云登录统一走 QCloudMusicApi（login_qr_* 接口），由其内部维护 cookie
    m_api = new ApiHelper();

    m_kugouPollTimer = new QTimer(this);
    m_kugouPollTimer->setInterval(3000);
    connect(m_kugouPollTimer, &QTimer::timeout, this, &AccountManager::pollKugou);

    m_configPath = QCoreApplication::applicationDirPath() + QStringLiteral("/Account.ini");

    loadPersisted();

    // 把「客户端身份 + 已登录 cookie」注入 SDK 的全局 cookie：
    // 之后所有网易云请求（扫码/轮询/其它接口）都会带上稳定 deviceId 与 os=pc。
    // 未登录时也要带身份（mergeNeteaseIdentity 对空 cookie 同样会补上这两个字段）
    m_api->set_cookie(mergeNeteaseIdentity(m_neteaseCookie));
}

AccountManager::~AccountManager()
{
    m_neteaseCancelled = 1;
    if (m_neteasePollTimer)
        m_neteasePollTimer->stop();
    if (m_kugouPollTimer)
        m_kugouPollTimer->stop();
    // m_api 由进程退出时自然释放，这里不主动 delete 以免与后台线程的阻塞 invoke 竞争
}

// 通用：把 cookie 字符串写回 QNAM 的 jar，供后续搜索/播放等请求带上登录态
void AccountManager::storeNeteaseCookieString(const QString &cookieStr)
{
    m_neteaseCookie = mergeNeteaseIdentity(cookieStr);
    writeNeteaseCookiesToJar(m_neteaseCookie);
    m_api->set_cookie(m_neteaseCookie);
}

// "a=1; b=2" 形式的 Cookie 写进 QNAM 的 jar，供后续请求自动携带
void AccountManager::writeNeteaseCookiesToJar(const QString &cookie)
{
    if (cookie.isEmpty())
        return;
    QList<QNetworkCookie> cookies;
    const QStringList parts = cookie.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        const QString t = p.trimmed();
        const int eq = t.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        QNetworkCookie c(t.left(eq).trimmed().toUtf8(), t.mid(eq + 1).trimmed().toUtf8());
        c.setDomain(QStringLiteral(".music.163.com"));
        c.setPath(QStringLiteral("/"));
        cookies << c;
    }
    m_jar->setCookiesFromUrl(cookies, QUrl(QStringLiteral("https://music.163.com")));
}

// 稳定的设备号：网易云按 deviceId 识别"同一台设备"。
// QCloudMusicApi 默认每次启动随机生成 deviceId（request.cpp 的 kStaticDeviceId），
// 同一台机器在服务端看来反复换设备，是扫码被判"环境异常"的主要嫌疑之一 ➜ 这里持久化。
QString AccountManager::ensureNeteaseDeviceId()
{
    if (!m_neteaseDeviceId.isEmpty())
        return m_neteaseDeviceId;

    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Netease"));
    m_neteaseDeviceId = s.value(QStringLiteral("deviceId")).toString().trimmed();
    s.endGroup();

    if (m_neteaseDeviceId.isEmpty()) {
        static const QString hex = QStringLiteral("0123456789ABCDEF");
        for (int i = 0; i < 52; ++i) // 与 SDK 生成格式一致（52 位大写 hex）
            m_neteaseDeviceId.append(hex.at(QRandomGenerator::global()->bounded(hex.size())));
        QSettings w(m_configPath, QSettings::IniFormat);
        w.beginGroup(QStringLiteral("Netease"));
        w.setValue(QStringLiteral("deviceId"), m_neteaseDeviceId);
        w.endGroup();
        w.sync();
    }
    return m_neteaseDeviceId;
}

// 给 cookie 补上客户端身份：os=pc（与 weapi 的 PC Chrome UA、网页扫码 type=1 自洽）
// + 持久 deviceId（替换掉 SDK 的每进程随机值，也替换外部导入的其它 os）
QString AccountManager::mergeNeteaseIdentity(const QString &cookie) const
{
    QStringList out;
    const QStringList parts = cookie.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &raw : parts) {
        const QString p = raw.trimmed();
        if (p.isEmpty())
            continue;
        const int eq = p.indexOf(QLatin1Char('='));
        const QString key = eq > 0 ? p.left(eq).trimmed() : p;
        if (key.compare(QStringLiteral("deviceId"), Qt::CaseInsensitive) == 0
            || key.compare(QStringLiteral("os"), Qt::CaseInsensitive) == 0) {
            continue; // 丢弃旧值，统一由下面两个字段决定
        }
        out << p;
    }
    out << QStringLiteral("deviceId=") + m_neteaseDeviceId;
    out << QStringLiteral("os=pc");
    return out.join(QStringLiteral("; "));
}

// 账号信息接口：既用于扫码登录成功后刷新昵称/头像，也用于校验手工填入的 Cookie
void AccountManager::verifyNeteaseLogin()
{
    QNetworkRequest req(QUrl(QStringLiteral("https://music.163.com/api/nuser/account/get")));
    req.setRawHeader("User-Agent", kUa);
    req.setRawHeader("Referer", "https://music.163.com/");
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply] { onNeteaseProfile(reply); });
}

// 备用登录①：粘贴含 MUSIC_U 的 Cookie（扫码被风控拦截时的兜底，登录态交由账号信息接口校验）
void AccountManager::loginNeteaseWithCookie(const QString &cookie)
{
    const QString c = cookie.trimmed();
    if (c.isEmpty()) {
        setNeteaseQr(QrError, QStringLiteral("请先粘贴 Cookie"));
        return;
    }
    if (!c.contains(QStringLiteral("MUSIC_U"))) {
        setNeteaseQr(QrError, QStringLiteral("Cookie 里没有 MUSIC_U，请复制完整 Cookie"));
        return;
    }
    m_neteasePollTimer->stop();
    storeNeteaseCookieString(c);
    m_neteaseLoggedIn = true;
    m_neteaseNickname = QStringLiteral("网易云音乐用户");
    m_neteaseCookieVerifying = true; // 校验失败会自动撤销登录态
    persistNetease();
    setNeteaseQr(QrWaiting, QStringLiteral("正在校验 Cookie…"));
    emit neteaseLoginChanged();
    verifyNeteaseLogin();
}

// 备用登录②：发送短信验证码
void AccountManager::sendNeteaseCaptcha(const QString &phone)
{
    const QString p = phone.trimmed();
    if (p.isEmpty()) {
        setNeteaseQr(QrError, QStringLiteral("请先填写手机号"));
        return;
    }
    m_neteasePollTimer->stop();
    setNeteaseQr(QrWaiting, QStringLiteral("正在发送验证码…"));
    (void)QtConcurrent::run([this, p]() {
        const QVariantMap res = m_api->invoke(
            QStringLiteral("captcha_sent"),
            QVariantMap{{QStringLiteral("cellphone"), p}, {QStringLiteral("ctcode"), QStringLiteral("86")}});
        const QVariantMap body = res.value(QStringLiteral("body")).toMap();
        const bool ok = res.value(QStringLiteral("status")).toInt() == 200
                        && (body.value(QStringLiteral("code")).toInt() == 200
                            || body.value(QStringLiteral("data")).toBool());
        const QString msg = body.value(QStringLiteral("message")).toString();
        QMetaObject::invokeMethod(
            this,
            [this, ok, msg] {
                setNeteaseQr(ok ? QrWaiting : QrError,
                             ok ? QStringLiteral("验证码已发送，请查看短信")
                                : QStringLiteral("验证码发送失败：%1")
                                      .arg(msg.isEmpty() ? QStringLiteral("请稍后重试") : msg));
            },
            Qt::QueuedConnection);
    });
}

// 备用登录③：手机号 + 验证码登录
void AccountManager::loginNeteaseWithCellphone(const QString &phone, const QString &captcha)
{
    const QString p = phone.trimmed();
    const QString code = captcha.trimmed();
    if (p.isEmpty() || code.isEmpty()) {
        setNeteaseQr(QrError, QStringLiteral("请填写手机号与验证码"));
        return;
    }
    m_neteasePollTimer->stop();
    setNeteaseQr(QrWaiting, QStringLiteral("正在登录…"));
    (void)QtConcurrent::run([this, p, code]() {
        const QVariantMap res = m_api->invoke(
            QStringLiteral("login_cellphone"),
            QVariantMap{{QStringLiteral("phone"), p},
                        {QStringLiteral("countrycode"), QStringLiteral("86")},
                        {QStringLiteral("captcha"), code}});
        const QVariantMap body = res.value(QStringLiteral("body")).toMap();
        const int bcode = body.value(QStringLiteral("code")).toInt();
        const QString cookie = res.value(QStringLiteral("cookie")).toString();
        const QString msg = body.value(QStringLiteral("message")).toString();
        QMetaObject::invokeMethod(
            this,
            [this, bcode, cookie, msg] {
                if (bcode != 200 || cookie.isEmpty()) {
                    setNeteaseQr(QrError,
                                 QStringLiteral("登录失败：%1")
                                     .arg(msg.isEmpty() ? QStringLiteral("验证码错误或已过期") : msg));
                    return;
                }
                storeNeteaseCookieString(cookie);
                m_neteaseLoggedIn = true;
                m_neteaseNickname = QStringLiteral("网易云音乐用户");
                persistNetease();
                setNeteaseQr(QrSuccess, QStringLiteral("登录成功"));
                emit neteaseLoginChanged();
                emit message(QStringLiteral("网易云账号登录成功"), 1);
                verifyNeteaseLogin();
            },
            Qt::QueuedConnection);
    });
}

// 通用：酷狗 web 签名 GET

void AccountManager::kugouGet(const QString &baseUrl, const QString &path,
                              const QJsonObject &customParams)
{
    QJsonObject params;
    params.insert(QStringLiteral("dfid"), m_kugouDfid);
    params.insert(QStringLiteral("mid"), m_kugouMid);
    params.insert(QStringLiteral("uuid"), QStringLiteral("-"));
    params.insert(QStringLiteral("appid"), 1001);
    params.insert(QStringLiteral("clientver"), 20489);
    params.insert(QStringLiteral("clienttime"), QString::number(QDateTime::currentSecsSinceEpoch()));

    // 自定义参数覆盖默认值
    const QStringList customKeys = customParams.keys();
    for (const QString &k : customKeys)
        params.insert(k, customParams.value(k).toVariant().toString());

    QByteArray sig = KugouApi::kugouWebSignature(params);
    params.insert(QStringLiteral("signature"), QString::fromLatin1(sig));

    QUrlQuery query;
    const QStringList keys = params.keys();
    for (const QString &k : keys)
        query.addQueryItem(k, params.value(k).toVariant().toString());

    QUrl url(baseUrl + path);
    url.setQuery(query);

    qDebug() << "[kugou] GET" << url.toString().left(300);

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", kUa);
    req.setRawHeader("Accept-Encoding", "identity"); // 避免 gzip，方便直接解析 JSON
    req.setRawHeader("dfid", m_kugouDfid.toUtf8());
    req.setRawHeader("clienttime", params.value(QStringLiteral("clienttime")).toVariant().toString().toUtf8());
    req.setRawHeader("mid", m_kugouMid.toUtf8());
    req.setRawHeader("kg-rc", "1");
    req.setRawHeader("kg-thash", "5d816a0");
    req.setRawHeader("kg-rec", "1");
    req.setRawHeader("kg-rf", "B9EDA08A64250DEFFBCADDEE00F8F25F");
    QNetworkReply *reply = m_nam->get(req);
    if (path.contains(QStringLiteral("qrcode")) && !path.contains(QStringLiteral("userinfo"))) {
        connect(reply, &QNetworkReply::finished, this,
                [this, reply] { onKugouKey(reply); });
    } else {
        connect(reply, &QNetworkReply::finished, this,
                [this, reply] { onKugouPoll(reply); });
    }
}

// 网易云

void AccountManager::startNeteaseQrLogin()
{
    m_neteasePollTimer->stop();
    m_neteaseCancelled = 0;
    m_neteaseUnikey.clear();
    setNeteaseQr(QrWaiting, QStringLiteral("正在获取二维码…"));

    // 登录走 QCloudMusicApi 的 login_qr_* 接口（同步阻塞），放到线程池执行避免卡 UI
    (void)QtConcurrent::run([this]() { neteaseFetchQrWorker(); });
}

void AccountManager::neteaseFetchQrWorker()
{
    if (m_neteaseCancelled.loadAcquire())
        return;
    if (!m_neteaseBusy.testAndSetAcquire(0, 1))
        return;

    // 1) 获取 unikey
    QVariantMap keyRes = m_api->invoke(QStringLiteral("login_qr_key"), QVariantMap());
    QString unikey;
    if (keyRes[QStringLiteral("status")].toInt() == 200 &&
        keyRes[QStringLiteral("body")].toMap()[QStringLiteral("code")].toInt() == 200) {
        unikey = keyRes[QStringLiteral("body")].toMap()
                     [QStringLiteral("unikey")].toString();
    }
    if (unikey.isEmpty()) {
        m_neteaseBusy.storeRelease(0);
        QMetaObject::invokeMethod(this, "onNeteaseFetchError", Qt::QueuedConnection,
                                  Q_ARG(QString, QStringLiteral("获取二维码失败（可能被网易云风控拦截），"
                                                                "可改用手机号或 Cookie 登录")));
        return;
    }

    if (m_neteaseCancelled.loadAcquire()) {
        m_neteaseBusy.storeRelease(0);
        return;
    }

    // 2) 用 unikey 生成二维码链接
    QVariantMap createRes = m_api->invoke(
        QStringLiteral("login_qr_create"),
        QVariantMap{{QStringLiteral("key"), unikey}});
    QString qrurl;
    if (createRes[QStringLiteral("status")].toInt() == 200 &&
        createRes[QStringLiteral("body")].toMap()[QStringLiteral("code")].toInt() == 200) {
        qrurl = createRes[QStringLiteral("body")].toMap()
                    [QStringLiteral("qrurl")].toString();
    }
    m_neteaseBusy.storeRelease(0);

    if (qrurl.isEmpty()) {
        QMetaObject::invokeMethod(this, "onNeteaseFetchError", Qt::QueuedConnection,
                                  Q_ARG(QString, QStringLiteral("生成二维码失败，请重试")));
        return;
    }

    // 3) 把结果送回主线程：记录 unikey、显示二维码、启动轮询
    QMetaObject::invokeMethod(this, "onNeteaseQrFetched", Qt::QueuedConnection,
                              Q_ARG(QString, unikey), Q_ARG(QString, qrurl));
}

void AccountManager::cancelNeteaseQrLogin()
{
    m_neteaseCancelled = 1;
    m_neteasePollTimer->stop();
}

void AccountManager::logoutNetease()
{
    m_neteasePollTimer->stop();
    m_neteaseLoggedIn = false;
    m_neteaseNickname.clear();
    m_neteaseAvatar.clear();
    m_neteaseCookie.clear();
    m_jar->deleteCookie(QNetworkCookie(QStringLiteral("MUSIC_U").toUtf8(), QByteArray()));
    m_jar->setCookiesFromUrl({}, QUrl(QStringLiteral("https://music.163.com")));

    clearLoginSettings(QStringLiteral("Netease"));

    setNeteaseQr(QrWaiting, QStringLiteral("已退出登录"));
    emit neteaseLoginChanged();
    emit message(QStringLiteral("已退出网易云账号"), 0);
}

void AccountManager::pollNetease()
{
    if (m_neteaseCancelled.loadAcquire() || m_neteaseUnikey.isEmpty()) {
        m_neteasePollTimer->stop();
        return;
    }
    if (m_neteaseQrDeadlineMs > 0 && QDateTime::currentMSecsSinceEpoch() > m_neteaseQrDeadlineMs) {
        m_neteasePollTimer->stop();
        setNeteaseQr(QrExpired, QStringLiteral("二维码已过期，请点击重新获取"));
        return;
    }
    // 登录走 QCloudMusicApi，invoke 为阻塞调用，放到线程池执行避免卡 UI
    const QString key = m_neteaseUnikey;
    (void)QtConcurrent::run([this, key]() { neteasePollWorker(key); });
}

void AccountManager::onNeteaseQrFetched(const QString &unikey, const QString &qrurl)
{
    if (m_neteaseCancelled.loadAcquire())
        return;
    m_neteaseUnikey = unikey;
    m_neteasePollFails = 0;
    m_neteaseQrDeadlineMs = QDateTime::currentMSecsSinceEpoch() + 3 * 60 * 1000; // 3 分钟有效
    setNeteaseQr(QrWaiting, QStringLiteral("请使用网易云音乐App扫码登录"), qrurl);
    m_neteasePollTimer->start();
}

void AccountManager::onNeteaseFetchError(const QString &msg)
{
    if (m_neteaseCancelled.loadAcquire())
        return;
    setNeteaseQr(QrError, msg);
}

void AccountManager::onNeteasePollResult(int code, const QString &cookie,
                                        const QString &nickname, const QString &msg)
{
    if (m_neteaseCancelled.loadAcquire())
        return;
    switch (code) {
    case 800: // 过期
        m_neteasePollTimer->stop();
        setNeteaseQr(QrExpired, QStringLiteral("二维码已过期，请点击重新获取"));
        break;
    case 801: // 等待
        m_neteasePollFails = 0;
        setNeteaseQr(QrWaiting, QStringLiteral("请使用网易云音乐App扫码登录"));
        break;
    case 802: // 已扫码
        m_neteasePollFails = 0;
        setNeteaseQr(QrScanned, nickname.isEmpty() ? QStringLiteral("已扫码，请在手机上确认")
                                                    : QStringLiteral("%1 正在确认登录").arg(nickname));
        break;
    case 803: { // 成功
        m_neteasePollTimer->stop();
        storeNeteaseCookieString(cookie); // 内部已统一身份（os=pc + 稳定 deviceId）
        m_neteaseLoggedIn = true;
        persistNetease();
        setNeteaseQr(QrSuccess, QStringLiteral("登录成功"));
        emit neteaseLoginChanged();
        emit message(QStringLiteral("网易云账号登录成功"), 1);
        verifyNeteaseLogin(); // 异步刷新昵称/头像
        break;
    }
    default:
        // 801/802 之外的未知状态多是风控拦截：连续几次就停手，避免继续刷分
        if (++m_neteasePollFails >= 3) {
            m_neteasePollTimer->stop();
            setNeteaseQr(QrError, QStringLiteral("网易云拒绝了本次登录（扫码环境被风控拦截），"
                                                 "可改用手机号或 Cookie 登录"));
        } else {
            setNeteaseQr(QrWaiting, msg.isEmpty() ? QStringLiteral("登录状态异常，正在重试…") : msg);
        }
        break;
    }
}

void AccountManager::neteasePollWorker(const QString &key)
{
    if (m_neteaseCancelled.loadAcquire())
        return;
    if (!m_neteaseBusy.testAndSetAcquire(0, 1))
        return; // 上一次轮询未结束，跳过本次

    QVariantMap res = m_api->invoke(QStringLiteral("login_qr_check"),
                                    QVariantMap{{QStringLiteral("key"), key}});
    m_neteaseBusy.storeRelease(0);

    const QVariantMap body = res[QStringLiteral("body")].toMap();
    const int code = body[QStringLiteral("code")].toInt();
    const QString cookie = res[QStringLiteral("cookie")].toString();
    const QString nickname = body[QStringLiteral("nickname")].toString();
    const QString msg = body[QStringLiteral("message")].toString();

    // 结果送回主线程处理
    QMetaObject::invokeMethod(this, "onNeteasePollResult", Qt::QueuedConnection,
                              Q_ARG(int, code), Q_ARG(QString, cookie),
                              Q_ARG(QString, nickname), Q_ARG(QString, msg));
}

void AccountManager::onNeteaseProfile(QNetworkReply *reply)
{
    reply->deleteLater();
    const QJsonObject obj = reply->error() == QNetworkReply::NoError
                                ? QJsonDocument::fromJson(reply->readAll()).object()
                                : QJsonObject();
    const QJsonObject profile = obj.value(QStringLiteral("profile")).toObject();
    const bool ok = obj.value(QStringLiteral("code")).toInt() == 200 && !profile.isEmpty();
    if (!ok) {
        // 手工粘贴 Cookie 的校验失败：撤销登录态并明确提示；
        // 扫码成功后的刷新失败则静默忽略（不影响已建立的登录态）
        if (!m_neteaseCookieVerifying)
            return;
        m_neteaseCookieVerifying = false;
        m_neteaseLoggedIn = false;
        m_neteaseCookie.clear();
        m_neteaseNickname.clear();
        m_neteaseAvatar.clear();
        m_jar->setCookiesFromUrl({}, QUrl(QStringLiteral("https://music.163.com")));
        persistNetease();
        setNeteaseQr(QrError, QStringLiteral("Cookie 无效或已过期，请重新获取"));
        emit message(QStringLiteral("网易云 Cookie 校验失败"), 3);
        emit neteaseLoginChanged();
        return;
    }
    m_neteaseCookieVerifying = false;
    m_neteaseNickname = profile.value(QStringLiteral("nickname")).toString();
    m_neteaseAvatar = profile.value(QStringLiteral("avatarUrl")).toString();
    if (m_neteaseNickname.isEmpty())
        m_neteaseNickname = QStringLiteral("网易云音乐用户");
    persistNetease();
    emit neteaseLoginChanged();
}

// 酷狗

void AccountManager::startKugouQrLogin()
{
    m_kugouPollTimer->stop();
    m_kugouKey.clear();

    // 设备信息（无则生成并持久化）
    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Kugou"));
    m_kugouGuid = s.value(QStringLiteral("guid")).toString();
    m_kugouDfid = s.value(QStringLiteral("dfid")).toString();
    s.endGroup();
    if (m_kugouGuid.isEmpty()) {
        m_kugouGuid = KugouApi::randomGuid();
        QSettings w(m_configPath, QSettings::IniFormat);
        w.beginGroup(QStringLiteral("Kugou"));
        w.setValue(QStringLiteral("guid"), m_kugouGuid);
        w.endGroup();
        w.sync();
    }
    if (m_kugouDfid.isEmpty()) {
        // 酷狗 dfid 通常为 24 位大写字母+数字（与官方客户端一致）
        static const char *pool = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        QString dfid;
        for (int i = 0; i < 24; ++i)
            dfid.append(pool[QRandomGenerator::global()->bounded(36)]);
        m_kugouDfid = dfid;
        QSettings w(m_configPath, QSettings::IniFormat);
        w.beginGroup(QStringLiteral("Kugou"));
        w.setValue(QStringLiteral("dfid"), m_kugouDfid);
        w.endGroup();
        w.sync();
    }
    m_kugouMid = KugouApi::kugouMidFromGuid(m_kugouGuid);

    setKugouQr(QrWaiting, QStringLiteral("正在获取二维码…"));

    QJsonObject custom;
    custom.insert(QStringLiteral("appid"), 1001);
    custom.insert(QStringLiteral("type"), 1);
    custom.insert(QStringLiteral("plat"), 4);
    custom.insert(QStringLiteral("qrcode_txt"),
                 QStringLiteral("https://h5.kugou.com/apps/loginQRCode/html/index.html?appid=1001&"));
    custom.insert(QStringLiteral("srcappid"), 2919);
    kugouGet(QStringLiteral("https://login-user.kugou.com"),
             QStringLiteral("/v2/qrcode"), custom);
}

void AccountManager::cancelKugouQrLogin()
{
    m_kugouPollTimer->stop();
}

void AccountManager::logoutKugou()
{
    m_kugouPollTimer->stop();
    m_kugouLoggedIn = false;
    m_kugouNickname.clear();
    m_kugouAvatar.clear();
    m_kugouCookie.clear();

    clearLoginSettings(QStringLiteral("Kugou"));

    setKugouQr(QrWaiting, QStringLiteral("已退出登录"));
    emit kugouLoginChanged();
    emit message(QStringLiteral("已退出酷狗账号"), 0);
}

void AccountManager::pollKugou()
{
    if (m_kugouKey.isEmpty())
        return;
    QJsonObject custom;
    custom.insert(QStringLiteral("plat"), 4);
    custom.insert(QStringLiteral("appid"), 1001);
    custom.insert(QStringLiteral("srcappid"), 2919);
    custom.insert(QStringLiteral("qrcode"), m_kugouKey);
    kugouGet(QStringLiteral("https://login-user.kugou.com"),
             QStringLiteral("/v2/get_userinfo_qrcode"), custom);
}

void AccountManager::onKugouKey(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[kugou] qrcode 网络错误:" << reply->errorString();
        setKugouQr(QrError, QStringLiteral("网络错误：%1").arg(reply->errorString()));
        return;
    }
    QByteArray raw = reply->readAll();
    QJsonObject obj = QJsonDocument::fromJson(raw).object();
    if (obj.value(QStringLiteral("status")).toInt() != 1) {
        qWarning() << "[kugou] qrcode 业务失败 status:" << obj.value("status").toInt()
                   << "errcode:" << obj.value("error_code").toInt()
                   << "errmsg:" << obj.value("error_msg").toString()
                   << obj.value("msg").toString();
        setKugouQr(QrError, QStringLiteral("获取二维码失败"));
        return;
    }
    m_kugouKey = obj.value(QStringLiteral("data")).toObject()
                     .value(QStringLiteral("qrcode")).toString();
    if (m_kugouKey.isEmpty()) {
        qWarning() << "[kugou] qrcode key 为空, data:" << QString::fromUtf8(raw).left(300);
        setKugouQr(QrError, QStringLiteral("二维码参数为空"));
        return;
    }
    // 扫码落地页：与接口 qrcode_txt 的 appid 保持一致
    QString qrText = QStringLiteral("https://h5.kugou.com/apps/loginQRCode/html/index.html?appid=1001&qrcode=")
                     + m_kugouKey;
    setKugouQr(QrWaiting, QStringLiteral("请使用酷狗音乐App扫码登录"), qrText);
    m_kugouPollTimer->start();
}

void AccountManager::onKugouPoll(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
        return;
    QByteArray raw = reply->readAll();
    QJsonObject obj = QJsonDocument::fromJson(raw).object();
    if (obj.value(QStringLiteral("status")).toInt() != 1)
        return;
    QJsonObject data = obj.value(QStringLiteral("data")).toObject();
    int state = data.value(QStringLiteral("status")).toInt(-1);
    switch (state) {
    case 0: // 过期
        m_kugouPollTimer->stop();
        setKugouQr(QrExpired, QStringLiteral("二维码已过期，请点击重新获取"));
        break;
    case 1: // 等待
        setKugouQr(QrWaiting, QStringLiteral("请使用酷狗音乐App扫码登录"));
        break;
    case 2: // 已扫码
        setKugouQr(QrScanned, QStringLiteral("已扫码，请在手机上确认登录"));
        break;
    case 4: { // 成功
        m_kugouPollTimer->stop();
        // 注意：酷狗的 user_id 是 JSON 数字类型，QJsonValue::toString() 对数字
        // 会返回空字符串，必须用 toVariant().toString() 才能拿到！
        QString token = data.value(QStringLiteral("token")).toVariant().toString();
        // 酷狗接口返回 user_id（下划线），老版本某些端才叫 userid，两个都兼容
        QString userid = data.value(QStringLiteral("user_id")).toVariant().toString();
        if (userid.isEmpty())
            userid = data.value(QStringLiteral("userid")).toVariant().toString();
        if (token.isEmpty()) {
            setKugouQr(QrError, QStringLiteral("登录失败：未获取到凭证"));
            return;
        }
        m_kugouCookie = QStringLiteral("token=%1; userid=%2").arg(token, userid);
        m_kugouNickname = data.value(QStringLiteral("nickname")).toString();
        if (m_kugouNickname.isEmpty())
            m_kugouNickname = QStringLiteral("酷狗用户(%1)").arg(userid);
        m_kugouAvatar = data.value(QStringLiteral("user_img")).toString();
        persistKugou();
        m_kugouLoggedIn = true;
        setKugouQr(QrSuccess, QStringLiteral("登录成功"));
        emit kugouLoginChanged();
        emit message(QStringLiteral("酷狗账号登录成功"), 1);
        break;
    }
    default:
        setKugouQr(QrError, QStringLiteral("未知扫码状态"));
        break;
    }
}

// 持久化 / 工具

// 退出登录时清掉该平台的持久化登录态（设备标识等保留）
void AccountManager::clearLoginSettings(const QString &group)
{
    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(group);
    s.remove(QStringLiteral("cookie"));
    s.remove(QStringLiteral("nickname"));
    s.remove(QStringLiteral("avatar"));
    s.endGroup();
    s.sync();
}

void AccountManager::persistNetease()
{
    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Netease"));
    s.setValue(QStringLiteral("cookie"), m_neteaseCookie);
    s.setValue(QStringLiteral("nickname"), m_neteaseNickname);
    s.setValue(QStringLiteral("avatar"), m_neteaseAvatar);
    s.endGroup();
    s.sync();
}

void AccountManager::persistKugou()
{
    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Kugou"));
    s.setValue(QStringLiteral("cookie"), m_kugouCookie);
    s.setValue(QStringLiteral("nickname"), m_kugouNickname);
    s.setValue(QStringLiteral("avatar"), m_kugouAvatar);
    s.setValue(QStringLiteral("guid"), m_kugouGuid);
    s.setValue(QStringLiteral("dfid"), m_kugouDfid);
    s.endGroup();
    s.sync();
}

void AccountManager::loadNetease()
{
    ensureNeteaseDeviceId(); // 身份先就绪，下面给 cookie 补 deviceId/os=pc

    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Netease"));
    m_neteaseCookie = s.value(QStringLiteral("cookie")).toString();
    m_neteaseNickname = s.value(QStringLiteral("nickname")).toString();
    m_neteaseAvatar = s.value(QStringLiteral("avatar")).toString();
    s.endGroup();

    if (m_neteaseCookie.isEmpty())
        return;

    const QString merged = mergeNeteaseIdentity(m_neteaseCookie);
    if (merged != m_neteaseCookie) {
        m_neteaseCookie = merged;
        persistNetease();
    }

    writeNeteaseCookiesToJar(m_neteaseCookie);
    m_neteaseLoggedIn = true;
    verifyNeteaseLogin();
}

void AccountManager::loadKugou()
{
    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Kugou"));
    m_kugouCookie = s.value(QStringLiteral("cookie")).toString();
    m_kugouNickname = s.value(QStringLiteral("nickname")).toString();
    m_kugouAvatar = s.value(QStringLiteral("avatar")).toString();
    m_kugouGuid = s.value(QStringLiteral("guid")).toString();
    m_kugouDfid = s.value(QStringLiteral("dfid")).toString();
    s.endGroup();

    if (m_kugouCookie.isEmpty())
        return;
    if (!m_kugouGuid.isEmpty())
        m_kugouMid = KugouApi::kugouMidFromGuid(m_kugouGuid);
    if (m_kugouDfid.isEmpty())
        m_kugouDfid = QStringLiteral("-");
    m_kugouLoggedIn = true;
}

void AccountManager::loadPersisted()
{
    loadNetease();
    loadKugou();
}

// 浏览器登录 Cookie 回填（内嵌 WebView 登录完成后调用）

void AccountManager::setNeteaseBrowserCookie(const QString &cookie, const QString &nickname,
                                             const QString &avatar)
{
    if (cookie.trimmed().isEmpty())
        return;
    m_neteaseCookie = mergeNeteaseIdentity(cookie.trimmed());
    m_api->set_cookie(m_neteaseCookie);
    m_neteaseNickname = nickname.trimmed().isEmpty() ? QStringLiteral("网易云音乐用户")
                                                     : nickname.trimmed();
    m_neteaseAvatar = avatar.trimmed();
    m_neteaseLoggedIn = true;

    writeNeteaseCookiesToJar(m_neteaseCookie);
    persistNetease();
    emit neteaseLoginChanged();
    emit message(QStringLiteral("网易云账号登录成功"), 1);
}

void AccountManager::setKugouBrowserCookie(const QString &cookie, const QString &nickname,
                                           const QString &avatar)
{
    if (cookie.trimmed().isEmpty())
        return;
    m_kugouCookie = cookie.trimmed();
    m_kugouNickname = nickname.trimmed().isEmpty() ? QStringLiteral("酷狗音乐用户")
                                                   : nickname.trimmed();
    m_kugouAvatar = avatar.trimmed();
    m_kugouLoggedIn = true;

    persistKugou();
    emit kugouLoginChanged();
    emit message(QStringLiteral("酷狗账号登录成功"), 1);
}

void AccountManager::setNeteaseQr(int state, const QString &msg, const QString &qrText)
{
    if (!qrText.isNull())
        m_neteaseQrText = qrText;
    m_neteaseQrState = state;
    m_neteaseQrMessage = msg;
    emit neteaseQrChanged();
}

void AccountManager::setKugouQr(int state, const QString &msg, const QString &qrText)
{
    if (!qrText.isNull())
        m_kugouQrText = qrText;
    m_kugouQrState = state;
    m_kugouQrMessage = msg;
    emit kugouQrChanged();
}

