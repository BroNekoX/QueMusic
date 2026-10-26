// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#ifndef ONLINELISTMODEL_H
#define ONLINELISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

class OnlineListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    explicit OnlineListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE void append(const QVariant &items);
    Q_INVOKABLE void remove(int index, int count = 1);

    void setItems(const QVariantList &items);

signals:
    void countChanged();

private:
    void collectRoles(const QVariantMap &map);

    QVector<QVariantMap> m_items;
    QHash<int, QByteArray> m_roleNames;
    QHash<int, QString> m_roleKeys;
    QHash<QString, int> m_roleIndex;
    int m_nextRole = Qt::UserRole + 1;
};

#endif