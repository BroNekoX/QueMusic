#ifndef CRYPTO_H
#define CRYPTO_H

#include "../qcloudmusicapiglobal.h"

#include <QByteArray>
#include <QVariantMap>

namespace QCloudMusicApi {

    // 加解密管理类，参考原项目文件util/crypto.js
    class QCLOUDMUSICAPI_EXPORT Crypto {
    public:
        static const QString iv;
        static const QString presetKey;
        static const QString linuxapiKey;
        static const QString base62;
        static const QString publicKey;
        static const QString eapiKey;

        static QByteArray aesEncrypt(const QByteArray& plainText, const QString mode, const QByteArray& key, const QByteArray& iv, QString format = "base64");

        static QByteArray aesDecrypt(const QByteArray& cipherText, const QString mode, const QByteArray& key, const QByteArray& iv, QString format = "base64");

        static QByteArray rsaEncrypt(const QString& plainText, const QString& strPubKey);

        static QVariantMap weapi(QJsonDocument object);

        static QVariantMap linuxapi(QJsonDocument object);

        static QVariantMap eapi(QString url, QJsonDocument object);

        static QVariantMap eapiResDecrypt(const QByteArray& encryptedParams, bool aeapi = false);

        static QVariantMap eapiReqDecrypt(const QByteArray& encryptedParams);

        static QByteArray decrypt(QByteArray cipherBuffer);
    };

}
#endif // CRYPTO_H
