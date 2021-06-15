/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qmockimagecapture.h>
#include <qmockcamera.h>
#include <qmockmediacapturesession.h>
#include <qimagecapture.h>
#include <qcamera.h>

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
