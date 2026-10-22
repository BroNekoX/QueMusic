#ifndef LOGGER_H
#define LOGGER_H

#include <QLoggingCategory>

#define LOGGER_NAME ApiLogger

namespace QCloudMusicApi {

    Q_DECLARE_LOGGING_CATEGORY(LOGGER_NAME)

}

#define DEBUG qCDebug(LOGGER_NAME)
#define INFO qCInfo(LOGGER_NAME)
#define WARNING qCWarning(LOGGER_NAME)

#endif // LOGGER_H
