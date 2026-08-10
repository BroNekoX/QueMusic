// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 模拟 QML ListModel 的动态列表模型：count/get/clear/append/remove
// roleNames 随 append 动态生成，委托里任意字段可直接访问。
#ifndef ONLINELISTMODEL_H
#define ONLINELISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QVariantList>
#include <QVariantMap>

class OnlineListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    explicit OnlineListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // QML ListModel 兼容 API
    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void clear();
    Q_INVOKABLE void append(const QVariant &items); // 单个 map 或 map 数组
    Q_INVOKABLE void remove(int index, int count = 1);

    // 批量替换（清空 + 填充）
    void setItems(const QVariantList &items);

signals:
    void countChanged();

private:
    void collectRoles(const QVariantMap &map); // 为新字段分配 role

    QList<QVariantMap> m_items;
    QHash<int, QByteArray> m_roleNames; // role id → 字段名
    QHash<QByteArray, int> m_roleIndex; // 字段名 → role id
    int m_nextRole = Qt::UserRole + 1;
};

#endif // ONLINELISTMODEL_H
