#include "aplugin.h"

#include <QMetaMethod>

APlugin::APlugin(QObject *parent)
{
    for(int i = QObject().metaObject()->methodCount(); i < api.metaObject()->methodCount(); i++) {
        m_memberList.push_back(api.metaObject()->method(i).name());
    }
}

QVariantMap APlugin::invoke(QString member, QVariantMap arg)
{
    QVariantMap ret;
    QMetaObject::invokeMethod(&api, member.toUtf8(),
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariantMap, ret),
                              Q_ARG(QVariantMap, arg));
    return ret;
}

QStringList APlugin::memberList()
{
    return m_memberList;
}
