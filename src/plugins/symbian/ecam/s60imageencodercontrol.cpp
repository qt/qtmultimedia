/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qstring.h>

#include "s60imageencodercontrol.h"
#include "s60imagecapturesession.h"

S60ImageEncoderControl::S60ImageEncoderControl(QObject *parent) :
    QImageEncoderControl(parent)
{
}

S60ImageEncoderControl::S60ImageEncoderControl(S60ImageCaptureSession *session, QObject *parent) :
    QImageEncoderControl(parent)
{
    m_session = session;
}

S60ImageEncoderControl::~S60ImageEncoderControl()
{
}

QList<QSize> S60ImageEncoderControl::supportedResolutions(
        const QImageEncoderSettings &settings, bool *continuous) const
{
    QList<QSize> resolutions = m_session->supportedCaptureSizesForCodec(settings.codec());

    // Discrete resolutions are returned
    if (continuous)
        *continuous = false;

    return resolutions;
}
QStringList S60ImageEncoderControl::supportedImageCodecs() const
{
    return m_session->supportedImageCaptureCodecs();
}

QString S60ImageEncoderControl::imageCodecDescription(const QString &codec) const
{
    return m_session->imageCaptureCodecDescription(codec);
}

QImageEncoderSettings S60ImageEncoderControl::imageSettings() const
{
    // Update setting values from session
    QImageEncoderSettings settings;
    settings.setCodec(m_session->imageCaptureCodec());
    settings.setResolution(m_session->captureSize());
    settings.setQuality(m_session->captureQuality());

    return settings;
}
void S60ImageEncoderControl::setImageSettings(const QImageEncoderSettings &settings)
{
    // Notify that settings have been implicitly set and there's no need to
    // initialize them in case camera is changed
    m_session->notifySettingsSet();

    if (!settings.isNull()) {
        if (!settings.codec().isEmpty()) {
            if (settings.resolution() != QSize(-1,-1)) { // Codec, Resolution & Quality
                m_session->setImageCaptureCodec(settings.codec());
                m_session->setCaptureSize(settings.resolution());
                m_session->setCaptureQuality(settings.quality());
            } else { // Codec and Quality
                m_session->setImageCaptureCodec(settings.codec());
                m_session->setCaptureQuality(settings.quality());
            }
        } else {
            if (settings.resolution() != QSize(-1,-1)) { // Resolution & Quality
                m_session->setCaptureSize(settings.resolution());
                m_session->setCaptureQuality(settings.quality());
            }
            else // Only Quality
                m_session->setCaptureQuality(settings.quality());
        }

        // Prepare ImageCapture with the settings and set error if needed
        int prepareSuccess = m_session->prepareImageCapture();

        // Preparation fails with KErrNotReady if camera has not been started.
        // That can be ignored since settings are set internally in that case.
        if (prepareSuccess != KErrNotReady && prepareSuccess != KErrNone)
            m_session->setError(prepareSuccess, tr("Failure in preparation of image capture."));
    }
}

// End of file
