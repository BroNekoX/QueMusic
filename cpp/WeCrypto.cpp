// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "WeCrypto.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QtEndian>

#include <algorithm>
#include <array>
#include <vector>

namespace WeCrypto {

// AES-128 (FIPS-197)

// clang-format off
static const unsigned char SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};
static const unsigned char RCON[10] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36};
// clang-format on

static inline unsigned char xtime(unsigned char b)
{
    unsigned char r = b << 1;
    if (b & 0x80)
        r ^= 0x1b;
    return r;
}

class Aes128
{
public:
    explicit Aes128(const QByteArray &key)
    {
        for (int i = 0; i < 16; ++i)
            m_key[i] = static_cast<unsigned char>(key.at(i));
        expandKey();
    }

    void encryptBlock(const unsigned char in[16], unsigned char out[16]) const
    {
        unsigned char s[16];
        for (int i = 0; i < 16; ++i)
            s[i] = in[i];
        addRoundKey(s, 0);

        for (int round = 1; round < 10; ++round) {
            subBytes(s);
            shiftRows(s);
            mixColumns(s);
            addRoundKey(s, round);
        }

        subBytes(s);
        shiftRows(s);
        addRoundKey(s, 10);
        for (int i = 0; i < 16; ++i)
            out[i] = s[i];
    }

private:
    unsigned char m_key[16];
    unsigned char m_roundKeys[176];

    void expandKey()
    {
        for (int i = 0; i < 16; ++i)
            m_roundKeys[i] = m_key[i];

        int rconIdx = 0;
        for (int i = 16; i < 176; i += 4) {
            unsigned char t[4];
            for (int j = 0; j < 4; ++j)
                t[j] = m_roundKeys[i - 4 + j];

            if (i % 16 == 0) {
                // RotWord
                unsigned char tmp = t[0];
                t[0] = t[1]; t[1] = t[2]; t[2] = t[3]; t[3] = tmp;
                // SubWord
                for (int j = 0; j < 4; ++j)
                    t[j] = SBOX[t[j]];
                // Rcon
                t[0] ^= RCON[rconIdx++];
            }

            for (int j = 0; j < 4; ++j)
                m_roundKeys[i + j] = m_roundKeys[i + j - 16] ^ t[j];
        }
    }

    void addRoundKey(unsigned char s[16], int round) const
    {
        for (int i = 0; i < 16; ++i)
            s[i] ^= m_roundKeys[round * 16 + i];
    }

    static void subBytes(unsigned char s[16])
    {
        for (int i = 0; i < 16; ++i)
            s[i] = SBOX[s[i]];
    }

    static void shiftRows(unsigned char s[16])
    {
        // 行 1 左移 1
        unsigned char t = s[1]; s[1] = s[5]; s[5] = s[9]; s[9] = s[13]; s[13] = t;
        // 行 2 左移 2
        t = s[2]; s[2] = s[10]; s[10] = t;
        t = s[6]; s[6] = s[14]; s[14] = t;
        // 行 3 左移 3（等价右移 1）
        t = s[15]; s[15] = s[11]; s[11] = s[7]; s[7] = s[3]; s[3] = t;
    }

    static void mixColumns(unsigned char s[16])
    {
        for (int c = 0; c < 16; c += 4) {
            unsigned char a0 = s[c], a1 = s[c + 1], a2 = s[c + 2], a3 = s[c + 3];
            unsigned char a0b = xtime(a0), a1b = xtime(a1), a2b = xtime(a2), a3b = xtime(a3);
            s[c]     = a0b ^ a1b ^ a1 ^ a2 ^ a3;
            s[c + 1] = a0 ^ a1b ^ a2b ^ a2 ^ a3;
            s[c + 2] = a0 ^ a1 ^ a2b ^ a3b ^ a3;
            s[c + 3] = a0b ^ a0 ^ a1 ^ a2 ^ a3b;
        }
    }
};

static QByteArray base64Encode(const QByteArray &data)
{
    return data.toBase64();
}

QByteArray aesCbcEncryptBase64(const QByteArray &plain, const QByteArray &key16,
                               const QByteArray &iv16)
{
    QByteArray key = key16;
    QByteArray iv = iv16;
    if (key.size() < 16) key = key.leftJustified(16, '\0', true);
    if (iv.size() < 16) iv = iv.leftJustified(16, '\0', true);

    Aes128 aes(key);

    // PKCS7 填充
    int padLen = 16 - (plain.size() % 16);
    if (padLen == 0) padLen = 16;
    QByteArray padded = plain;
    padded.append(QByteArray(padLen, static_cast<char>(padLen)));

    QByteArray result;
    result.reserve(padded.size());

    unsigned char block[16];
    unsigned char prev[16];
    for (int i = 0; i < 16; ++i)
        prev[i] = static_cast<unsigned char>(iv.at(i));

    for (int off = 0; off < padded.size(); off += 16) {
        for (int i = 0; i < 16; ++i)
            block[i] = static_cast<unsigned char>(padded.at(off + i)) ^ prev[i];
        unsigned char out[16];
        aes.encryptBlock(block, out);
        for (int i = 0; i < 16; ++i) {
            result.append(static_cast<char>(out[i]));
            prev[i] = out[i];
        }
    }
    return base64Encode(result);
}

// 大整数（仅用于 RSA modpow 与 hex/dec 转换）

namespace {

class BigUint
{
public:
    // little-endian 32-bit words
    std::vector<quint32> w;

    BigUint() = default;
    explicit BigUint(quint32 v)
    {
        if (v)
            w.push_back(v);
    }

    static BigUint fromBytes(const QByteArray &be) // big-endian
    {
        BigUint r;
        if (be.isEmpty())
            return r;
        int n = (be.size() + 3) / 4;
        r.w.resize(n, 0);
        for (int i = 0; i < be.size(); ++i) {
            int wordIdx = (be.size() - 1 - i) / 4;
            int shift = (i % 4) * 8;
            r.w[wordIdx] |= (static_cast<quint32>(static_cast<unsigned char>(be.at(be.size() - 1 - i))) << shift);
        }
        r.normalize();
        return r;
    }

    QByteArray toBytes(int len) const // big-endian, 左补零
    {
        QByteArray out(len, '\0');
        for (int i = 0; i < len; ++i) {
            int bytePos = len - 1 - i;
            int wordIdx = bytePos / 4;
            int shift = (bytePos % 4) * 8;
            quint32 v = (wordIdx < static_cast<int>(w.size())) ? w[wordIdx] : 0;
            out[i] = static_cast<char>((v >> shift) & 0xff);
        }
        return out;
    }

    void normalize()
    {
        while (!w.empty() && w.back() == 0)
            w.pop_back();
    }

    int bitLength() const
    {
        if (w.empty())
            return 0;
        int top = w.size() - 1;
        quint32 v = w[top];
        int bits = 32;
        while ((v & 0x80000000u) == 0) {
            v <<= 1;
            --bits;
        }
        return top * 32 + bits;
    }

    bool bit(int i) const
    {
        int wordIdx = i / 32;
        if (wordIdx >= static_cast<int>(w.size()))
            return false;
        return (w[wordIdx] >> (i % 32)) & 1u;
    }

    void shl1()
    {
        quint32 carry = 0;
        for (auto &v : w) {
            quint32 next = v >> 31;
            v = (v << 1) | carry;
            carry = next;
        }
        if (carry)
            w.push_back(carry);
    }

    void addSmall(quint32 v)
    {
        quint64 carry = v;
        for (auto &word : w) {
            quint64 sum = static_cast<quint64>(word) + carry;
            word = static_cast<quint32>(sum & 0xffffffffu);
            carry = sum >> 32;
            if (!carry)
                break;
        }
        while (carry) {
            w.push_back(static_cast<quint32>(carry & 0xffffffffu));
            carry >>= 32;
        }
    }

    bool ge(const BigUint &o) const
    {
        if (w.size() != o.w.size())
            return w.size() > o.w.size();
        for (int i = static_cast<int>(w.size()) - 1; i >= 0; --i) {
            if (w[i] != o.w[i])
                return w[i] > o.w[i];
        }
        return true;
    }

    void sub(const BigUint &o) // 要求 *this >= o
    {
        qint64 borrow = 0;
        for (int i = 0; i < static_cast<int>(w.size()); ++i) {
            qint64 sv = static_cast<qint64>(w[i]) - borrow;
            qint64 ov = (i < static_cast<int>(o.w.size())) ? static_cast<qint64>(o.w[i]) : 0;
            qint64 r = sv - ov;
            if (r < 0) {
                r += 0x100000000LL;
                borrow = 1;
            } else {
                borrow = 0;
            }
            w[i] = static_cast<quint32>(r);
        }
        normalize();
    }

    // 模乘：二进制长乘法
    BigUint mulmod(const BigUint &b, const BigUint &n) const
    {
        BigUint r;
        for (int i = b.bitLength() - 1; i >= 0; --i) {
            // r = (r * 2) mod n
            r.shl1();
            if (r.ge(n))
                r.sub(n);
            if (b.bit(i)) {
                // r = (r + *this) mod n
                BigUint sum = r;
                for (int j = 0; j < static_cast<int>(w.size()); ++j) {
                    if (j >= static_cast<int>(sum.w.size()))
                        sum.w.push_back(w[j]);
                    else {
                        quint64 s = static_cast<quint64>(sum.w[j]) + w[j];
                        sum.w[j] = static_cast<quint32>(s & 0xffffffffu);
                        // 处理进位
                        quint64 carry = s >> 32;
                        int k = j + 1;
                        while (carry) {
                            if (k >= static_cast<int>(sum.w.size()))
                                sum.w.push_back(0);
                            quint64 ns = static_cast<quint64>(sum.w[k]) + carry;
                            sum.w[k] = static_cast<quint32>(ns & 0xffffffffu);
                            carry = ns >> 32;
                            ++k;
                        }
                    }
                }
                sum.normalize();
                if (sum.ge(n))
                    sum.sub(n);
                r = sum;
            }
        }
        r.normalize();
        return r;
    }

    BigUint modpow(const BigUint &exp, const BigUint &n) const
    {
        BigUint result(1);
        BigUint base = *this;
        for (int i = exp.bitLength() - 1; i >= 0; --i) {
            result = result.mulmod(result, n);
            if (exp.bit(i))
                result = result.mulmod(base, n);
        }
        return result;
    }

    QString toDecimalString() const
    {
        if (w.empty())
            return QStringLiteral("0");
        // 反复除以 10
        std::vector<quint32> cur = w;
        QString digits;
        while (!cur.empty()) {
            quint64 rem = 0;
            for (int i = static_cast<int>(cur.size()) - 1; i >= 0; --i) {
                quint64 v = (rem << 32) | cur[i];
                cur[i] = static_cast<quint32>(v / 10);
                rem = v % 10;
            }
            digits.prepend(QChar(static_cast<ushort>('0' + rem)));
            while (!cur.empty() && cur.back() == 0)
                cur.pop_back();
        }
        return digits;
    }
};

} // namespace

// RSA (网易云公钥)

QByteArray rsaEncryptHex(const QByteArray &dataBigEndian)
{
    // 网易云 web 端固定公钥
    static const char *N_HEX =
        "00e0b509f6259df8642dbc35662901477df22677ec152b5ff68ace615bb7b725"
        "152b3ab17a876aea8a5aa76d2e417629ec4ee341f56135fccf695280104e0312"
        "ecbda92557c93870114af6c9d05c4f7f0c3685b7a46bee255932575cce10b424"
        "d813cfe4875d3e82047b97ddef52741d546b8e289dc6935b3ece0462db0a22b8e7";
    BigUint n = BigUint::fromBytes(QByteArray::fromHex(N_HEX));
    BigUint e(0x010001);
    BigUint m = BigUint::fromBytes(dataBigEndian);
    BigUint c = m.modpow(e, n);
    return c.toBytes(128).toHex();
}

// 网易云 weapi

WeapiPayload makeWeapi(const QJsonObject &json)
{
    const QByteArray firstKey = "0CoJUm6Qyw8W8jud";
    const QByteArray iv = "0102030405060708";

    // 随机 16 字节 secKey（可打印字符范围 0-9a-zA-Z，与官方脚本一致）
    QByteArray secKey;
    secKey.reserve(16);
    const QByteArray pool = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 16; ++i) {
        int idx = QRandomGenerator::global()->bounded(pool.size());
        secKey.append(pool.at(idx));
    }

    // params = AES(AES(json, firstKey, iv), secKey, iv)
    QByteArray jsonBytes = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QByteArray once = aesCbcEncryptBase64(jsonBytes, firstKey, iv);
    QByteArray twice = aesCbcEncryptBase64(once, secKey, iv);

    // encSecKey = RSA(反转 secKey)
    QByteArray reversed = secKey;
    std::reverse(reversed.begin(), reversed.end());
    QByteArray enc = rsaEncryptHex(reversed);

    WeapiPayload payload;
    payload.params = QUrl::toPercentEncoding(QString::fromLatin1(twice));
    payload.encSecKey = QString::fromLatin1(enc);
    return payload;
}

// 酷狗签名 / mid / GUID

QByteArray kugouWebSignature(const QJsonObject &params)
{
    const QByteArray salt = "NVPh5oo715z5DIWAeQlhMDsWXXQV4hwt";
    QStringList keyValues;
    const QStringList keys = params.keys();
    for (const QString &k : keys) {
        QString v = QJsonValue(params.value(k)).toVariant().toString();
        keyValues << (k + QLatin1Char('=') + v);
    }
    keyValues.sort();
    QByteArray paramsStr = keyValues.join(QString()).toUtf8();
    return md5Hex(salt + paramsStr + salt);
}

QString kugouMidFromGuid(const QString &guid)
{
    QByteArray digest = QCryptographicHash::hash(guid.toUtf8(), QCryptographicHash::Md5);
    BigUint v = BigUint::fromBytes(digest);
    return v.toDecimalString();
}

QString randomGuid()
{
    const char *hexChars = "0123456789abcdef";
    auto rndHex = [&](int len) {
        QString s;
        for (int i = 0; i < len; ++i)
            s.append(hexChars[QRandomGenerator::global()->bounded(16)]);
        return s;
    };
    // UUID v4: 第三段首字符 4，第四段首字符 8-9-a-b
    QString p3 = "4" + rndHex(3);
    QString p4 = QString::number(8 + QRandomGenerator::global()->bounded(4)) + rndHex(3);
    return rndHex(8) + "-" + rndHex(4) + "-" + p3 + "-" + p4 + "-" + rndHex(12);
}

QByteArray md5Hex(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

} // namespace WeCrypto
