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
#include "qffmpegmediaintegration_p.h"
#include "qffmpegmediadevices_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegmediaplayer_p.h"
#include "qffmpegvideosink_p.h"

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
    qDebug() << "QFFMpegMediaIntegration constructor";
    m_devices = new QFFmpegMediaDevices(this);
    m_formatsInfo = new QFFmpegMediaFormatInfo();
}

QFFmpegMediaIntegration::~QFFmpegMediaIntegration()
{
    delete m_devices;
    delete m_formatsInfo;
}

QPlatformMediaDevices *QFFmpegMediaIntegration::devices()
{
    return m_devices;
}

QPlatformMediaFormatInfo *QFFmpegMediaIntegration::formatInfo()
{
    return m_formatsInfo;
}

QPlatformAudioDecoder *QFFmpegMediaIntegration::createAudioDecoder(QAudioDecoder */*decoder*/)
{
    return nullptr;//new QFFmpegAudioDecoder(decoder);
}

QPlatformMediaCaptureSession *QFFmpegMediaIntegration::createCaptureSession()
{
    return nullptr; //new QFFmpegMediaCapture();
}

QPlatformMediaPlayer *QFFmpegMediaIntegration::createPlayer(QMediaPlayer *player)
{
    return new QFFmpegMediaPlayer(player);
}

QPlatformCamera *QFFmpegMediaIntegration::createCamera(QCamera */*camera*/)
{
    return nullptr;//new QFFmpegCamera(camera);
}

QPlatformMediaRecorder *QFFmpegMediaIntegration::createRecorder(QMediaRecorder */*recorder*/)
{
    return nullptr;//new QFFmpegMediaEncoder(recorder);
}

QPlatformImageCapture *QFFmpegMediaIntegration::createImageCapture(QImageCapture */*imageCapture*/)
{
    return nullptr;//new QFFmpegImageCapture(imageCapture);
}

QPlatformVideoSink *QFFmpegMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QFFmpegVideoSink(sink);
}

QT_END_NAMESPACE

#include "qffmpegmediaintegration.moc"
