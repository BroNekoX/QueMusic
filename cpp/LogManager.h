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
#include <QQmlEngine>
#include <QTimer>
#include <QtQml/qqmlregistration.h>
#include <atomic>

class QJSEngine;

// 应用级日志管理器。
// - 接管 Qt 全局消息（qDebug/qInfo/qWarning/qCritical/qFatal），同时保留控制台输出。
// - 中文日志，写入“安装目录/logs/QueMusic_yyyyMMdd.log”（每日一个文件，UTF-8）。
// - 多平台：安装目录不可写时回退到用户数据目录。
// - 支持分级筛选（默认记录“信息”及以上），默认开启。
class LogManager : public QObject
{
    Q_OBJECT
    // QML 单例：QML 侧按类型名访问（如 LogManager.logPreview），
    // 这样相关绑定才能被 qmlcachegen 编译成 C++
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int minimumLevel READ minimumLevel WRITE setMinimumLevel NOTIFY minimumLevelChanged)
    Q_PROPERTY(QString logDirectory READ logDirectory CONSTANT)
    Q_PROPERTY(QString currentLogFile READ currentLogFile NOTIFY currentLogFileChanged)
    Q_PROPERTY(QString logPreview READ logPreview NOTIFY logPreviewChanged)

public:
    // QML 单例工厂。main.cpp 会提前调用一次，保证日志从启动早期就接管 Qt 消息；
    // 之后 QML 首次访问 LogManager 时复用同一实例。
    static LogManager *create(QQmlEngine *qmlEngine, QJSEngine *scriptEngine)
    {
        Q_UNUSED(scriptEngine)
        static LogManager *instance = nullptr;
        if (!instance)
            instance = new LogManager(qmlEngine);
        return instance;
    }

    // 与 Qt QtMsgType 严重程度一一对应，供 QML 直接使用
    enum Level {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Fatal = 4
    };
    Q_ENUM(Level)

    // 不能给 parent 默认值：否则引擎不会调用下面的 create()，会导致出现两个实例
    // （详见 cpp/AppModels.h 说明）
    explicit LogManager(QObject *parent);
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
    // 重建日志预览串并通知 QML。多条日志到达时会被 QTimer 合并，避免每条日志都拼接数百行。
    void refreshPreview();
    void persistSettings() const;

    static QString levelLabel(int level);
    static QString timestamp();

    static std::atomic<LogManager *> s_instance;

    bool m_enabled = true;
    int m_minimumLevel = Level::Info;  // 默认：信息
    QString m_logDir;
    QString m_logFile;
    QString m_dateStamp;
    QFile m_file;
    QStringList m_recentLines;
    QString m_preview;
    QTimer m_previewTimer;
};

#endif // LOGMANAGER_H