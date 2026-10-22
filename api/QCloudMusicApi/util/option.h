#ifndef OPTION_H
#define OPTION_H

#include "../qcloudmusicapiglobal.h"

#include <QObject>
#include <QVariantMap>

namespace QCloudMusicApi {

class QCLOUDMUSICAPI_EXPORT Option
{
public:
    static QVariantMap createOption(QVariantMap query, QString crypto = "");
};

}

#endif // OPTION_H
