// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#ifndef COVERHELPER_H
#define COVERHELPER_H

#include <QObject>
#include <QImage>
#include <QVariant>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class CoverHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString currentCoverUrl READ currentCoverUrl NOTIFY currentCoverUrlChanged)

public:
    explicit CoverHelper(QObject *parent = nullptr);

    Q_INVOKABLE QString convertVariantToUrl(const QVariant &imageVariant);
    QString currentCoverUrl() const;

signals:
    void currentCoverUrlChanged();

private:
    QString m_currentCoverUrl;
};

#endif // COVERHELPER_H