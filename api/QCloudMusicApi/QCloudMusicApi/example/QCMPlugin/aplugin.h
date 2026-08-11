#ifndef APLUGIN_H
#define APLUGIN_H

#include <QGenericPlugin>

#include "../../qcloudmusicapiplugin.h"

#include "api.h"

class APlugin : public QObject, public QCloudMusicApiPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QCloudMusicApiPlugin_iid)
    Q_INTERFACES(QCloudMusicApiPlugin)

public:
    explicit APlugin(QObject *parent = nullptr);

    virtual QVariantMap invoke(QString member, QVariantMap arg) override;

    virtual QStringList memberList() override;

private:
    Api api;
    QStringList m_memberList;
};

#endif // APLUGIN_H
