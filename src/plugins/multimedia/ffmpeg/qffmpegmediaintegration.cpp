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

#include <QtMultimedia/private/qplatformmediaplugin_p.h>
#include <qcameradevice.h>
#include "qffmpegmediaintegration_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegmediaplayer_p.h"
#include "qffmpegvideosink_p.h"
#include "qffmpegmediacapturesession_p.h"
#include "qffmpegmediarecorder_p.h"
#include "qffmpegimagecapture_p.h"
#include "qffmpegaudioinput_p.h"
#include "qffmpegaudiodecoder_p.h"

#ifdef Q_OS_MACOS
#include <VideoToolbox/VideoToolbox.h>
#endif

#ifdef Q_OS_DARWIN
#include "qavfcamera_p.h"
#elif defined(Q_OS_WINDOWS)
#include "qwindowscamera_p.h"
#endif

#if QT_CONFIG(linux_v4l)
#include "qv4l2camera_p.h"
#endif

QT_BEGIN_NAMESPACE

class QFFmpegMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "ffmpeg.json")

public:
    QFFmpegMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("ffmpeg"))
            return new QFFmpegMediaIntegration;
        return nullptr;
    }
};

QFFmpegMediaIntegration::QFFmpegMediaIntegration()
{
    m_formatsInfo = new QFFmpegMediaFormatInfo();

#if QT_CONFIG(linux_v4l)
    m_videoDevices = new QV4L2CameraDevices(this);
#endif
#ifdef Q_OS_DARWIN
    m_videoDevices = new QAVFVideoDevices(this);
#endif

#ifndef QT_NO_DEBUG
    qDebug() << "Available HW decoding frameworks:";
    AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
    while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
        qDebug() << "    " << av_hwdevice_get_type_name(type);
#endif
}

QFFmpegMediaIntegration::~QFFmpegMediaIntegration()
{
    delete m_formatsInfo;
}

QPlatformMediaFormatInfo *QFFmpegMediaIntegration::formatInfo()
{
    return m_formatsInfo;
}

QPlatformAudioDecoder *QFFmpegMediaIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QFFmpegAudioDecoder(decoder);
}

QPlatformMediaCaptureSession *QFFmpegMediaIntegration::createCaptureSession()
{
    return new QFFmpegMediaCaptureSession();
}

QPlatformMediaPlayer *QFFmpegMediaIntegration::createPlayer(QMediaPlayer *player)
{
    return new QFFmpegMediaPlayer(player);
}

QPlatformCamera *QFFmpegMediaIntegration::createCamera(QCamera *camera)
{
#ifdef Q_OS_DARWIN
    return new QAVFCamera(camera);
#elif QT_CONFIG(linux_v4l)
    return new QV4L2Camera(camera);
#elif defined(Q_OS_WINDOWS)
    return new QWindowsCamera(camera);
#else
    Q_UNUSED(camera);
    return nullptr;//new QFFmpegCamera(camera);
#endif
}

QPlatformMediaRecorder *QFFmpegMediaIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QFFmpegMediaRecorder(recorder);
}

QPlatformImageCapture *QFFmpegMediaIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QFFmpegImageCapture(imageCapture);
}

QPlatformVideoSink *QFFmpegMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QFFmpegVideoSink(sink);
}

QPlatformAudioInput *QFFmpegMediaIntegration::createAudioInput(QAudioInput *input)
{
    return new QFFmpegAudioInput(input);
}

QT_END_NAMESPACE

#include "qffmpegmediaintegration.moc"
