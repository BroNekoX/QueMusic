// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// WeCrypto — 网易云 weapi 加密 / 酷狗签名 等在线服务需要的密码学工具
// 仅用于实现"用户自己的账号授权登录"，不绕开任何版权保护。
#ifndef WECRYPTO_H
#define WECRYPTO_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace WeCrypto {

// AES-128-CBC
// 输入明文 bytes，输出 base64 字符串（PKCS7 填充）
QByteArray aesCbcEncryptBase64(const QByteArray &plain, const QByteArray &key16,
                               const QByteArray &iv16);

// RSA
// 用网易云公钥对数据做 RSA 加密（e = 0x010001），输出 256 位 hex（128 字节大端）
QByteArray rsaEncryptHex(const QByteArray &dataBigEndian);

// 网易云 weapi
struct WeapiPayload {
    QString params;      // urlencoded 后作为请求体
    QString encSecKey;
};
// 将 JSON 参数加密为 weapi 请求体（params=xxx&encSecKey=xxx 已做 form-urlencode）
WeapiPayload makeWeapi(const QJsonObject &json);

// 酷狗
// 酷狗 web 版签名：MD5("NVPh5oo..." + 排序后的 key=value 串 + "NVPh5oo...")
QByteArray kugouWebSignature(const QJsonObject &params);

// 设备 mid：MD5(guid) 的 16 进制大整数 → 十进制字符串
QString kugouMidFromGuid(const QString &guid);

// 生成随机 GUID（UUID v4 风格，酷狗设备标识）
QString randomGuid();

// 工具
QByteArray md5Hex(const QByteArray &data);

} // namespace WeCrypto

#endif // WECRYPTO_H
