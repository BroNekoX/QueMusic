// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef DOWNLOADEDMUSICMODEL_H
#define DOWNLOADEDMUSICMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QVariant>
#include <QtQmlIntegration/qqmlintegration.h>

// 已下载歌曲项：元数据来自与音频同名的 .json
struct DownloadedItem {
    QString title;
    QString artist;
    QString cover;
    int duration = 0;
    QString hash;
    QString fileName;
    QString fileUrl;
    QVariantList lyrics;
    QVariantList translate;
};

class DownloadedMusicModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString downloadDir READ downloadDir WRITE setDownloadDir NOTIFY downloadDirChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        ArtistRole,
        CoverRole,
        DurationRole,
        HashRole,
        FileNameRole,
        FileUrlRole,
        LyricsRole,
        TranslateRole,
        PaytypeRole        // 本地文件恒免费：只给角色名，data() 返回空 → QML 取 0
    };

    explicit DownloadedMusicModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reload();
    Q_INVOKABLE QVariantMap get(int row) const;

    QString downloadDir() const { return m_downloadDir; }
    void setDownloadDir(const QString &dir);

signals:
    void downloadDirChanged();
    void countChanged();

private:
    void applyItems(const QVector<DownloadedItem> &items);

    QString m_downloadDir;
    QVector<DownloadedItem> m_items;
    // 丢弃过期的扫描结果
    quint64 m_generation = 0;
};

#endif // DOWNLOADEDMUSICMODEL_H
