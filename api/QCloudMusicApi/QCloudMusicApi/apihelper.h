#ifndef APIHELPER_H
#define APIHELPER_H

#include "module.h"

#include <QMutex>
#include <QObject>

class ApiPluginImpl;
class QCloudMusicApiPlugin;

/**
 * @class ApiHelper
 * @brief API助手类，通过反射机制调用API。
 * 继承自NeteaseCloudMusicApi，提供了调用API的高级接口。
 */
class QCLOUDMUSICAPI_EXPORT ApiHelper : public NeteaseCloudMusicApi
{
    Q_OBJECT
public:
    /**
     * @brief 封装了读取/修改方法
     * proxy(): 获取代理
     * set_proxy(proxy): 设置代理
     * 可直接通过元对象系统修改/访问属性，不要直接修改私有属性
     */
    DEFINE_VALUE_SAFETY(QString, proxy, "");

    /**
     * @brief 封装了读取/修改方法
     * realIP(): 获取realIP
     * set_realIP(realIP): 设置realIP
     * 可直接通过元对象系统修改/访问属性，不要直接修改私有属性
     */
    DEFINE_VALUE_SAFETY(QString, realIP, "");

    /**
     * @brief 封装了读取/修改方法
     * cookie(): cookie
     * set_cookie(cookie): 设置cookie
     * 可直接通过元对象系统修改/访问属性，不要直接修改私有属性
     */
    DEFINE_VALUE_SAFETY(QString, cookie, "");

    /**
     * @brief 封装了读取/修改方法
     * domain(): domain
     * set_domain(domain): 设置domain
     * 可直接通过元对象系统修改/访问属性，不要直接修改私有属性
     */
    DEFINE_VALUE_SAFETY(QString, domain, "");

public:
    explicit ApiHelper(QObject* parent = nullptr);

    /**
     * @brief 调用API的成员函数
     * @param member 成员函数名
     * @param arg 调用参数
     * @return 返回调用结果
     */
    Q_INVOKABLE QVariantMap invoke(QString member, QVariantMap arg);

    /**
     * @brief 调用API的成员函数
     * @param url 要调用的API的URL，例如："/song/url/v1?id=2058263032, 2057797340&level=exhigh", "http://localhost:3000/activate/init/profile"
     * @return 返回API调用结果
     */
    Q_INVOKABLE QVariantMap invokeUrl(QString url);

    /**
     * @brief 获取API列表
     * @return API列表
     */
    Q_INVOKABLE QStringList memberList();

    /**
     * @brief 设置全局log规则，例如："QCloudMusicApi.debug=false"
     * @param rules log规则
     */
    Q_INVOKABLE void setFilterRules(const QString& rules);

    /**
     * @brief 加载插件
     * @param fileName 插件路径
     * @return 插件ID（从1开始），失败返回-1
     */
    Q_INVOKABLE int loadPlugin(const QString& fileName);

    /**
     * @brief 加载插件
     * @param plugin 插件指针
     * @return 插件ID（从1开始），失败返回-1
     */
    Q_INVOKABLE int loadPlugin(QCloudMusicApiPlugin* plugin);

    /**
     * @brief 卸载插件
     * @param id 插件ID
     * @return 卸载结果
     */
    Q_INVOKABLE bool unloadPlugin(int id);

private:
    void beforeInvoke(QVariantMap& arg);
    void afterInvoke(QVariantMap& ret);

private:
    QStringList m_memberList;
    QList<ApiPluginImpl*> m_pluginImpls;
    QMutex m_mutex;
};

#endif // APIHELPER_H
