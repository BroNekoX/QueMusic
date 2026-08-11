#include "plugins.h"

#include "util/request.h"
#include "util/option.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRegularExpression>

using namespace QCloudMusicApi;
Plugins::Plugins(Request* request, QObject* parent)
    : QObject{ parent }
{
    m_request = request;
}

QVariantMap Plugins::request(QString uri, QVariantMap data, QVariantMap options)
{
    return m_request->createRequest(uri, data, options);
}

QVariantMap Plugins::songUpload(QVariantMap query)
{
    QString ext = "mp3";
    if (query.value("songFile").toMap()["name"].toString().toLower().indexOf("flac") > -1) {
        ext = "flac";
    }
    QString filename = query.value("songFile").toMap()["name"].toString()
        .replace("." + ext, "")
        .replace(QRegularExpression("\\s"), "")
        .replace(QRegularExpression("\\."), "_");
    const QString bucket = "jd-musicrep-privatecloud-audio-public";
    //   获取key和token
    const auto tokenRes = request(
        "/api/nos/token/alloc",
        {
            { "bucket", bucket },
            { "ext", ext },
            { "filename", filename },
            { "local", false },
            { "nos_product", 3 },
            { "type", "audio" },
            { "md5", query["songFile"].toMap()["md5"] },
        },
        Option::createOption(query, "weapi")
        );

    // 上传
    const auto objectKey = tokenRes["body"].toMap()["result"].toMap()["objectKey"].toString().replace("/", "%2F");

    auto reply = m_request->axios(
        QNetworkAccessManager::GetOperation,
        "https://wanproxy.127.net/lbs?version=1.0&bucketname=" + bucket,
        {}, {}, "");
    const QVariantMap lbs = QJsonDocument::fromJson(reply->readAll()).toVariant().toMap();
    reply = m_request->axios(
        QNetworkAccessManager::PostOperation,
        lbs["upload"].toList().value(0).toString() + "/" + bucket + "/" + objectKey + "?offset=0&complete=true&version=1.0",
        {},
        {
            { "x-nos-token", tokenRes["body"].toMap()["result"].toMap()["token"] },
            { "Content-MD5", query["songFile"].toMap()["md5"] },
            { "Content-Type", "audio/mpeg" },
            { "Content-Length", query["songFile"].toMap()["size"] },
        },
        query["songFile"].toMap()["data"].toByteArray());

    return tokenRes;
}

QVariantMap Plugins::upload(QVariantMap query)
{
    const QVariantMap data{
        { "bucket", "yyimgs" },
        { "ext", "jpg" },
        { "filename", query["imgFile"].toMap()["name"] },
        { "local", false },
        { "nos_product", 0 },
        { "return_body", "{\"code\":200,\"size\":\"$(ObjectSize)\"}" },
        { "type", "other" }
    };
    //   获取key和token
    const auto res = request(
        "/api/nos/token/alloc",
        data,
        Option::createOption(query, "weapi")
    );
    auto reply = m_request->axios(QNetworkAccessManager::PostOperation,
        "https://nosup-hz1.127.net/yyimgs/"
        + res["body"].toMap()["result"].toMap()["objectKey"].toString()
        + "?offset=0&complete=true&version=1.0",
        {},
                                {
                                    { "x-nos-token", res["body"].toMap()["result"].toMap()["token"] },
                                    { "Content-Type", "image/jpeg" }
                                },
        query["imgFile"].toMap()["data"].toByteArray());

    // 读取响应内容
    auto body = reply->readAll();
    qDebug().noquote() << body.isEmpty() << "body" << body;

    QVariantMap res2 = QJsonDocument::fromJson(body).toVariant().toMap();

    return {
        // ...res.body.result,
        // ...res2.data,
        // ...res3.body,
        { "url_pre", "https://p1.music.126.net/" + res["body"].toMap()["result"].toMap()["objectKey"].toString() },
        { "imgId", res["body"].toMap()["result"].toMap()["id"] },
    };
}
