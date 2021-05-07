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

#include "qgstreamermediacapture_p.h"
#include "qgstreamermediaencoder_p.h"
#include "qgstreamercameraimagecapture_p.h"
#include "qgstreamercamera_p.h"
#include <private/qgstpipeline_p.h>

#include "qgstreamercameraimagecapture_p.h"
#include "private/qgstreameraudioinput_p.h"
#include "private/qgstreameraudiooutput_p.h"
#include "private/qgstreamervideooutput_p.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcMediaCapture, "qt.multimedia.capture")

QGstreamerMediaCapture::QGstreamerMediaCapture(QMediaRecorder::CaptureMode)
    : gstPipeline("pipeline")
{
    gstAudioInput = new QGstreamerAudioInput(this);
    gstAudioInput->setPipeline(gstPipeline);
    connect(gstAudioInput, &QGstreamerAudioInput::mutedChanged, this, &QGstreamerMediaCapture::mutedChanged);
    connect(gstAudioInput, &QGstreamerAudioInput::volumeChanged, this, &QGstreamerMediaCapture::volumeChanged);

    gstAudioOutput = new QGstreamerAudioOutput(this);
    gstAudioOutput->setPipeline(gstPipeline);
    gstAudioTee = QGstElement("tee", "audiotee");

    gstPipeline.add(gstAudioInput->gstElement(), gstAudioTee, gstAudioOutput->gstElement());
    gstAudioInput->gstElement().link(gstAudioTee);
    auto pad = gstAudioTee.getRequestPad("src_%u");
    pad.link(gstAudioOutput->gstElement().staticPad("sink"));

    gstVideoOutput = new QGstreamerVideoOutput(this);
    gstVideoOutput->setIsPreview();
    gstVideoOutput->setPipeline(gstPipeline);

    gstPipeline.setState(GST_STATE_PLAYING);

    gstPipeline.dumpGraph("initial");
}

QGstreamerMediaCapture::~QGstreamerMediaCapture()
{
    setMediaEncoder(nullptr);
    setImageCapture(nullptr);
    setCamera(nullptr);
    gstPipeline.setStateSync(GST_STATE_NULL);
}

QPlatformCamera *QGstreamerMediaCapture::camera()
{
    return gstCamera;
}

void QGstreamerMediaCapture::setCamera(QPlatformCamera *camera)
{
    QGstreamerCamera *control = static_cast<QGstreamerCamera *>(camera);
    if (gstCamera == control)
        return;

    //auto state = gstPipeline.state();
    gstPipeline.setStateSync(GST_STATE_PAUSED);

    if (gstVideoTee.isNull()) {
        gstVideoTee = QGstElement("tee", "videotee");

        gstPipeline.add(gstVideoTee, gstVideoOutput->gstElement());
        auto pad = gstVideoTee.getRequestPad("src_%u");
        pad.link(gstVideoOutput->gstElement().staticPad("sink"));
    }


    if (gstCamera) {
        gstCamera->setCaptureSession(nullptr);
        auto camera = gstCamera->gstElement();
        camera.setStateSync(GST_STATE_NULL);
        gstPipeline.remove(camera);
    }

    gstCamera = control;
    if (gstCamera) {
        gstPipeline.add(gstCamera->gstElement());
        gstCamera->gstElement().link(gstVideoTee);
        gstCamera->gstElement().setStateSync(gstPipeline.state());
        gstCamera->setCaptureSession(this);
        //gstCamera->setPipeline(gstPipeline); // needed?
    }
    gstPipeline.setStateSync(GST_STATE_PLAYING);

    emit cameraChanged();
    gstPipeline.dumpGraph("camera");
}

QPlatformCameraImageCapture *QGstreamerMediaCapture::imageCapture()
{
    return m_imageCapture;
}

void QGstreamerMediaCapture::setImageCapture(QPlatformCameraImageCapture *imageCapture)
{
    QGstreamerCameraImageCapture *control = static_cast<QGstreamerCameraImageCapture *>(imageCapture);
    if (m_imageCapture == control)
        return;

    gstPipeline.setStateSync(GST_STATE_PAUSED);

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(nullptr);

    m_imageCapture = control;
    if (m_imageCapture)
        m_imageCapture->setCaptureSession(this);

    gstPipeline.setStateSync(GST_STATE_PLAYING);
    emit imageCaptureChanged();
}

void QGstreamerMediaCapture::setMediaEncoder(QPlatformMediaEncoder *encoder)
{
    QGstreamerMediaEncoder *control = static_cast<QGstreamerMediaEncoder *>(encoder);
    if (m_mediaEncoder == control)
        return;

    auto state = gstPipeline.state();
    if (state == GST_STATE_PLAYING)
        gstPipeline.setStateSync(GST_STATE_PAUSED);

    if (m_mediaEncoder)
        m_mediaEncoder->setCaptureSession(nullptr);

    m_mediaEncoder = control;
    if (m_mediaEncoder)
        m_mediaEncoder->setCaptureSession(this);
    if (state == GST_STATE_PLAYING)
        gstPipeline.setStateSync(state);

    emit encoderChanged();
    gstPipeline.dumpGraph("encoder");
}

QPlatformMediaEncoder *QGstreamerMediaCapture::mediaEncoder()
{
    return m_mediaEncoder;
}

QAudioDeviceInfo QGstreamerMediaCapture::audioInput() const
{
    return gstAudioInput->audioInput();
}

bool QGstreamerMediaCapture::setAudioInput(const QAudioDeviceInfo &info)
{
    return gstAudioInput->setAudioInput(info);
}

bool QGstreamerMediaCapture::isMuted() const
{
    return gstAudioOutput->isMuted();
}

void QGstreamerMediaCapture::setMuted(bool muted)
{
    gstAudioOutput->setMuted(muted);
}

qreal QGstreamerMediaCapture::volume() const
{
    return gstAudioOutput->volume();
}

void QGstreamerMediaCapture::setVolume(qreal volume)
{
    gstAudioOutput->setVolume(volume);
}

void QGstreamerMediaCapture::setVideoPreview(QVideoSink *sink)
{
    gstVideoOutput->setVideoSink(sink);
}

QAudioDeviceInfo QGstreamerMediaCapture::audioPreview() const
{
    return gstAudioOutput->audioOutput();
}

bool QGstreamerMediaCapture::setAudioPreview(const QAudioDeviceInfo &info)
{
    gstAudioOutput->setAudioOutput(info);
    return true;
}

QGstPad QGstreamerMediaCapture::getAudioPad() const
{
    return gstAudioTee.getRequestPad("src_%u");
}

QGstPad QGstreamerMediaCapture::getVideoPad() const
{
    return gstVideoTee.isNull() ? QGstPad() : gstVideoTee.getRequestPad("src_%u");
}

void QGstreamerMediaCapture::releaseAudioPad(const QGstPad &pad) const
{
    if (!pad.isNull())
        gstAudioTee.releaseRequestPad(pad);
}

void QGstreamerMediaCapture::releaseVideoPad(const QGstPad &pad) const
{
    if (!pad.isNull())
        gstVideoTee.releaseRequestPad(pad);
}


QT_END_NAMESPACE
