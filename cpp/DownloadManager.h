// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>
#include <QList>
#include <QString>

struct DownloadTask {
    enum Status { Queued, Downloading, Completed, Error };
    int id = 0;
    QString url;
    QString fileName;
    QString filePath;
    qreal progress = 0.0;
    Status status = Queued;
    QString errorString;
};

class DownloadManager : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int currentTaskId READ currentTaskId NOTIFY currentTaskIdChanged)
    Q_PROPERTY(bool hasActiveTasks READ hasActiveTasks NOTIFY hasActiveTasksChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY completedCountChanged)
    Q_PROPERTY(int taskCount READ taskCount NOTIFY taskCountChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        UrlRole,
        FileNameRole,
        FilePathRole,
        ProgressRole,
        StatusRole,
        ErrorStringRole,
        TitleRole,
        ArtistRole,
        CoverRole,
        DurationRole,
        HashRole
    };

    explicit DownloadManager(QObject *parent = nullptr);
    ~DownloadManager() override;

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Public API
    Q_INVOKABLE void addDownload(const QString &url, const QString &fileName);
    Q_INVOKABLE void retryTask(int taskId);
    Q_INVOKABLE void removeTask(int taskId);
    Q_INVOKABLE void clearCompleted();
    Q_INVOKABLE void cancelCurrent();

    int currentTaskId() const { return m_currentTaskId; }
    bool hasActiveTasks() const { return m_currentTaskId >= 0; }
    int completedCount() const;
    int taskCount() const { return m_tasks.size(); }

signals:
    void currentTaskIdChanged();
    void hasActiveTasksChanged();
    void completedCountChanged();
    void taskCountChanged();

private slots:
    void onReadyRead();
    void onFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onErrorOccurred(QNetworkReply::NetworkError code);

private:
    void startNextTask();
    void abortCurrentDownload();
    int findTaskById(int id) const;
    int nextTaskId();

    QNetworkAccessManager *m_manager;
    QNetworkReply *m_reply = nullptr;
    QFile *m_file = nullptr;

    QList<DownloadTask> m_tasks;
    int m_currentTaskId = -1;
    int m_nextId = 1;
};

#endif // DOWNLOADMANAGER_H