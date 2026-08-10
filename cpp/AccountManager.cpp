// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "AccountManager.h"
#include "WeCrypto.h"

#include <QCoreApplication>
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
    m_neteasePollTimer->setInterval(2000);
    connect(m_neteasePollTimer, &QTimer::timeout, this, &AccountManager::pollNetease);

    m_kugouPollTimer = new QTimer(this);
    m_kugouPollTimer->setInterval(3000);
    connect(m_kugouPollTimer, &QTimer::timeout, this, &AccountManager::pollKugou);

    m_configPath = QCoreApplication::applicationDirPath() + QStringLiteral("/Account.ini");

    loadPersisted();
}

AccountManager::~AccountManager() = default;

// 通用：网易云 weapi POST

void AccountManager::weapiPost(const QString &path, const QJsonObject &json)
{
    WeCrypto::WeapiPayload payload = WeCrypto::makeWeapi(json);
    QUrl url(QStringLiteral("https://music.163.com") + path + QStringLiteral("?csrf_token="));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    req.setRawHeader("User-Agent", kUa);
    req.setRawHeader("Referer", "https://music.163.com/");
    req.setRawHeader("Origin", "https://music.163.com");
    req.setRawHeader("Accept", "*/*");
    req.setRawHeader("Accept-Encoding", "identity"); // 避免 gzip，方便直接解析 JSON
    req.setRawHeader("Sec-Fetch-Site", "same-origin");
    req.setRawHeader("Sec-Fetch-Mode", "cors");
    req.setRawHeader("Sec-Fetch-Dest", "empty");
    req.setRawHeader("sec-ch-ua", "\"Not A(Brand\";v=\"99\", \"Google Chrome\";v=\"120\", \"Chromium\";v=\"120\"");
    req.setRawHeader("sec-ch-ua-mobile", "?0");
    req.setRawHeader("sec-ch-ua-platform", "\"Windows\"");
    req.setRawHeader("Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8");

    // 手动拼完整 Cookie 链（jar 里的 NMTID 等 + os=pc），避免被风控拦截
    QString cookieStr;
    const QList<QNetworkCookie> cookies =
        m_jar->cookiesForUrl(QUrl(QStringLiteral("https://music.163.com")));
    for (const QNetworkCookie &c : cookies) {
        if (c.name().isEmpty())
            continue;
        cookieStr += QString::fromLatin1(c.name()) + QLatin1Char('=') +
                     QString::fromLatin1(c.value()) + QStringLiteral("; ");
    }
    if (!cookieStr.contains(QStringLiteral("os=pc")))
        cookieStr += QStringLiteral("os=pc; ");
    if (!cookieStr.contains(QStringLiteral("appver=")))
        cookieStr += QStringLiteral("appver=8.9.1; ");
    req.setRawHeader("Cookie", cookieStr.toUtf8());

    QByteArray body = "params=" + payload.params.toUtf8() + "&encSecKey=" + payload.encSecKey.toUtf8();
    qDebug() << "[weapi] POST" << url.toString()
             << "cookie:" << cookieStr
             << "params len:" << payload.params.size();
    QNetworkReply *reply = m_nam->post(req, body);
    if (path.contains(QStringLiteral("unikey"))) {
        connect(reply, &QNetworkReply::finished, this,
                [this, reply] { onNeteaseUnikey(reply); });
    } else {
        connect(reply, &QNetworkReply::finished, this,
                [this, reply] { onNeteasePoll(reply); });
    }
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

    QByteArray sig = WeCrypto::kugouWebSignature(params);
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
    m_neteaseUnikey.clear();
    setNeteaseQr(QrWaiting, QStringLiteral("正在获取二维码…"));

    // 种下 os=pc 基础 Cookie（weapi 风控要求，让 QNAM 自动附带）
    QList<QNetworkCookie> baseCookies;
    QNetworkCookie osCookie(QStringLiteral("os").toUtf8(), QStringLiteral("pc").toUtf8());
    osCookie.setDomain(QStringLiteral(".music.163.com"));
    osCookie.setPath(QStringLiteral("/"));
    baseCookies << osCookie;
    QNetworkCookie verCookie(QStringLiteral("appver").toUtf8(), QStringLiteral("8.9.1").toUtf8());
    verCookie.setDomain(QStringLiteral(".music.163.com"));
    verCookie.setPath(QStringLiteral("/"));
    baseCookies << verCookie;
    m_jar->setCookiesFromUrl(baseCookies, QUrl(QStringLiteral("https://music.163.com")));

    // 先访问主页，让 cookieJar 收集 NMTID 等基础 Cookie（weapi 缺少会被风控拦截返回空）
    QNetworkRequest pre(QUrl(QStringLiteral("https://music.163.com")));
    pre.setRawHeader("User-Agent", kUa);
    pre.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    QNetworkReply *reply = m_nam->get(pre);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        const int httpCode =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "[netease] 预热主页完成 http:" << httpCode
                 << "错误:" << reply->errorString();
        if (reply->error() != QNetworkReply::NoError) {
            setNeteaseQr(QrError, QStringLiteral("访问网易云失败：%1").arg(reply->errorString()));
            return;
        }
        weapiPost(QStringLiteral("/weapi/login/qrcode/unikey"),
                  QJsonObject{{QStringLiteral("type"), QStringLiteral("1")}});
    });
}

void AccountManager::cancelNeteaseQrLogin()
{
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

    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Netease"));
    s.remove(QStringLiteral("cookie"));
    s.remove(QStringLiteral("nickname"));
    s.remove(QStringLiteral("avatar"));
    s.endGroup();
    s.sync();

    setNeteaseQr(QrWaiting, QStringLiteral("已退出登录"));
    emit neteaseLoginChanged();
    emit message(QStringLiteral("已退出网易云账号"), 0);
}

void AccountManager::pollNetease()
{
    if (m_neteaseUnikey.isEmpty())
        return;
    weapiPost(QStringLiteral("/weapi/login/qrcode/client/login"),
              QJsonObject{{QStringLiteral("csrf_token"), QString()},
                          {QStringLiteral("key"), m_neteaseUnikey},
                          {QStringLiteral("type"), QStringLiteral("1")}});
}

void AccountManager::onNeteaseUnikey(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "[netease] unikey 网络错误:" << reply->errorString();
        setNeteaseQr(QrError, QStringLiteral("网络错误：%1").arg(reply->errorString()));
        return;
    }
    QByteArray raw = reply->readAll();
    qDebug() << "[netease] unikey 响应:" << QString::fromUtf8(raw).left(200);
    QJsonDocument doc = QJsonDocument::fromJson(raw);
    QJsonObject obj = doc.object();
    if (obj.value(QStringLiteral("code")).toInt() != 200) {
        qWarning() << "[netease] unikey 业务失败 code:" << obj.value("code").toInt()
                   << "msg:" << obj.value("message").toString();
        setNeteaseQr(QrError, QStringLiteral("获取二维码失败：%1")
                                 .arg(obj.value(QStringLiteral("message")).toString()));
        return;
    }
    m_neteaseUnikey = obj.value(QStringLiteral("unikey")).toString();
    if (m_neteaseUnikey.isEmpty()) {
        qWarning() << "[netease] unikey 为空";
        setNeteaseQr(QrError, QStringLiteral("二维码参数为空"));
        return;
    }
    QString qrText = QStringLiteral("https://music.163.com/login?codekey=") + m_neteaseUnikey;
    setNeteaseQr(QrWaiting, QStringLiteral("请使用网易云音乐App扫码登录"), qrText);
    m_neteasePollTimer->start();
}

void AccountManager::onNeteasePoll(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        // 网络抖动不打断轮询
        qWarning() << "[netease] poll 网络错误:" << reply->errorString();
        return;
    }
    QByteArray raw = reply->readAll();
    qDebug() << "[netease] poll 响应:" << QString::fromUtf8(raw).left(200);
    QJsonDocument doc = QJsonDocument::fromJson(raw);
    QJsonObject obj = doc.object();
    int code = obj.value(QStringLiteral("code")).toInt(-1);
    switch (code) {
    case 800: // 过期
        m_neteasePollTimer->stop();
        setNeteaseQr(QrExpired, QStringLiteral("二维码已过期，请点击重新获取"));
        break;
    case 801: // 等待
        setNeteaseQr(QrWaiting, QStringLiteral("请使用网易云音乐App扫码登录"));
        break;
    case 802: { // 已扫码
        QString nick = obj.value(QStringLiteral("nickname")).toString();
        setNeteaseQr(QrScanned, nick.isEmpty() ? QStringLiteral("已扫码，请在手机上确认")
                                               : QStringLiteral("%1 正在确认登录").arg(nick));
        break;
    }
    case 803: { // 成功
        m_neteasePollTimer->stop();
        m_neteaseCookie = buildNeteaseCookieString();
        persistNetease();
        m_neteaseLoggedIn = true;
        setNeteaseQr(QrSuccess, QStringLiteral("登录成功"));
        emit neteaseLoginChanged();
        emit message(QStringLiteral("网易云账号登录成功"), 1);

        // 获取用户信息
        QNetworkRequest req(QUrl(QStringLiteral("https://music.163.com/api/nuser/account/get")));
        req.setRawHeader("User-Agent", kUa);
        req.setRawHeader("Referer", "https://music.163.com/");
        QNetworkReply *profileReply = m_nam->get(req);
        connect(profileReply, &QNetworkReply::finished, this,
                [this, profileReply] { onNeteaseProfile(profileReply); });
        break;
    }
    default:
        setNeteaseQr(QrError, obj.value(QStringLiteral("message")).toString(QStringLiteral("未知状态")));
        break;
    }
}

void AccountManager::onNeteaseProfile(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError)
        return;
    QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
    if (obj.value(QStringLiteral("code")).toInt() != 200)
        return;
    QJsonObject profile = obj.value(QStringLiteral("profile")).toObject();
    if (profile.isEmpty())
        return;
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
        m_kugouGuid = WeCrypto::randomGuid();
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
    m_kugouMid = WeCrypto::kugouMidFromGuid(m_kugouGuid);

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

    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Kugou"));
    s.remove(QStringLiteral("cookie"));
    s.remove(QStringLiteral("nickname"));
    s.remove(QStringLiteral("avatar"));
    s.endGroup();
    s.sync();

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
    qDebug() << "[kugou] qrcode 响应:" << QString::fromUtf8(raw).left(300);
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
    qDebug() << "[kugou] poll 响应:" << QString::fromUtf8(raw);
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
        qDebug() << "[kugou] 登录成功 data keys:" << data.keys();
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

QString AccountManager::buildNeteaseCookieString() const
{
    QStringList parts;
    const QList<QNetworkCookie> cookies =
        m_jar->cookiesForUrl(QUrl(QStringLiteral("https://music.163.com")));
    for (const QNetworkCookie &c : cookies) {
        if (c.name().isEmpty() || c.value().isEmpty())
            continue;
        parts << QString::fromLatin1(c.name() + "=" + c.value());
    }
    return parts.join(QStringLiteral("; "));
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
    QSettings s(m_configPath, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Netease"));
    m_neteaseCookie = s.value(QStringLiteral("cookie")).toString();
    m_neteaseNickname = s.value(QStringLiteral("nickname")).toString();
    m_neteaseAvatar = s.value(QStringLiteral("avatar")).toString();
    s.endGroup();

    if (m_neteaseCookie.isEmpty())
        return;

    // 恢复 cookie 到 jar
    QList<QNetworkCookie> cookies;
    const QStringList parts = m_neteaseCookie.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        QString t = p.trimmed();
        int eq = t.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        QNetworkCookie c(t.left(eq).trimmed().toUtf8(), t.mid(eq + 1).trimmed().toUtf8());
        c.setDomain(QStringLiteral(".music.163.com"));
        c.setPath(QStringLiteral("/"));
        cookies << c;
    }
    m_jar->setCookiesFromUrl(cookies, QUrl(QStringLiteral("https://music.163.com")));
    m_neteaseLoggedIn = true;

    // 异步刷新昵称
    QNetworkRequest req(QUrl(QStringLiteral("https://music.163.com/api/nuser/account/get")));
    req.setRawHeader("User-Agent", kUa);
    req.setRawHeader("Referer", "https://music.163.com/");
    QNetworkReply *r = m_nam->get(req);
    connect(r, &QNetworkReply::finished, this, [this, r] { onNeteaseProfile(r); });
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
        m_kugouMid = WeCrypto::kugouMidFromGuid(m_kugouGuid);
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
    m_neteaseCookie = cookie.trimmed();
    m_neteaseNickname = nickname.trimmed().isEmpty() ? QStringLiteral("网易云音乐用户")
                                                     : nickname.trimmed();
    m_neteaseAvatar = avatar.trimmed();
    m_neteaseLoggedIn = true;

    // 把 Cookie 同步进 jar，供后续 QNetworkAccessManager 请求自动携带
    QList<QNetworkCookie> cookies;
    const QStringList parts = m_neteaseCookie.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        QString t = p.trimmed();
        int eq = t.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        QNetworkCookie c(t.left(eq).trimmed().toUtf8(), t.mid(eq + 1).trimmed().toUtf8());
        c.setDomain(QStringLiteral(".music.163.com"));
        c.setPath(QStringLiteral("/"));
        cookies << c;
    }
    m_jar->setCookiesFromUrl(cookies, QUrl(QStringLiteral("https://music.163.com")));

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

