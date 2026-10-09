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

    // worker 线程执行：匹配最近一次扫描结果，分批回传命中行下标
    void search(const QString &needle, quint64 searchGeneration);

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
    void searchBatch(quint64 searchGeneration, const QList<int> &rows);
    void searchDone(quint64 searchGeneration, int total);

private:
    // 搜索索引，只在 worker 线程访问；下标与 UI 侧 m_entries 对应
    QList<LocalFileEntry> m_index;
};

class LocalMusicScanner : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged FINAL)
    Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters NOTIFY nameFiltersChanged FINAL)
    Q_PROPERTY(bool showDirs READ showDirs WRITE setShowDirs NOTIFY showDirsChanged FINAL)
    Q_PROPERTY(int sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged FINAL)
    Q_PROPERTY(bool sortReversed READ sortReversed WRITE setSortReversed NOTIFY sortReversedChanged FINAL)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged FINAL)
    Q_PROPERTY(bool deleting READ deleting NOTIFY deletingChanged FINAL)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)
    Q_PROPERTY(bool searchActive READ searchActive NOTIFY searchActiveChanged FINAL)
    Q_PROPERTY(SearchResultModel *searchResults READ searchResults CONSTANT FINAL)
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

    // 资源管理器式搜索：匹配在扫描线程执行，命中行分批追加进 searchResults
    Q_INVOKABLE void startSearch(const QString &text);
    Q_INVOKABLE void clearSearch();

    // 重新扫描当前目录（刷新按钮用）。直接复用 startScan()，
    // 避免 QML 侧用 folder="" → folder=原值 去"骗"一次重扫：
    // 那会让 setFolder("") 真的触发 clearEntries() 把列表先清空一次，
    // 紧接着再扫一遍，等于连续两次模型重置。
    Q_INVOKABLE void rescan();
    bool searchActive() const { return m_searchActive; }
    SearchResultModel *searchResults() const { return m_searchResults; }

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
    void onWorkerSearchBatch(quint64 searchGeneration, const QList<int> &rows);
    void onWorkerSearchDone(quint64 searchGeneration, int total);

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
    std::atomic<quint64> m_searchGeneration{0};
    SearchResultModel *m_searchResults = nullptr;
    QString m_searchText;
    bool m_searchActive = false;
};
