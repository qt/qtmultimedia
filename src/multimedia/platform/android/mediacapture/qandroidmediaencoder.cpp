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

#include "qandroidmediaencoder_p.h"
#include "qandroidmultimediautils_p.h"
#include "qandroidcapturesession_p.h"
#include "qandroidmediacapturesession_p.h"

QT_BEGIN_NAMESPACE

QAndroidMediaEncoder::QAndroidMediaEncoder(QMediaRecorder *parent)
    : QPlatformMediaRecorder(parent)
{
}

bool QAndroidMediaEncoder::isLocationWritable(const QUrl &location) const
{
    return location.isValid()
            && (location.isLocalFile() || location.isRelative());
}

QMediaRecorder::RecorderState QAndroidMediaEncoder::state() const
{
    return m_session ? m_session->state() : QMediaRecorder::StoppedState;
}

qint64 QAndroidMediaEncoder::duration() const
{
    return m_session ? m_session->duration() : 0;

}

void QAndroidMediaEncoder::record(QMediaEncoderSettings &settings)
{
    if (m_session)
        m_session->start(settings, outputLocation());
}

void QAndroidMediaEncoder::stop()
{
    if (m_session)
        m_session->stop();
}

void QAndroidMediaEncoder::setOutputLocation(const QUrl &location)
{
    if (location.isLocalFile()) {
        qt_androidRequestWriteStoragePermission();
    }
    QPlatformMediaRecorder::setOutputLocation(location);
}

void QAndroidMediaEncoder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QAndroidMediaCaptureSession *captureSession = static_cast<QAndroidMediaCaptureSession *>(session);
    if (m_service == captureSession)
        return;

    if (m_service)
        stop();
    if (m_session)
        m_session->setMediaEncoder(nullptr);

    m_service = captureSession;
    if (!m_service)
        return;
    m_session = m_service->captureSession();
    Q_ASSERT(m_session);
    m_session->setMediaEncoder(this);
}

QT_END_NAMESPACE
