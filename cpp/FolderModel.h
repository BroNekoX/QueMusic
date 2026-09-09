// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include "CoverHelper.h"
#include "SearchResultModel.h"

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QThread>
#include <QVector>
#include <QDateTime>
#include <atomic>

class QTimer;

struct FolderItem {
    int id = -1;
    QString name;
    QString type;
    QString path;
    QDateTime createdAt;
};

struct SongItem {
    int id = -1;
    int folderId = -1;
    QString name;
    QString path;
    QString singer;
    int duration = 0;
    // 后台富集的 TAG 信息（不落库）
    QString tagTitle;
    QString tagArtist;
    QString tagCoverUrl;
};

// 工作线程逐文件解析 TAG（title/artist/内嵌封面），批量回传
struct SongEnrichResult {
    int row = -1;
    QString title;
    QString artist;
    QString coverUrl;
};
Q_DECLARE_METATYPE(SongEnrichResult)
Q_DECLARE_METATYPE(QList<SongEnrichResult>)

class SongEnrichWorker : public QObject {
    Q_OBJECT
public:
    struct Task {
        int row;
        QString path;
    };
    QList<Task> tasks;
    QString cacheDir;
    std::atomic<bool> cancel{false};
    std::atomic<quint64> generation{0};

public slots:
    void run();

signals:
    void enriched(quint64 gen, QList<SongEnrichResult> results);
    void enrichDone(quint64 gen, int count);
};

class FolderModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterType READ filterType WRITE setFilterType NOTIFY filterTypeChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        PathRole,
        CreatedAtRole
    };

    explicit FolderModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadFromDatabase();
    Q_INVOKABLE int addFolder(const QString &name, const QString &type, const QString &path = "");
    Q_INVOKABLE bool deleteFolder(int folderId);
    Q_INVOKABLE bool renameFolder(int folderId, const QString &newName);

    QString filterType() const { return m_filterType; }
    void setFilterType(const QString &type);

signals:
    void filterTypeChanged();
    void errorOccurred(const QString &message);

private:
    void refreshModel();
    QSqlDatabase m_db;
    QVector<FolderItem> m_items;
    QString m_filterType = "my";
};

class SongModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int folderId READ folderId WRITE setFolderId NOTIFY folderIdChanged)
    Q_PROPERTY(bool searchActive READ searchActive NOTIFY searchActiveChanged)
    Q_PROPERTY(QAbstractListModel *searchResults READ searchResults CONSTANT)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        FolderIdRole,
        NameRole,
        PathRole,
        SingerRole,
        DurationRole,
        TagTitleRole,
        TagArtistRole,
        TagCoverUrlRole
    };

    explicit SongModel(QObject *parent = nullptr);
    ~SongModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void loadByFolder(int folderId);
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE int addSong(int folderId, const QString &name, const QString &path, const QString &singer = "");
    Q_INVOKABLE int addSongs(int folderId, const QVariantList &songs);
    Q_INVOKABLE bool deleteSong(int songId);

    // 分块遍历内存行，命中行流式追加进 searchResults
    Q_INVOKABLE void startSearch(const QString &text);
    Q_INVOKABLE void clearSearch();
    bool searchActive() const { return m_searchActive; }
    QAbstractListModel *searchResults() const { return m_searchResults; }

    int folderId() const { return m_folderId; }
    void setFolderId(int folderId);

signals:
    void folderIdChanged();
    void errorOccurred(const QString &message);
    void metadataReady(int count);
    void searchActiveChanged();
    void searchFinished(int count);

private slots:
    void onEnriched(quint64 gen, QList<SongEnrichResult> results);
    void onEnrichDone(quint64 gen, int count);
    void searchStep();

private:
    void refreshModel();
    void startEnrichment();
    QSqlDatabase m_db;
    QVector<SongItem> m_items;
    int m_folderId = -1;
    QThread m_enrichThread;
    SongEnrichWorker *m_enrichWorker = nullptr;
    std::atomic<quint64> m_enrichGen{0};
    SearchResultModel *m_searchResults = nullptr;
    QTimer *m_searchTimer = nullptr;
    QString m_searchText;
    int m_searchPos = 0;
    bool m_searchActive = false;
};

#endif // FOLDERMODEL_H