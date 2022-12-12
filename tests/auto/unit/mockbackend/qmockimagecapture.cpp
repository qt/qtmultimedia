// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmockimagecapture.h>
#include <qmockcamera.h>
#include <qmockmediacapturesession.h>
#include <qimagecapture.h>
#include <qcamera.h>

QT_BEGIN_NAMESPACE

QMockImageCapture::QMockImageCapture(QImageCapture *parent)
    : QPlatformImageCapture(parent)
{
}

bool QMockImageCapture::isReadyForCapture() const
{
    return m_ready;
}

int QMockImageCapture::capture(const QString &fileName)
{
    if (isReadyForCapture()) {
        m_fileName = fileName;
        m_captureRequest++;
        emit readyForCaptureChanged(m_ready = false);
        QTimer::singleShot(5, this, SLOT(captured()));
        return m_captureRequest;
    } else {
        emit error(-1, QImageCapture::NotReadyError,
                   QLatin1String("Could not capture in stopped state"));
    }

    return -1;
}

void QMockImageCapture::captured()
{
    emit imageCaptured(m_captureRequest, QImage());

    QMediaMetaData metaData;
    metaData.insert(QMediaMetaData::Author, QString::fromUtf8("Author"));
    metaData.insert(QMediaMetaData::Date, QDateTime(QDate(2021, 1, 1), QTime()));

    emit imageMetadataAvailable(m_captureRequest, metaData);

    if (!m_ready)
    {
        emit readyForCaptureChanged(m_ready = true);
        emit imageExposed(m_captureRequest);
    }

    emit imageSaved(m_captureRequest, m_fileName);
}

QT_END_NAMESPACE

#include "moc_qmockimagecapture.cpp"
