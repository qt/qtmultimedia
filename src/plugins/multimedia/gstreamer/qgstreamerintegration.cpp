/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qgstreamerintegration_p.h"
#include "qgstreamervideodevices_p.h"
#include "qgstreamermediaplayer_p.h"
#include "qgstreamermediacapture_p.h"
#include "qgstreameraudiodecoder_p.h"
#include "qgstreamercamera_p.h"
#include "qgstreamermediaencoder_p.h"
#include "qgstreamerimagecapture_p.h"
#include "qgstreamerformatinfo_p.h"
#include "qgstreamervideosink_p.h"
#include "qgstreameraudioinput_p.h"
#include "qgstreameraudiooutput_p.h"
#include <QtMultimedia/private/qplatformmediaplugin_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QGstreamerMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "gstreamer.json")

public:
    QGstreamerMediaPlugin()
        : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("gstreamer"))
            return new QGstreamerIntegration;
        return nullptr;
    }
};

QGstreamerIntegration::QGstreamerIntegration()
{
    gst_init(nullptr, nullptr);
    m_videoDevices = new QGstreamerVideoDevices(this);
    m_formatsInfo = new QGstreamerFormatInfo();
}

QGstreamerIntegration::~QGstreamerIntegration()
{
    delete m_formatsInfo;
}

QPlatformMediaFormatInfo *QGstreamerIntegration::formatInfo()
{
    return m_formatsInfo;
}

const QGstreamerFormatInfo *QGstreamerIntegration::gstFormatsInfo() const
{
    return m_formatsInfo;
}

QPlatformAudioDecoder *QGstreamerIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QGstreamerAudioDecoder(decoder);
}

QPlatformMediaCaptureSession *QGstreamerIntegration::createCaptureSession()
{
    return new QGstreamerMediaCapture();
}

QPlatformMediaPlayer *QGstreamerIntegration::createPlayer(QMediaPlayer *player)
{
    return new QGstreamerMediaPlayer(player);
}

QPlatformCamera *QGstreamerIntegration::createCamera(QCamera *camera)
{
    return new QGstreamerCamera(camera);
}

QPlatformMediaRecorder *QGstreamerIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QGstreamerMediaEncoder(recorder);
}

QPlatformImageCapture *QGstreamerIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QGstreamerImageCapture(imageCapture);
}

QPlatformVideoSink *QGstreamerIntegration::createVideoSink(QVideoSink *sink)
{
    return new QGstreamerVideoSink(sink);
}

QPlatformAudioInput *QGstreamerIntegration::createAudioInput(QAudioInput *q)
{
    return new QGstreamerAudioInput(q);
}

QPlatformAudioOutput *QGstreamerIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QGstreamerAudioOutput(q);
}

GstDevice *QGstreamerIntegration::videoDevice(const QByteArray &id) const
{
    return m_videoDevices ? static_cast<QGstreamerVideoDevices*>(m_videoDevices)->videoDevice(id) : nullptr;
}

QT_END_NAMESPACE

#include "qgstreamerintegration.moc"
