// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2024-2026 QueMusic Contributors
//
#include "CoverHelper.h"
#include <QTemporaryFile>
#include <QDir>
#include <QDebug>

CoverHelper::CoverHelper(QObject *parent)
    : QObject(parent)
    , m_currentCoverUrl("")
{
}

QString CoverHelper::convertVariantToUrl(const QVariant &imageVariant)
{
    qDebug() << "convertVariantToUrl called, type:" << imageVariant.typeName();

    if (!imageVariant.isValid() || imageVariant.isNull()) {
        m_currentCoverUrl = "";
        emit currentCoverUrlChanged();
        return "";
    }

    QImage img;

    // 转换获取QImage
    if (imageVariant.userType() == QMetaType::QImage) {
        img = imageVariant.value<QImage>();
        qDebug() << "Got QImage directly, size:" << img.size();
    }
    // byteArrray加载
    else if (imageVariant.canConvert<QByteArray>()) {
        QByteArray data = imageVariant.toByteArray();
        img.loadFromData(data);
        qDebug() << "Loaded from QByteArray, size:" << img.size();
    }

    if (img.isNull()) {
        qDebug() << "Failed to extract image from variant";
        m_currentCoverUrl = "";
        emit currentCoverUrlChanged();
        return "";
    }

    // 保存为临时文件
    QTemporaryFile tempFile(QDir::tempPath() + "/cover_XXXXXX.png");
    if (tempFile.open() && img.save(&tempFile, "PNG")) {
        tempFile.setAutoRemove(false);  // 防删除缓存，使用通用目录格式
        m_currentCoverUrl = "file:///" + tempFile.fileName();
        qDebug() << "Cover saved to:" << m_currentCoverUrl;
        emit currentCoverUrlChanged();
        return m_currentCoverUrl;
    }

    m_currentCoverUrl = "";
    emit currentCoverUrlChanged();
    return "";
}

QString CoverHelper::currentCoverUrl() const
{
    return m_currentCoverUrl;
}