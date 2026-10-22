#ifndef INDEX_H
#define INDEX_H

#include "../qcloudmusicapiglobal.h"

#include <QVariantMap>

namespace QCloudMusicApi {

    class QCLOUDMUSICAPI_EXPORT Index {
    public:
        static bool toBoolean(const QVariant& val);
        static QVariantMap cookieToJson(const QString& cookie);
        static QString cookieObjToString(const QVariantMap& cookie);
        static int getRandom(int num);
        static QString generateRandomChineseIP();
        // 生成一个随机整数
        static int getRandomInt(int min, int max);
        // 生成一个随机IP地址段
        static int generateIPSegment();
        static QVariantMap mergeMap(const QVariantMap& map0, const QVariantMap& map1);
    };

}
#endif // INDEX_H
