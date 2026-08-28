// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QDebug>
#include <atomic>

// 应用级日志管理器。
// - 接管 Qt 全局消息（qDebug/qInfo/qWarning/qCritical/qFatal），同时保留控制台输出。
// - 中文日志，写入“安装目录/logs/QueMusic_yyyyMMdd.log”（每日一个文件，UTF-8）。
// - 多平台：安装目录不可写时回退到用户数据目录。
// - 支持分级筛选（默认只记录“错误/致命”），默认开启。
class LogManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int minimumLevel READ minimumLevel WRITE setMinimumLevel NOTIFY minimumLevelChanged)
    Q_PROPERTY(QString logDirectory READ logDirectory CONSTANT)
    Q_PROPERTY(QString currentLogFile READ currentLogFile NOTIFY currentLogFileChanged)
    Q_PROPERTY(QString logPreview READ logPreview NOTIFY logPreviewChanged)

public:
    // 与 Qt QtMsgType 严重程度一一对应，供 QML 直接使用
    enum Level {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4
    };
    Q_ENUM(Level)

    explicit LogManager(QObject *parent = nullptr);
    ~LogManager() override;

    bool enabled() const;
    void setEnabled(bool enabled);

    int minimumLevel() const;
    void setMinimumLevel(int level);

    QString logDirectory() const;
    QString currentLogFile() const;
    QString logPreview() const;

    Q_INVOKABLE void debug(const QString &message, const QString &category = QString());
    Q_INVOKABLE void info(const QString &message, const QString &category = QString());
    Q_INVOKABLE void warning(const QString &message, const QString &category = QString());
    Q_INVOKABLE void error(const QString &message, const QString &category = QString());
    Q_INVOKABLE void fatal(const QString &message, const QString &category = QString());
    Q_INVOKABLE void openLogFolder();

signals:
    void enabledChanged();
    void minimumLevelChanged();
    void currentLogFileChanged();
    void logPreviewChanged();

private:
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    void writeLine(int level, const QString &category, const QString &message);
    void ensureLogFile();
    void persistSettings() const;

    static QString levelLabel(int level);
    static QString timestamp();

    static std::atomic<LogManager *> s_instance;

    bool m_enabled = true;
    int m_minimumLevel = Level::Error;   // 默认：错误（问题）
    QString m_logDir;
    QString m_logFile;
    QString m_dateStamp;
    QFile m_file;
    QStringList m_recentLines;
    QString m_preview;
};

#endif // LOGMANAGER_H