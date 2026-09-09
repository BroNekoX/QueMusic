// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef SEARCHRESULTMODEL_H
#define SEARCHRESULTMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QUrl>
#include <QVariant>

// 搜索结果模型
class SearchResultModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Role {
        FileNameRole = Qt::UserRole + 1,
        FileUrlRole,
        FileSizeRole,
        FileModifiedRole,
        TitleRole,
        ArtistRole,
        CoverUrlRole,
        SongIdRole,
        FolderIdRole,
        NameRole,
        PathRole,
        SingerRole,
        DurationRole,
        TagTitleRole,
        TagArtistRole,
        TagCoverUrlRole
    };

    struct Row {
        QString name;
        QString path;
        QUrl fileUrl;
        qint64 size = 0;
        QDateTime modified;
        QString title;
        QString artist;
        QString coverUrl;
        int songId = -1;
        int folderId = -1;
        QString singer;
        int duration = 0;
        QString tagTitle;
        QString tagArtist;
        QString tagCoverUrl;
    };

    explicit SearchResultModel(const QList<int> &roles, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariant get(int index, const QString &role) const;
    Q_INVOKABLE QVariantMap getRow(int index) const;

    void appendBatch(const QList<Row> &rows);
    void clearRows();

    static QByteArray roleName(int role);

signals:
    void countChanged();

private:
    QList<Row> m_rows;
    QList<int> m_roles;
};

#endif // SEARCHRESULTMODEL_H
