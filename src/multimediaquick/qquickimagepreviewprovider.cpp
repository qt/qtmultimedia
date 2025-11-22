// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickimagepreviewprovider_p.h"
#include <QtCore/qmutex.h>
#include <QtCore/qdebug.h>

#include <mutex>

QT_BEGIN_NAMESPACE

namespace {

struct QQuickImagePreviewProviderPrivate
{
    void clear()
    {
        std::lock_guard guard(mutex);
        records.clear();
    }

    void registerImage(QUuid instance, QString id, QImage preview)
    {
        // we only keep the most recent preview for each instances
        std::lock_guard guard(mutex);
        records.insert_or_assign(instance, Record{ std::move(id), std::move(preview) });
    }

    QImage getImage(const QString &id, QSize *size, const QSize &requestedSize)
    {
        QImage preview = [&] {
            std::lock_guard guard(mutex);
            for (const auto &record : records) {
                if (record.second.id == id)
                    return record.second.image;
            }
            return QImage();
        }();

        if (preview.isNull())
            return QImage();

        if (requestedSize.isValid())
            preview = preview.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        if (size)
            *size = preview.size();

        return preview;
    }

    void cleanupInstance(QUuid instance)
    {
        std::lock_guard guard(mutex);
        records.erase(instance);
    }

    struct Record
    {
        QString id;
        QImage image;
    };

    QMutex mutex;
    std::map<QUuid, Record> records;
};

Q_GLOBAL_STATIC(QQuickImagePreviewProviderPrivate, previewProviderSingleton)

} // namespace

QQuickImagePreviewProvider::QQuickImagePreviewProvider()
: QQuickImageProvider(QQuickImageProvider::Image)
{
}

QQuickImagePreviewProvider::~QQuickImagePreviewProvider()
{
    previewProviderSingleton()->clear();
}

QImage QQuickImagePreviewProvider::requestImage(const QString &id, QSize *size, const QSize& requestedSize)
{
    return previewProviderSingleton()->getImage(id, size, requestedSize);
}

void QQuickImagePreviewProvider::registerPreview(QUuid captureInstance, QString id, QImage preview)
{
    previewProviderSingleton()->registerImage(captureInstance, std::move(id), std::move(preview));
}

void QQuickImagePreviewProvider::cleanupInstance(QUuid captureInstance)
{
    previewProviderSingleton()->cleanupInstance(captureInstance);
}

QT_END_NAMESPACE
