#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QVariantMap>
#include <QByteArray>
#include <QRandomGenerator>
#include <QNetworkCookie>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QThread>
#include <QScopeGuard>

#include "crypto.h"
#include "index.h"
#include "request.h"
#include "logger.h"
#include "config.h"

QString anonymous_token;

const QVariantMap osMap{
    { "pc", QVariantMap {
        { "os", "pc" },
        { "appver", "3.0.18.203152" },
        { "osver", "Microsoft-Windows-10-Professional-build-22631-64bit" },
        } },
    { "android", QVariantMap {
        { "os", "android" },
        { "appver", "8.20.20.231215173437" },
        { "osver", "14" },
        } },
    { "iphone", QVariantMap {
        { "os", "iPhone OS" },
        { "appver", "9.0.90" },
        { "osver", "16.2" },
        } },
};

using namespace QCloudMusicApi;

namespace {

QString generateStaticDeviceId() {
    static const QString hexChars = QStringLiteral("0123456789ABCDEF");
    QString deviceId;
    deviceId.reserve(52);
    for (int i = 0; i < 52; ++i) {
        const int index = QRandomGenerator::global()->bounded(hexChars.size());
        deviceId.append(hexChars.at(index));
    }
    return deviceId;
}

const QString kStaticDeviceId = generateStaticDeviceId();

}

Request::Request(QObject *parent): QObject(parent)
{
    m_thread = new QThread;
    m_networkAccessManager = new QNetworkAccessManager;
    m_networkAccessManager->moveToThread(m_thread);
    m_thread->start();
}

Request::~Request()
{
    if (m_networkAccessManager) {
        if (m_networkAccessManager->thread() == QThread::currentThread() || !m_thread || !m_thread->isRunning()) {
            delete m_networkAccessManager;
        }
        else {
            QMetaObject::invokeMethod(m_networkAccessManager, [manager = m_networkAccessManager]() {
                delete manager;
            }, Qt::BlockingQueuedConnection);
        }
        m_networkAccessManager = nullptr;
    }

    if (m_thread) {
        m_thread->quit();
        if (QThread::currentThread() != m_thread) {
            m_thread->wait();
            delete m_thread;
        }
        else {
            m_thread->deleteLater();
        }
        m_thread = nullptr;
    }
}

QString Request::chooseUserAgent(QString crypto, QString uaType) {
    const QVariantMap userAgentMap{
        {
            "weapi", QVariantMap{
                { "pc", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36 Edg/124.0.0.0" },
            }
        },
        {
            "api", QVariantMap{
                { "pc", "Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Safari/537.36 Chrome/91.0.4472.164 NeteaseMusicDesktop/3.0.18.203152" },
                { "android", "NeteaseMusic/9.1.65.240927161425(9001065);Dalvik/2.1.0 (Linux; U; Android 14; 23013RK75C Build/UKQ1.230804.001)" },
                { "iphone", "NeteaseMusic 9.0.90/5038 (iPhone; iOS 16.2; zh_CN)" },
            }
        },
    };
    return userAgentMap[crypto].toMap()[uaType].toString();
}

QVariantMap Request::createRequest(
    QString uri,
    QVariantMap data,
    QVariantMap options) {
    QNetworkAccessManager::Operation method = QNetworkAccessManager::PostOperation;
    DEBUG.noquote() <<
        QJsonDocument::fromVariant(
            QVariantMap{
                { "method", method },
                { "uri", uri },
                { "data", data },
                { "options", options}
            }
        ).toJson();
    QVariantMap cookie = options.value("cookie", QVariantMap()).toMap();
    QVariantMap headers{
        { "Content-Type", "application/x-www-form-urlencoded" },
    };
    options["headers"] = options.value("headers", QVariantMap()).toMap();
    headers = Index::mergeMap(headers, options["headers"].toMap());

    QString ip = options.value("realIP", options.value("ip", "")).toString();

    if (!ip.isEmpty()) {
        headers["X-Real-IP"] = ip;
        headers["X-Forwarded-For"] = ip;
    }
    // headers["X-Real-IP"] = "118.88.88.88";
    auto randomBytes = [](int num) {
        QByteArray bytes;
        for (int i = 0; i < num; ++i) {
            bytes.append(QRandomGenerator::global()->generate());
        }
        return bytes.toHex();
    };

    auto _ntes_nuid = randomBytes(32);
    QString deviceId = cookie.value("deviceId").toString();
    if (deviceId.isEmpty()) {
        deviceId = kStaticDeviceId;
    }
    auto os = osMap.value(cookie.value("os").toString(), osMap["iphone"]).toMap();
    cookie =
        Index::mergeMap(cookie,
                        {
                            { "__remember_me", "true" },
                            // { "NMTID", randomBytes(16) },
                            { "ntes_kaola_ad", "1" },
                            { "_ntes_nuid", _ntes_nuid },
                            { "_ntes_nnid", _ntes_nuid + "," + QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()) },

                            { "osver", cookie.contains("osver") ? cookie["osver"] : os["osver"] }, //系统版本
                            { "deviceId", deviceId },
                            { "os", cookie.contains("os") ? cookie["os"] : os["os"] },
                            { "channel", cookie.contains("channel") ? cookie["channel"] : "netease" },
                            { "appver", cookie.contains("appver") ? cookie["appver"] : os["appver"] }, // app版本
                        });
    if (uri.indexOf("login") == -1) {
        cookie["NMTID"] = randomBytes(16);
    }
    if (!cookie.contains("MUSIC_U")) {
        // 游客
        if (!cookie.contains("MUSIC_A")) {
            cookie["MUSIC_A"] = []() -> QString {
                if (anonymous_token.isEmpty()) {
                    QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
                    QFile file(QDir(tmpPath).absoluteFilePath("anonymous_token"));
                    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) anonymous_token = file.readAll();
                }
                return anonymous_token;
            }();
        }
    }
    headers["cookie"] = Index::cookieObjToString(cookie);

    QString url = "";
    QVariantMap encryptData;
    auto crypto = options.value("crypto", "");
    auto csrfToken = cookie.value("__csrf", "");

    if (crypto == "") {
        // 加密方式为空，以配置文件的加密方式为准
        if (Config::APP_CONF["encrypt"].toBool()) {
            crypto = "eapi";
        }
        else {
            crypto = "api";
        }
    }

    auto domain = options.value("domain", "").toString();
    // 根据加密方式加密请求数据；目前任意uri都支持四种加密方式
    if (crypto == "weapi") {
        headers["Referer"] = Config::APP_CONF["domain"];
        headers["User-Agent"] = options.value("ua", chooseUserAgent("weapi"));
        data["csrf_token"] = csrfToken;

        encryptData = Crypto::weapi(QJsonDocument::fromVariant(data));
        url = (!domain.isEmpty()
                   ? domain
                   : Config::APP_CONF["domain"].toString()) + "/weapi/" + uri.mid(5);
    }
    else if (crypto == "linuxapi") {
        encryptData = Crypto::linuxapi(QJsonDocument::fromVariant(QVariantMap{
            { "method", method },
            { "url", Config::APP_CONF["domain"].toString() + uri },
            { "params", data }
            }));
        headers["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/60.0.3112.90 Safari/537.36";
        url = (!domain.isEmpty()
                   ? domain
                   : Config::APP_CONF["domain"].toString()) + "/api/linux/forward";
    }
    else if (crypto == "eapi" || crypto == "api") {
        // 两种加密方式，都应生成客户端的cookie
        QVariantMap header{
            { "osver", cookie.value("osver") }, //系统版本
            { "deviceId", cookie["deviceId"] },
            { "os", cookie.value("os") },
            { "appver", cookie.value("appver") }, // app版本
            { "versioncode", cookie.value("versioncode", "140") }, //版本号
            { "mobilename", cookie.value("mobilename", "") }, //设备model
            { "buildver", cookie.value("buildver", QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()).mid(0, 10)) },
            { "resolution", cookie.value("resolution", "1920x1080") }, //设备分辨率
            { "__csrf", csrfToken },
            { "channel", cookie.value("channel") },
            { "requestId", QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch())
                              + "_"
                              + QString::number((int)(QRandomGenerator::global()->bounded(1.0) * 1000)).rightJustified(4, '0')
            },
        };
        if (cookie.contains("MUSIC_U")) header["MUSIC_U"] = cookie["MUSIC_U"];
        if (cookie.contains("MUSIC_A")) header["MUSIC_A"] = cookie["MUSIC_A"];

        headers["Cookie"] = [&header]() -> QString {
            QStringList result;
            for (auto i = header.constBegin(); i != header.constEnd(); i++) {
                result.push_back(
                    QUrl::toPercentEncoding(i.key()) + "=" + QUrl::toPercentEncoding(i.value().toString())
                    );
            }
            return result.join("; ");
        }();
        headers["User-Agent"] = !options.value("ua").toString().isEmpty() ? options.value("ua") : chooseUserAgent("api", "iphone");
        if (crypto == "eapi") {
            // 使用eapi加密
            // headers["x-aeapi"] = true; // 服务器会使用gzip压缩返回值
            data["header"] = header;
            data["e_r"] = options.value("e_r").isValid() ? options.value("e_r") : data.value("e_r", Config::APP_CONF.value("encryptResponse")); // 用于加密eapi接口的返回值
            data["e_r"] = Index::toBoolean(data["e_r"]);
            encryptData = Crypto::eapi(uri, QJsonDocument::fromVariant(data));
            url = (!domain.isEmpty()
                       ? domain
                       : Config::APP_CONF["apiDomain"].toString()) + "/eapi/" + uri.mid(5);
        }
        else if (crypto == "api") {
            // 不使用任何加密
            url = (!domain.isEmpty()
                       ? domain
                       : Config::APP_CONF["apiDomain"].toString()) + uri;
            encryptData = data;
        }
    }
    else {
        // 未知的加密方式
        DEBUG << "[ERR]" << "Unknown Crypto:" << options["crypto"].toString();
    }

    QNetworkProxy proxy = QNetworkProxy::NoProxy;
    if (options.contains("proxy") && !options["proxy"].isNull()) {
        QUrl purl(options["proxy"].toString());
        if (!purl.host().isEmpty()) {
            proxy.setType(QNetworkProxy::HttpProxy);
            proxy.setHostName(purl.host());
            proxy.setPort(purl.port(80));

        }
        else {
            proxy = QNetworkProxy::DefaultProxy;
            DEBUG << "代理配置无效，不使用代理";
        }
    }

    QUrlQuery query;
    query.setQuery(QUrl(url).query());
    for (auto i = encryptData.constBegin(); i != encryptData.constEnd(); ++i) {
        query.addQueryItem(i.key(), QUrl::toPercentEncoding(i.value().toString()));
    }
    DEBUG << "method" << method;
    DEBUG << "url" << url;
    DEBUG << "data" << encryptData;
    QNetworkReply* reply = axios(method, url, encryptData, headers, query.toString().toUtf8(), proxy);
    auto guard = qScopeGuard([=]() { reply->deleteLater(); });

    QVariantMap answer{
        { "status", 500 },
        { "body", {} },
        { "cookie", {} }
    };

    if (reply->error() != QNetworkReply::NoError) { // http请求出错，进行错误处理
        answer["body"] = QVariantMap{
            { "code", 502 },
            { "msg", reply->errorString() }
        };
        answer["status"] = 502;
    }
    else {
        // 打印响应头
        DEBUG.noquote() << reply->rawHeaderPairs();

        // 读取响应内容
        auto body = reply->readAll();
        DEBUG.noquote() << "body" << body;

        QString cookie;
        auto setCookie = QString(reply->rawHeader("set-cookie"));
        for (auto& i : setCookie.split("\n")) {
            cookie.append(i.replace(QRegularExpression("\\s*Domain=[^;]*"), "") + ";");
        }
        answer["cookie"] = cookie;

        if (data["e_r"].toBool()) {
            // eapi接口返回值被加密，需要解密
            const QByteArray aeapiHeader = reply->rawHeader("x-aeapi").toLower();
            const bool aeapi = aeapiHeader == "true" || aeapiHeader == "1";
            answer["body"] = Crypto::eapiResDecrypt(body.toHex().toUpper(), aeapi);
        }
        else {
            answer["body"] = QJsonDocument::fromJson(body).toVariant().toMap();
        }
        if (answer["body"].toMap().isEmpty()) {
            answer["body"] = body;
        }
        answer["status"] = answer["body"].toMap().value("code", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
        if (QList<int> { 201, 302, 400, 502, 800, 801, 802, 803 }.indexOf(answer["body"].toMap()["code"].toInt()) > -1) {
            // 特殊状态码
            answer["status"] = 200;
        }

        answer["status"] = 100 < answer["status"].toInt() && answer["status"].toInt() < 600
            ? answer["status"]
            : 400;
    }
    return answer;
}

QNetworkReply* Request::axios(QNetworkAccessManager::Operation method,
    QString url,
    const QVariantMap& urlQuery,
    const QVariantMap& headers,
    const QByteArray& data,
    QNetworkProxy proxy)
{
    QNetworkRequest request;
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(30000);
#endif
    for (auto i = headers.constBegin(); i != headers.constEnd(); i++) {
        request.setRawHeader(i.key().toUtf8(), i.value().toByteArray());
    }
    QMetaObject::invokeMethod(m_networkAccessManager, [=]() { m_networkAccessManager->setProxy(proxy); }, Qt::BlockingQueuedConnection);

    QUrlQuery query;
    QUrl qurl(url);
    query.setQuery(qurl.query());
    if (method != QNetworkAccessManager::PostOperation) {
        for (auto i = urlQuery.constBegin(); i != urlQuery.constEnd(); ++i) {
            query.addQueryItem(i.key(), i.value().toString());
        }
    }
    qurl.setQuery(query);
    request.setUrl(qurl);

    // 发送HTTP请求
    QNetworkReply* reply;
    if (method == QNetworkAccessManager::PostOperation) {
        QMetaObject::invokeMethod(m_networkAccessManager, [&]() { reply = m_networkAccessManager->post(request, data); }, Qt::BlockingQueuedConnection);
    }
    else {
        QMetaObject::invokeMethod(m_networkAccessManager, [&]() { reply = m_networkAccessManager->get(request); }, Qt::BlockingQueuedConnection);
    }

    // 开启一个局部的事件循环，等待响应结束，退出
    QEventLoop eventLoop;
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, [=]() { if (!reply->isFinished()) reply->abort(); });
    timer.start(30000);
#endif

    eventLoop.exec(); // 启动事件循环

    return reply;
}
