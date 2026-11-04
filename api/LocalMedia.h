// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 本地媒体文件（TagLib）：读标签 / 音频信息 / 歌词文本，写标签与封面。
// 与在线音源完全解耦，异步接口通过信号回到 QML 线程。

#ifndef LOCALMEDIA_H
#define LOCALMEDIA_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariantMap>

class LocalMedia : public QObject
{
    Q_OBJECT

public:
    explicit LocalMedia(QObject *parent = nullptr);
    ~LocalMedia() override = default;

    // 同步接口；耗时操作请走下面的 Async 版本。
    static QVariantMap readMetadata(const QString &filePath);
    static QVariantMap readAudioInfo(const QString &filePath);
    static QString readLyricsText(const QString &filePath);
    static bool writeMetadata(const QString &filePath, const QVariantMap &meta);

    // sidecar JSON 里的封面提示（带小缓存）。
    QString coverHint(const QString &filePath);

    void readMetadataAsync(const QString &filePath);
    void writeMetadataAsync(const QString &filePath, const QVariantMap &meta);

signals:
    void metadataReady(const QString &filePath, const QVariantMap &meta);
    void metadataSaved(const QString &filePath, bool ok);

private:
    QHash<QString, QString> m_coverHints;
};

#endif // LOCALMEDIA_H
