// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef QUEUEMODEL_H
#define QUEUEMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

class QueueModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int playListIndex READ playListIndex WRITE setPlayListIndex NOTIFY playListIndexChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SongerRole,
        SourceRole
    };
    Q_ENUM(Roles)

    explicit QueueModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void append(const QVariantMap &item);
    Q_INVOKABLE void insert(int index, const QVariantMap &item);
    Q_INVOKABLE void remove(int index, int count = 1);
    Q_INVOKABLE void move(int from, int to, int count = 1);
    Q_INVOKABLE void clear();
    Q_INVOKABLE int indexOfPath(const QString &path) const;
    Q_INVOKABLE int indexOfName(const QString &name) const;

    int count() const { return m_items.size(); }
    int playListIndex() const { return m_playListIndex; }
    void setPlayListIndex(int index);

signals:
    void countChanged();
    void playListIndexChanged();

private:
    struct Track
    {
        QString name;
        QString path;
        QString songer;
        int source = 0;
    };

    void rebuildIndex();
    Track toTrack(const QVariantMap &item) const;

    QVector<Track> m_items;
    QHash<QString, int> m_indexOfPath;
    int m_playListIndex = -1;
};

#endif
