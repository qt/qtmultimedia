/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qffmpegimagecapture_p.h"
#include <private/qplatformcamera_p.h>
#include <private/qplatformimagecapture_p.h>
#include <qvideoframeformat.h>
#include <private/qmediastoragelocation_p.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <qstandardpaths.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcImageCapture, "qt.multimedia.imageCapture")

QFFmpegImageCapture::QFFmpegImageCapture(QImageCapture *parent)
  : QPlatformImageCapture(parent)
{
}

QFFmpegImageCapture::~QFFmpegImageCapture()
{
}

bool QFFmpegImageCapture::isReadyForCapture() const
{
    return m_session && !passImage && cameraActive;
}

int QFFmpegImageCapture::capture(const QString &fileName)
{
    QString path = QMediaStorageLocation::generateFileName(fileName, QStandardPaths::PicturesLocation, QLatin1String("jpg"));
    return doCapture(path);
}

int QFFmpegImageCapture::captureToBuffer()
{
    return doCapture(QString());
}

int QFFmpegImageCapture::doCapture(const QString &fileName)
{
    qCDebug(qLcImageCapture) << "do capture";
    if (!m_session) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::ResourceError),
                                  Q_ARG(QString, QPlatformImageCapture::msgImageCaptureNotSet()));

        qCDebug(qLcImageCapture) << "error 1";
        return -1;
    }
    if (!m_session->camera()) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::ResourceError),
                                  Q_ARG(QString,tr("No camera available.")));

        qCDebug(qLcImageCapture) << "error 2";
        return -1;
    }
    if (passImage) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::NotReadyError),
                                  Q_ARG(QString, QPlatformImageCapture::msgCameraNotReady()));

        qCDebug(qLcImageCapture) << "error 3";
        return -1;
    }
    m_lastId++;

    pendingImages.enqueue({m_lastId, fileName, QMediaMetaData{}});
    // let one image pass the pipeline
    passImage = true;

    // #### trigger actual frame capture

    emit readyForCaptureChanged(false);
    return m_lastId;
}

void QFFmpegImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    auto *captureSession = static_cast<QFFmpegMediaCaptureSession *>(session);
    if (m_session == captureSession)
        return;

    bool readyForCapture = isReadyForCapture();
    if (m_session) {
        disconnect(m_session, nullptr, this, nullptr);
        m_lastId = 0;
        pendingImages.clear();
        passImage = false;
        cameraActive = false;
    }

    m_session = captureSession;
    if (!m_session) {
        if (readyForCapture)
            emit readyForCaptureChanged(false);
        return;
    }

    connect(m_session, &QPlatformMediaCaptureSession::cameraChanged, this, &QFFmpegImageCapture::onCameraChanged);
    onCameraChanged();
}

void QFFmpegImageCapture::cameraActiveChanged(bool active)
{
    qCDebug(qLcImageCapture) << "cameraActiveChanged" << cameraActive << active;
    if (cameraActive == active)
        return;
    cameraActive = active;
    qCDebug(qLcImageCapture) << "isReady" << isReadyForCapture();
    emit readyForCaptureChanged(isReadyForCapture());
}

void QFFmpegImageCapture::onCameraChanged()
{
    if (m_session->camera()) {
        cameraActiveChanged(m_session->camera()->isActive());
        connect(m_session->camera(), &QPlatformCamera::activeChanged, this, &QFFmpegImageCapture::cameraActiveChanged);
    } else {
        cameraActiveChanged(false);
    }
}

QImageEncoderSettings QFFmpegImageCapture::imageSettings() const
{
    return m_settings;
}

void QFFmpegImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    if (m_settings != settings) {
        m_settings = settings;
        // ###
    }
}

QT_END_NAMESPACE
