#include "apihelper.h"
#include "util/index.h"
#include "util/logger.h"
#include "qcloudmusicapiplugin.h"

#include <QMetaMethod>
#include <QPluginLoader>
#include <QThread>
#include <QUrl>
#include <QUrlQuery>

using namespace QCloudMusicApi;
ApiHelper::ApiHelper(QObject* parent)
    : NeteaseCloudMusicApi{ parent }
{
    m_memberList.clear();
    NeteaseCloudMusicApi api;
    for (int i = QObject().metaObject()->methodCount(); i < api.metaObject()->methodCount(); i++) {
        m_memberList.push_back(api.metaObject()->method(i).name());
    }
}

void ApiHelper::beforeInvoke(QVariantMap& arg)
{
    QVariantMap arg_cookie_map;
    { // 加个锁
        QMutexLocker locker(&m_mutex);
        arg_cookie_map = Index::cookieToJson(this->cookie());
    }

    // Api只能处理map类型的cookie
    if (arg.contains("cookie")) {
        // 若传入新的cookie，替换原有的cookie
        if (arg["cookie"].userType() == QMetaType::QVariantMap) {
            arg_cookie_map = arg["cookie"].toMap();
        }
        else if (arg["cookie"].userType() == QMetaType::QString) {
            arg_cookie_map = Index::cookieToJson(arg["cookie"].toString());
        }

        { // 加个锁
            QMutexLocker locker(&m_mutex);
            this->set_cookie(Index::cookieObjToString(arg_cookie_map));
        }
    }
    // 使用存储的cookie map
    arg["cookie"] = arg_cookie_map;

    // 设置全局代理
    if (!proxy().isEmpty() && !arg.contains("proxy")) {
        arg.insert("proxy", proxy());
    }

    // 设置全局realIP
    if (!realIP().isEmpty() && !arg.contains("realIP")) {
        arg.insert("realIP", realIP());
    }

    // 设置全局domain
    if (!domain().isEmpty() && !arg.contains("domain")) {
        arg.insert("domain", domain());
    }
}

void ApiHelper::afterInvoke(QVariantMap& ret)
{
    QVariantMap arg_cookie_map;
    { // 加个锁
        QMutexLocker locker(&m_mutex);
        arg_cookie_map = Index::cookieToJson(this->cookie());
    }
        auto newCookie = Index::cookieToJson(ret.value("cookie").toString());
        if (!newCookie.isEmpty()) {
            arg_cookie_map = Index::mergeMap(arg_cookie_map, newCookie);
        }
    auto token = ret.value("body").toMap()["token"].toString();
    if (!token.isEmpty()) {
        arg_cookie_map["MUSIC_A"] = token;
    }

    { // 加个锁
        QMutexLocker locker(&m_mutex);
        this->set_cookie(Index::cookieObjToString(arg_cookie_map));
    }
}

QVariantMap ApiHelper::invoke(QString member, QVariantMap arg)
{
    beforeInvoke(arg);

    QVariantMap ret;

    // 若方法重名，优先调用插件方法，尽量不要重名
    bool useNative = true;
    for (auto& pluginImpl : m_pluginImpls) {
        if (pluginImpl->plugin->memberList().contains(member)) {
            DEBUG << QString("found member %1 in: %2").arg(member).arg(pluginImpl->loader->fileName());
            ret = pluginImpl->plugin->invoke(member, arg);
            useNative = false;
            break;
        }
    }
    if (useNative) {
        QMetaObject::invokeMethod(this, member.toUtf8(),
                                  Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantMap, ret),
                                  Q_ARG(QVariantMap, arg));
    }

    afterInvoke(ret);

    return ret;
}

QVariantMap ApiHelper::invokeUrl(QString url)
{
    QUrl qurl(url);
    auto member = qurl.path();
    if (member.startsWith('/')) {
        member.remove(0, 1);
    }
    member = member.replace("/", "_").trimmed();
    QVariantMap arg;
    for (auto& queryItem : QUrlQuery(qurl.query()).queryItems()) {
        arg[queryItem.first] = queryItem.second;
    }
    DEBUG << "host" << qurl.host()
          << "port" << qurl.port()
          << "member" << member
          << "arg" << arg;
    return invoke(member, arg);
}

QStringList ApiHelper::memberList()
{
    // 原生方法 + 插件中的方法
    auto memberList = m_memberList;

    for (auto& pluginImpl : m_pluginImpls) {
        memberList.append(pluginImpl->plugin->memberList());
    }
    return memberList;
}

void ApiHelper::setFilterRules(const QString& rules)
{
    QLoggingCategory::setFilterRules(rules);
}

int ApiHelper::loadPlugin(const QString& fileName)
{
    // 加载过的插件不再重复加载，返回已有ID
    for (auto i = 0; i < m_pluginImpls.size(); i++) {
        auto pluginImpl = m_pluginImpls[i];
        if (pluginImpl->loader->fileName() == fileName) {
            return pluginImpl->id;
        }
    }

    ApiPluginImpl* pluginImpl = new ApiPluginImpl();
    QPluginLoader* loader = new QPluginLoader(this);
    loader->setFileName(fileName);
    if (loader->load())
    {
        QCloudMusicApiPlugin* plugin = qobject_cast<QCloudMusicApiPlugin*>(loader->instance());
        if (plugin) {
            pluginImpl->loader = loader;
            pluginImpl->plugin = plugin;
            pluginImpl->id = ApiPluginImpl::generateId();
            m_pluginImpls.push_back(pluginImpl);
            return pluginImpl->id;
        }
    }
    DEBUG << loader->errorString();
    loader->deleteLater();
    return -1;
}

int ApiHelper::loadPlugin(QCloudMusicApiPlugin *plugin)
{
    ApiPluginImpl* pluginImpl = new ApiPluginImpl();
    if (plugin) {
        pluginImpl->loader = new QPluginLoader(this);
        pluginImpl->plugin = plugin;
        pluginImpl->id = ApiPluginImpl::generateId();
        m_pluginImpls.push_back(pluginImpl);
        return pluginImpl->id;
    }
    return -1;
}

bool ApiHelper::unloadPlugin(int id)
{
    auto result = false;
    for (auto i = 0; i < m_pluginImpls.size(); i++) {
        auto pluginImpl = m_pluginImpls[i];
        if (pluginImpl->id == id) {
            m_pluginImpls.removeAt(i);
            pluginImpl->loader->deleteLater();
            result = pluginImpl->loader->unload();
            if (!result) {
                DEBUG << pluginImpl->loader->errorString();
            }
            delete pluginImpl;
            break;
        }
    }
    return result;
}
