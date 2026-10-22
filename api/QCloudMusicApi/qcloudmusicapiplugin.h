#ifndef QCLOUDMUSICAPIPLUGIN_H
#define QCLOUDMUSICAPIPLUGIN_H

#include <QGenericPlugin>
#include <QPluginLoader>

class QCloudMusicApiPlugin
{
public:
    virtual QVariantMap invoke(QString member, QVariantMap arg) = 0;

    virtual QStringList memberList() = 0;
};

QT_BEGIN_NAMESPACE

#define QCloudMusicApiPlugin_iid "org.QCloudMusicApi.ApiHelperPlugin"
Q_DECLARE_INTERFACE(QCloudMusicApiPlugin, QCloudMusicApiPlugin_iid)

QT_END_NAMESPACE

class ApiPluginImpl
{
public:
    int id = -1;
    QPluginLoader* loader = nullptr;
    QCloudMusicApiPlugin* plugin = nullptr;

    static int generateId() {
        static int nextId = 1;
        return nextId++;
    }
};

#endif // QCLOUDMUSICAPIPLUGIN_H
