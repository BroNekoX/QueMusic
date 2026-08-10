// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//
#include "CoverHelper.h"
#include <QTemporaryFile>
#include <QDir>

CoverHelper::CoverHelper(QObject *parent)
    : QObject(parent)
{
}

QString CoverHelper::convertVariantToUrl(const QVariant &imageVariant)
{
    if (!imageVariant.isValid() || imageVariant.isNull()) {
        m_currentCoverUrl.clear();
        emit currentCoverUrlChanged();
        return QString();
    }

    // 从 QVariant 提取 QImage（支持直接 QImage 或 QByteArray）
    QImage img;
    if (imageVariant.userType() == QMetaType::QImage) {
        img = imageVariant.value<QImage>();
    } else if (imageVariant.canConvert<QByteArray>()) {
        img.loadFromData(imageVariant.toByteArray());
    }

    if (img.isNull()) {
        m_currentCoverUrl.clear();
        emit currentCoverUrlChanged();
        return QString();
    }

    // 保存为临时 PNG 文件供 QML Image 加载
    QTemporaryFile tempFile(QDir::tempPath() + "/cover_XXXXXX.png");
    if (tempFile.open() && img.save(&tempFile, "PNG")) {
        tempFile.setAutoRemove(false); // 保留文件，避免 QML 加载时被清理
        m_currentCoverUrl = "file:///" + tempFile.fileName();
        emit currentCoverUrlChanged();
        return m_currentCoverUrl;
    }

    m_currentCoverUrl.clear();
    emit currentCoverUrlChanged();
    return QString();
}

QString CoverHelper::currentCoverUrl() const
{
    return m_currentCoverUrl;
}