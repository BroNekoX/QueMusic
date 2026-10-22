#ifndef PLUGINS_H
#define PLUGINS_H

#include "qcloudmusicapiglobal.h"

#include <QVariantMap>

namespace QCloudMusicApi {
    class Request;
    class QCLOUDMUSICAPI_EXPORT Plugins : public QObject
    {
        Q_OBJECT
    public:
        explicit Plugins(Request* request, QObject* parent = nullptr);

    private:
        QVariantMap request(QString uri, QVariantMap data, QVariantMap options);
    public:
        Q_INVOKABLE QVariantMap songUpload(QVariantMap);

        Q_INVOKABLE QVariantMap upload(QVariantMap);
    signals:
    private:
        Request* m_request = nullptr;
    };

}

#endif // PLUGINS_H
