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

#include "qwindowsintegration_p.h"
#include <qwindowsmediadevices_p.h>
#include <qwindowsformatinfo_p.h>
#include <qwindowsmediacapture_p.h>
#include <qwindowsimagecapture_p.h>
#include <qwindowscamera_p.h>
#include <qwindowsmediaencoder_p.h>
#include <mfplayercontrol_p.h>
#include <mfaudiodecodercontrol_p.h>
#include <mfevrvideowindowcontrol_p.h>
#include <private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

class QWindowsMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "windows.json")

public:
    QWindowsMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("windows"))
            return new QWindowsMediaIntegration;
        return nullptr;
    }
};

static int g_refCount = 0;

QWindowsMediaIntegration::QWindowsMediaIntegration()
{
    g_refCount++;
    if (g_refCount == 1) {
        CoInitialize(NULL);
        MFStartup(MF_VERSION);
    }
}

QWindowsMediaIntegration::~QWindowsMediaIntegration()
{
    delete m_devices;
    delete m_formatInfo;

    g_refCount--;
    if (g_refCount == 0) {
        // ### This currently crashes on exit
//        MFShutdown();
//        CoUninitialize();
    }
}

QPlatformMediaDevices *QWindowsMediaIntegration::devices()
{
    if (!m_devices)
        m_devices = new QWindowsMediaDevices(this);
    return m_devices;
}

QPlatformMediaFormatInfo *QWindowsMediaIntegration::formatInfo()
{
    if (!m_formatInfo)
        m_formatInfo = new QWindowsFormatInfo();
    return m_formatInfo;
}

QPlatformMediaCaptureSession *QWindowsMediaIntegration::createCaptureSession()
{
    return new QWindowsMediaCaptureService();
}

QPlatformAudioDecoder *QWindowsMediaIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new MFAudioDecoderControl(decoder);
}

QPlatformMediaPlayer *QWindowsMediaIntegration::createPlayer(QMediaPlayer *parent)
{
    return new MFPlayerControl(parent);
}

QPlatformCamera *QWindowsMediaIntegration::createCamera(QCamera *camera)
{
    return new QWindowsCamera(camera);
}

QPlatformMediaRecorder *QWindowsMediaIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QWindowsMediaEncoder(recorder);
}

QPlatformImageCapture *QWindowsMediaIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QWindowsImageCapture(imageCapture);
}

QPlatformVideoSink *QWindowsMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new MFEvrVideoWindowControl(sink);
}

QT_END_NAMESPACE

#include "qwindowsintegration.moc"
