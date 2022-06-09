/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qqnxmediaintegration_p.h"
#include "qqnxmediacapturesession_p.h"
#include "qqnxmediarecorder_p.h"
#include "qqnxformatinfo_p.h"
#include "qqnxvideodevices_p.h"
#include "qqnxvideosink_p.h"
#include "qqnxmediaplayer_p.h"
#include "qqnximagecapture_p.h"
#include "qqnxplatformcamera_p.h"
#include <QtMultimedia/private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

class QQnxMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "qnx.json")

public:
    QQnxMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("qnx"))
            return new QQnxMediaIntegration;
        return nullptr;
    }
};

QQnxMediaIntegration::QQnxMediaIntegration()
{
    m_videoDevices = new QQnxVideoDevices(this);
}

QQnxMediaIntegration::~QQnxMediaIntegration()
{
    delete m_formatInfo;
}

QPlatformMediaFormatInfo *QQnxMediaIntegration::formatInfo()
{
    if (!m_formatInfo)
        m_formatInfo = new QQnxFormatInfo();
    return m_formatInfo;
}

QPlatformVideoSink *QQnxMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QQnxVideoSink(sink);
}

QPlatformMediaPlayer *QQnxMediaIntegration::createPlayer(QMediaPlayer *parent)
{
    return new QQnxMediaPlayer(parent);
}

QPlatformMediaCaptureSession *QQnxMediaIntegration::createCaptureSession()
{
    return new QQnxMediaCaptureSession();
}

QPlatformMediaRecorder *QQnxMediaIntegration::createRecorder(QMediaRecorder *parent)
{
    return new QQnxMediaRecorder(parent);
}

QPlatformCamera *QQnxMediaIntegration::createCamera(QCamera *parent)
{
    return new QQnxPlatformCamera(parent);
}

QT_END_NAMESPACE

#include "qqnxmediaintegration.moc"
