// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "WebDavStore.h"
#include "WebDavClient.h"

#include <QSettings>
#include <QUuid>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace {

#ifdef Q_OS_WIN

// Windows DPAPI：绑当前用户，密文只能在同用户下解开
QString encryptText(const QString &plain)
{
    if (plain.isEmpty())
        return {};
    QByteArray raw = plain.toUtf8();
    DATA_BLOB src{ DWORD(raw.size()), reinterpret_cast<BYTE *>(raw.data()) };
    DATA_BLOB out{ 0, nullptr };
    if (!CryptProtectData(&src, L"QueMusic", nullptr, nullptr, nullptr, 0, &out))
        return {};
    const QByteArray cipher(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
    LocalFree(out.pbData);
    return QString::fromLatin1(cipher.toBase64());
}

QString decryptText(const QString &cipher)
{
    if (cipher.isEmpty())
        return {};
    const QByteArray raw = QByteArray::fromBase64(cipher.toLatin1());
    DATA_BLOB src{ DWORD(raw.size()),
                   reinterpret_cast<BYTE *>(const_cast<char *>(raw.constData())) };
    DATA_BLOB out{ 0, nullptr };
    if (!CryptUnprotectData(&src, nullptr, nullptr, nullptr, nullptr, 0, &out))
        return {};
    const QString plain = QString::fromUtf8(reinterpret_cast<const char *>(out.pbData),
                                            int(out.cbData));
    LocalFree(out.pbData);
    return plain;
}

#else

// 其它平台暂无系统凭据库（Keychain 后续接），这里只是编码，避免明文直读
QString encryptText(const QString &plain)
{
    return plain.isEmpty() ? QString() : QString::fromLatin1(plain.toUtf8().toBase64());
}

QString decryptText(const QString &cipher)
{
    return cipher.isEmpty() ? QString()
                            : QString::fromUtf8(QByteArray::fromBase64(cipher.toLatin1()));
}

#endif

QString field(const QVariantMap &server, const char *key)
{
    return server.value(QLatin1String(key)).toString();
}

// 统一成 "scheme://host/path/"：缺 scheme 补 https，末尾补 '/'（PROPFIND 对目录更稳）
QString normalizeUrl(const QString &url)
{
    QString out = url.trimmed();
    if (out.isEmpty())
        return {};
    if (!out.contains(QStringLiteral("://")))
        out.prepend(QStringLiteral("https://"));
    if (!out.endsWith(QLatin1Char('/')))
        out.append(QLatin1Char('/'));
    return out;
}

} // namespace

WebDavStore::WebDavStore(QObject *parent)
    : QObject(parent)
{
    load();
}

QVariantList WebDavStore::servers() const
{
    QVariantList out = m_servers;
    for (QVariant &item : out) {
        QVariantMap server = item.toMap();
        server.remove(QStringLiteral("password"));   // 凭据不进 QML
        item = server;
    }
    return out;
}

void WebDavStore::load()
{
    QSettings settings;
    m_servers = settings.value(QStringLiteral("webdav/servers")).toList();
}

void WebDavStore::save() const
{
    m_authCacheId.clear();                          // 配置变了，缓存的鉴权头作废
    QSettings settings;
    settings.setValue(QStringLiteral("webdav/servers"), m_servers);
}

int WebDavStore::indexOf(const QString &id) const
{
    for (int i = 0; i < m_servers.size(); ++i) {
        if (field(m_servers.at(i).toMap(), "id") == id)
            return i;
    }
    return -1;
}

void WebDavStore::addServer(const QString &name, const QString &url, const QString &user,
                            const QString &password)
{
    const QString root = normalizeUrl(url);
    if (root.isEmpty())
        return;
    for (const QVariant &item : m_servers) {
        const QVariantMap server = item.toMap();
        if (field(server, "url") == root) {          // 同地址不重复添加
            updateServer(field(server, "id"), name, root, user, password);
            return;
        }
    }
    QVariantMap server;
    server.insert(QStringLiteral("id"), QUuid::createUuid().toString(QUuid::WithoutBraces));
    server.insert(QStringLiteral("name"), name.trimmed());
    server.insert(QStringLiteral("url"), root);
    server.insert(QStringLiteral("user"), user);
    server.insert(QStringLiteral("password"), encryptText(password));
    m_servers.append(server);
    save();
    emit serversChanged();
}

void WebDavStore::updateServer(const QString &id, const QString &name, const QString &url,
                               const QString &user, const QString &password)
{
    const int index = indexOf(id);
    const QString root = normalizeUrl(url);
    if (index < 0 || root.isEmpty())
        return;
    QVariantMap server = m_servers.at(index).toMap();
    server.insert(QStringLiteral("name"), name.trimmed());
    server.insert(QStringLiteral("url"), root);
    server.insert(QStringLiteral("user"), user);
    if (!password.isEmpty())
        server.insert(QStringLiteral("password"), encryptText(password));
    m_servers[index] = server;
    save();
    emit serversChanged();
}

void WebDavStore::removeServer(const QString &id)
{
    const int index = indexOf(id);
    if (index < 0)
        return;
    m_servers.removeAt(index);
    save();
    emit serversChanged();
}

QString WebDavStore::authHeader(const QString &id) const
{
    if (!id.isEmpty() && m_authCacheId == id)
        return m_authCacheHeader;                       // 每请求都要用，避免反复 DPAPI 解密
    const int index = indexOf(id);
    if (index < 0)
        return {};
    const QVariantMap server = m_servers.at(index).toMap();
    const QString header = WebDavClient::basicAuth(field(server, "user"),
                                                   decryptText(field(server, "password")));
    m_authCacheId = id;
    m_authCacheHeader = header;
    return header;
}

QString WebDavStore::authHeaderOfUrl(const QString &url) const
{
    for (const QVariant &item : m_servers) {
        const QVariantMap server = item.toMap();
        const QString root = field(server, "url");
        if (!root.isEmpty() && url.startsWith(root))
            return authHeader(field(server, "id"));
    }
    return {};
}

void WebDavStore::rememberSidecars(const QString &audioUrl, const QString &lyricsUrl,
                                   const QString &coverUrl)
{
    if (audioUrl.isEmpty())
        return;
    QVariantMap sidecars;
    sidecars.insert(QStringLiteral("lyricsUrl"), lyricsUrl);
    sidecars.insert(QStringLiteral("coverUrl"), coverUrl);
    m_sidecars.insert(audioUrl, sidecars);
}

QVariantMap WebDavStore::sidecarsOf(const QString &audioUrl) const
{
    return m_sidecars.value(audioUrl);
}

QString WebDavStore::userOf(const QString &id) const
{
    const int index = indexOf(id);
    return index < 0 ? QString() : field(m_servers.at(index).toMap(), "user");
}
