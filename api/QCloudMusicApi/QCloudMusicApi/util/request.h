#ifndef REQUEST_H
#define REQUEST_H

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QVariantMap>

#include "../qcloudmusicapiglobal.h"

class QNetworkAccessManager;
class QThread;

namespace QCloudMusicApi {

    // 请求管理类，参考原项目文件util/request.js
    class QCLOUDMUSICAPI_EXPORT Request: public QObject {
        Q_OBJECT
    public:
        explicit Request(QObject* parent = nullptr);
        ~Request();

    public:
        static QString chooseUserAgent(QString crypto, QString uaType = "pc");
        QVariantMap createRequest(
            QString uri,
            QVariantMap data,
            QVariantMap options
        );

        // 仿axios的网络请求
        QNetworkReply* axios(QNetworkAccessManager::Operation method,
            QString url,
            const QVariantMap& urlQuery,
            const QVariantMap& headers,
            const QByteArray& data,
            QNetworkProxy proxy = QNetworkProxy::DefaultProxy);
    private:
        QNetworkAccessManager* m_networkAccessManager = Q_NULLPTR;
        QThread*               m_thread               = Q_NULLPTR;
    };

}

#endif // REQUEST_H
