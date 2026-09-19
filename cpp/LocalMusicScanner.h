// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QUrl>
#include <QtQmlIntegration/qqmlintegration.h>
#include <atomic>

#include "SearchResultModel.h"

class QTimer;

struct LocalFileEntry {
    QString name;
    QString path;
    qint64 size = 0;
    QDateTime modified;
    QString title;
    QString artist;
    QString coverUrl;
};
Q_DECLARE_METATYPE(LocalFileEntry)
Q_DECLARE_METATYPE(QList<LocalFileEntry>)

class LocalScanWorker : public QObject {
    Q_OBJECT
public:
    QString folder;
    QStringList nameFilters;
    bool showDirs = false;
    int sortField = 0;
    bool sortReversed = false;
    QString cacheDir;
    std::atomic<bool> cancel{false};
    std::atomic<quint64> generation{0};

public slots:
    void run();
    // 批量移入回收站（与扫描同线程，串行执行）
    void deleteFiles(const QStringList &paths);

signals:
    void progress(quint64 gen, int count);
    void finished(quint64 gen, QList<LocalFileEntry> entries);
    void enriched(quint64 gen, int startIndex, QList<LocalFileEntry> chunk);
    void enrichDone(quint64 gen, int count);
    void failed(quint64 gen, const QString &message);
    void deleteProgress(quint64 gen, int processed, int total);
    void deleteFinished(quint64 gen, const QStringList &removed, const QStringList &failed);
};

class LocalMusicScanner : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged)
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters NOTIFY nameFiltersChanged)
    Q_PROPERTY(bool showDirs READ showDirs WRITE setShowDirs NOTIFY showDirsChanged)
    Q_PROPERTY(int sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(bool sortReversed READ sortReversed WRITE setSortReversed NOTIFY sortReversedChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(bool deleting READ deleting NOTIFY deletingChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool searchActive READ searchActive NOTIFY searchActiveChanged)
    Q_PROPERTY(QAbstractListModel *searchResults READ searchResults CONSTANT)
public:
    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        FileUrlRole,
        FileSizeRole,
        FileModifiedRole,
        TitleRole,
        ArtistRole,
        CoverUrlRole
    };

    explicit LocalMusicScanner(QObject *parent = nullptr);
    ~LocalMusicScanner() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariant get(int index, const QString &role) const;

    // 批量移入回收站：工作线程执行 + 上报进度；完成后只摘掉对应行，不重扫目录
    Q_INVOKABLE void deleteFiles(const QVariantList &paths);

    // 资源管理器式搜索：分块遍历已扫描条目，命中行流式追加进 searchResults
    Q_INVOKABLE void startSearch(const QString &text);
    Q_INVOKABLE void clearSearch();
    bool searchActive() const { return m_searchActive; }
    QAbstractListModel *searchResults() const { return m_searchResults; }

    QUrl folder() const { return m_folder; }
    void setFolder(const QUrl &folder);
    QStringList nameFilters() const { return m_nameFilters; }
    void setNameFilters(const QStringList &filters);
    bool showDirs() const { return m_showDirs; }
    void setShowDirs(bool v);
    int sortField() const { return m_sortField; }
    void setSortField(int v);
    bool sortReversed() const { return m_sortReversed; }
    void setSortReversed(bool v);
    bool scanning() const { return m_scanning; }
    bool deleting() const { return m_deleting; }

signals:
    void folderChanged();
    void nameFiltersChanged();
    void showDirsChanged();
    void sortFieldChanged();
    void sortReversedChanged();
    void scanningChanged();
    void countChanged();
    void scanProgress(int count);
    void scanFinished(int count);
    void metadataReady(int count);
    void searchActiveChanged();
    void searchFinished(int count);
    void deletingChanged();
    void deleteProgress(int processed, int total);
    void deleteFinished(int removed, int failedCount);

private slots:
    void onWorkerProgress(quint64 gen, int count);
    void onWorkerFinished(quint64 gen, QList<LocalFileEntry> entries);
    void onWorkerEnriched(quint64 gen, int startIndex, QList<LocalFileEntry> chunk);
    void onWorkerEnrichDone(quint64 gen, int count);
    void onWorkerFailed(quint64 gen, const QString &message);
    void onWorkerDeleteProgress(quint64 gen, int processed, int total);
    void onWorkerDeleteFinished(quint64 gen, const QStringList &removed, const QStringList &failed);
    void searchStep();

private:
    void clearEntries();
    void startScan();
    void removeEntriesByPath(const QStringList &paths);

    QUrl m_folder;
    QStringList m_nameFilters;
    bool m_showDirs = false;
    int m_sortField = 0;
    bool m_sortReversed = false;
    bool m_scanning = false;
    bool m_deleting = false;
    QList<LocalFileEntry> m_entries;
    QThread m_thread;
    LocalScanWorker *m_worker = nullptr;
    std::atomic<quint64> m_generation{0};
    SearchResultModel *m_searchResults = nullptr;
    QTimer *m_searchTimer = nullptr;
    QString m_searchText;
    int m_searchPos = 0;
    bool m_searchActive = false;
};
