// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 列式 OnlineListModel：data() 是下标访问，避免 role->key->map 两次哈希。

#ifndef ONLINELISTMODEL_H
#define ONLINELISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
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
    ~OnlineListModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE void append(const QVariant &items);
    Q_INVOKABLE void remove(int index, int count = 1);

signals:
    void countChanged();

private:
    void appendItems(const QVariantList &items);
    int columnFor(const QString &name);
    void ensureColumns(const QVector<QVariantMap> &items);
    QVector<QVariant> rowValues(const QVariantMap &item) const;

    QVector<QVector<QVariant>> m_rows;
    QStringList m_columns;
    QHash<QString, int> m_columnIndex;
    QHash<int, QByteArray> m_roleNames;
};

#endif // ONLINELISTMODEL_H
