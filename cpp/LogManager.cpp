// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "LogManager.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QSettings>

#include <cstdio>
#include <cstdlib>

std::atomic<LogManager *> LogManager::s_instance{ nullptr };

LogManager::LogManager(QObject *parent)
    : QObject(parent)
{
    // 1) 先注册全局实例指针（消息处理器可能在任何时刻被调用）
    s_instance.store(this);

    // 2) 确定日志目录：优先“安装目录/logs”；不可写时回退到用户数据目录（多平台健壮性）
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/logs");
    m_logDir = QDir(baseDir).absolutePath();

    // 3) 读取配置（与 QML Settings 共享同一 ini：configDir + /BroNekoX/QueMusic.ini）
    {
        const QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
        QSettings settings(configPath + QStringLiteral("/BroNekoX/QueMusic.ini"), QSettings::IniFormat);
        m_enabled = settings.value(QStringLiteral("Options/logEnabled"), true).toBool();
        m_minimumLevel = settings.value(QStringLiteral("Options/logLevel"), int(Level::Error)).toInt();
        m_minimumLevel = qBound(int(Level::Debug), m_minimumLevel, int(Level::Fatal));
    }

    // 4) 立即创建/打开今天的日志文件并写入会话横幅
    ensureLogFile();

    // 5) 接管 Qt 全局消息（qDebug/qInfo/qWarning/qCritical/qFatal 都汇聚到这里）
    qInstallMessageHandler(&LogManager::messageHandler);

    info(QStringLiteral("日志系统已启动"), QStringLiteral("系统"));
}

LogManager::~LogManager()
{
    qInstallMessageHandler(nullptr);
    s_instance.store(nullptr);
    if (m_file.isOpen())
        m_file.close();
}

bool LogManager::enabled() const
{
    return m_enabled;
}

void LogManager::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;
    m_enabled = enabled;
    persistSettings();
    emit enabledChanged();
    if (enabled)
        info(QStringLiteral("日志已启用"), QStringLiteral("系统"));
}

int LogManager::minimumLevel() const
{
    return m_minimumLevel;
}

void LogManager::setMinimumLevel(int level)
{
    level = qBound(int(Level::Debug), level, int(Level::Fatal));
    if (m_minimumLevel == level)
        return;
    m_minimumLevel = level;
    persistSettings();
    emit minimumLevelChanged();
    info(QStringLiteral("记录等级已切换为: %1").arg(levelLabel(level)), QStringLiteral("系统"));
}

QString LogManager::logDirectory() const
{
    return m_logDir;
}

QString LogManager::currentLogFile() const
{
    return m_logFile;
}

QString LogManager::logPreview() const
{
    return m_preview;
}

void LogManager::debug(const QString &message, const QString &category)
{
    writeLine(Level::Debug, category, message);
}

void LogManager::info(const QString &message, const QString &category)
{
    writeLine(Level::Info, category, message);
}

void LogManager::warning(const QString &message, const QString &category)
{
    writeLine(Level::Warning, category, message);
}

void LogManager::error(const QString &message, const QString &category)
{
    writeLine(Level::Error, category, message);
}

void LogManager::fatal(const QString &message, const QString &category)
{
    writeLine(Level::Fatal, category, message);
}

void LogManager::openLogFolder()
{
    QDir().mkpath(m_logDir);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_logDir));
}

void LogManager::writeLine(int level, const QString &category, const QString &message)
{
    // 致命错误始终记录；其余级别遵从“启用开关 + 最低等级”筛选
    if (!m_enabled && level != Level::Fatal)
        return;
    if (level < m_minimumLevel)
        return;

    ensureLogFile();
    if (!m_file.isOpen())
        return;

    const QString line = QStringLiteral("[%1] [%2]%3 %4")
                             .arg(timestamp(),
                                  levelLabel(level),
                                  category.isEmpty()
                                      ? QString()
                                      : QStringLiteral(" [%1]").arg(category),
                                  message);

    m_file.write(line.toUtf8());
    m_file.write("\n");
    m_file.flush();

    m_recentLines.append(line);
    constexpr int kMaxPreviewLines = 500;
    while (m_recentLines.size() > kMaxPreviewLines)
        m_recentLines.removeFirst();
    m_preview = m_recentLines.join(QLatin1Char('\n'));
    emit logPreviewChanged();
}

void LogManager::ensureLogFile()
{
    const QString today = QDate::currentDate().toString(QStringLiteral("yyyyMMdd"));
    if (m_file.isOpen() && m_dateStamp == today)
        return;

    if (m_file.isOpen())
        m_file.close();

    QDir().mkpath(m_logDir);
    const QString path = m_logDir + QStringLiteral("/QueMusic_") + today + QStringLiteral(".log");
    m_file.setFileName(path);

    if (m_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_dateStamp = today;
        m_logFile = QFileInfo(path).absoluteFilePath();

        // 新文件写入 UTF-8 BOM，便于 Windows 记事本识别中文
        if (m_file.size() == 0)
            m_file.write("\xEF\xBB\xBF");

        const QString banner =
            QStringLiteral("[%1] [信息] [系统] ===== 日志会话开始（记录等级: %2） =====")
                .arg(timestamp(), levelLabel(m_minimumLevel));
        m_file.write(banner.toUtf8());
        m_file.write("\n");
        m_file.flush();

        if (m_recentLines.isEmpty()) {
            m_recentLines.append(banner);
            m_preview = m_recentLines.join(QLatin1Char('\n'));
            emit logPreviewChanged();
        }
    }
}

void LogManager::persistSettings() const
{
    const QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QSettings settings(configPath + QStringLiteral("/BroNekoX/QueMusic.ini"), QSettings::IniFormat);
    settings.setValue(QStringLiteral("Options/logEnabled"), m_enabled);
    settings.setValue(QStringLiteral("Options/logLevel"), m_minimumLevel);
}

QString LogManager::levelLabel(int level)
{
    switch (level) {
    case Level::Debug:
        return QStringLiteral("调试");
    case Level::Info:
        return QStringLiteral("信息");
    case Level::Warning:
        return QStringLiteral("警告");
    case Level::Error:
        return QStringLiteral("错误");
    case Level::Fatal:
        return QStringLiteral("致命");
    }
    return QStringLiteral("未知");
}

QString LogManager::timestamp()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
}

void LogManager::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    LogManager *self = s_instance.load();

    // 保留控制台输出（stderr），命令行调试时日志依旧可见
    const QByteArray localMsg = msg.toLocal8Bit();
    const char *category = context.category ? context.category : "default";
    switch (type) {
    case QtDebugMsg:
        std::fprintf(stderr, "[调试] %s: %s\n", category, localMsg.constData());
        break;
    case QtInfoMsg:
        std::fprintf(stderr, "[信息] %s: %s\n", category, localMsg.constData());
        break;
    case QtWarningMsg:
        std::fprintf(stderr, "[警告] %s: %s\n", category, localMsg.constData());
        break;
    case QtCriticalMsg:
        std::fprintf(stderr, "[错误] %s: %s\n", category, localMsg.constData());
        break;
    case QtFatalMsg:
        std::fprintf(stderr, "[致命] %s: %s\n", category, localMsg.constData());
        break;
    }

    if (!self)
        return;

    int level = Level::Debug;
    switch (type) {
    case QtInfoMsg:
        level = Level::Info;
        break;
    case QtWarningMsg:
        level = Level::Warning;
        break;
    case QtCriticalMsg:
        level = Level::Error;
        break;
    case QtFatalMsg:
        level = Level::Fatal;
        break;
    default:
        level = Level::Debug;
        break;
    }

    const QString cat = QString::fromUtf8(context.category ? context.category : "");

    // 文件写入统一在主线程执行，避免跨线程访问 QFile / QML 预览的竞态
    if (QThread::currentThread() == self->thread()) {
        self->writeLine(level, cat, msg);
    } else {
        QMetaObject::invokeMethod(self, [self, level, cat, msg]() {
            self->writeLine(level, cat, msg);
        }, Qt::QueuedConnection);
    }

    if (type == QtFatalMsg)
        std::abort();
}