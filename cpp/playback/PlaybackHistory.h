// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <QtQml/qqmlregistration.h>

struct HistoryEntry {
    QString title;
    QString artist;
    QString path;
    QString cover;
    int source = -1;
    int duration = 0;
    qint64 time = 0;
};

class PlaybackHistory : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        ArtistRole,
        PathRole,
        SourceRole,
        CoverRole,
        DurationRole,
        TimeRole
    };

    explicit PlaybackHistory(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void remove(int index, int count = 1);
    Q_INVOKABLE void clear();

    void add(const HistoryEntry &entry);
    void load();
    void save();
    void setLimit(int limit)
    {
        m_limit = qMax(20, limit);
        if (m_entries.size() > m_limit)
            trim();
    }

signals:
    void countChanged();

private:
    void trim();

    QVector<HistoryEntry> m_entries;
    int m_limit = 200;
};