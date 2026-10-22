#include "option.h"

using namespace QCloudMusicApi;
QVariantMap Option::createOption(QVariantMap query, QString crypto)
{
    QVariantMap result;
    auto func = [&](QString key) { if (query.contains(key)) result[key] = query[key]; };
    for (auto i: { "crypto", "cookie", "ua", "proxy", "realIP", "e_r", "domain" }) func(i);
    return result;
}
