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
#include "qgstreamerimagecapture_p.h"
#include "qgstreamercamera_p.h"
#include <private/qgstpipeline_p.h>

#include "private/qgstreameraudioinput_p.h"
#include "private/qgstreameraudiooutput_p.h"
#include "private/qgstreamervideooutput_p.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcMediaCapture, "qt.multimedia.capture")

QGstreamerMediaCapture::QGstreamerMediaCapture()
    : gstPipeline("pipeline")
{
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

    gstPipeline.beginConfig();

    if (gstCamera) {
        gstCamera->setCaptureSession(nullptr);
        auto camera = gstCamera->gstElement();
        camera.setStateSync(GST_STATE_NULL);
        gstVideoTee.setStateSync(GST_STATE_NULL);
        gstVideoOutput->gstElement().setStateSync(GST_STATE_NULL);

        gstPipeline.remove(camera);
        gstPipeline.remove(gstVideoTee);
        gstPipeline.remove(gstVideoOutput->gstElement());

        gstVideoTee = {};
    }

    gstCamera = control;
    if (gstCamera) {
        QGstElement camera = gstCamera->gstElement();
        gstVideoTee = QGstElement("tee", "videotee");

        gstPipeline.add(camera, gstVideoTee, gstVideoOutput->gstElement());
        auto pad = gstVideoTee.getRequestPad("src_%u");
        pad.link(gstVideoOutput->gstElement().staticPad("sink"));
        gstCamera->gstElement().link(gstVideoTee);
        gstCamera->setCaptureSession(this);

        camera.setState(GST_STATE_PAUSED);
    }

    gstPipeline.endConfig();

    emit cameraChanged();
    gstPipeline.dumpGraph("camera");
}

QPlatformImageCapture *QGstreamerMediaCapture::imageCapture()
{
    return m_imageCapture;
}

void QGstreamerMediaCapture::setImageCapture(QPlatformImageCapture *imageCapture)
{
    QGstreamerImageCapture *control = static_cast<QGstreamerImageCapture *>(imageCapture);
    if (m_imageCapture == control)
        return;

    gstPipeline.beginConfig();

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(nullptr);

    m_imageCapture = control;
    if (m_imageCapture)
        m_imageCapture->setCaptureSession(this);

    gstPipeline.endConfig();
    emit imageCaptureChanged();
}

void QGstreamerMediaCapture::setMediaEncoder(QPlatformMediaEncoder *encoder)
{
    QGstreamerMediaEncoder *control = static_cast<QGstreamerMediaEncoder *>(encoder);
    if (m_mediaEncoder == control)
        return;

    gstPipeline.setStateSync(GST_STATE_PAUSED);

    if (m_mediaEncoder)
        m_mediaEncoder->setCaptureSession(nullptr);
    m_mediaEncoder = control;
    if (m_mediaEncoder)
        m_mediaEncoder->setCaptureSession(this);

    gstPipeline.setState(GST_STATE_PLAYING);

    emit encoderChanged();
    gstPipeline.dumpGraph("encoder");
}

QPlatformMediaEncoder *QGstreamerMediaCapture::mediaEncoder()
{
    return m_mediaEncoder;
}

void QGstreamerMediaCapture::setAudioInput(QPlatformAudioInput *input)
{
    if (gstAudioInput == input)
        return;

    gstPipeline.beginConfig();
    if (gstAudioInput) {
        gstAudioInput->gstElement().setStateSync(GST_STATE_NULL);
        gstPipeline.remove(gstAudioInput->gstElement());
        gstAudioInput->setPipeline({});
        gstAudioTee.setStateSync(GST_STATE_NULL);
        if (gstAudioOutput) {
            gstAudioOutputPad.unlinkPeer();
            gstAudioTee.releaseRequestPad(gstAudioOutputPad);
            gstAudioOutputPad = {};
            gstAudioOutput->gstElement().setStateSync(GST_STATE_NULL);
            gstPipeline.remove(gstAudioOutput->gstElement());
        }
        gstPipeline.remove(gstAudioTee);
        gstAudioTee = {};
    }

    gstAudioInput = static_cast<QGstreamerAudioInput *>(input);
    if (gstAudioInput) {
        gstAudioInput->setPipeline(gstPipeline);
        Q_ASSERT(gstAudioTee.isNull());
        gstAudioTee = QGstElement("tee", "audiotee");
        gstAudioTee.set("allow-not-linked", true);
        gstPipeline.add(gstAudioInput->gstElement(), gstAudioTee);
        gstAudioInput->gstElement().link(gstAudioTee);

        if (gstAudioOutput) {
            gstPipeline.add(gstAudioOutput->gstElement());
            gstAudioOutputPad = gstAudioTee.getRequestPad("src_%u");
            gstAudioOutputPad.link(gstAudioOutput->gstElement().staticPad("sink"));
        }
    }

    gstPipeline.endConfig();
}

void QGstreamerMediaCapture::setVideoPreview(QVideoSink *sink)
{
    gstVideoOutput->setVideoSink(sink);
}

void QGstreamerMediaCapture::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;
    gstPipeline.beginConfig();

    if (gstAudioOutput) {
        gstAudioOutput->gstElement().setStateSync(GST_STATE_NULL);
        if (!gstAudioTee.isNull()) {
            gstAudioOutputPad.unlinkPeer();
            gstAudioTee.releaseRequestPad(gstAudioOutputPad);
            gstAudioOutputPad = {};
            gstPipeline.remove(gstAudioOutput->gstElement());
        }
        gstAudioOutput->setPipeline({});
    }

    gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
    if (gstAudioOutput) {
        gstAudioOutput->setPipeline(gstPipeline);
        if (!gstAudioTee.isNull()) {
            gstPipeline.add(gstAudioOutput->gstElement());
            gstAudioOutputPad = gstAudioTee.getRequestPad("src_%u");
            gstAudioOutputPad.link(gstAudioOutput->gstElement().staticPad("sink"));
        }
    }

    gstPipeline.endConfig();
}


QGstPad QGstreamerMediaCapture::getAudioPad() const
{
    return gstAudioTee.isNull() ? QGstPad() : gstAudioTee.getRequestPad("src_%u");
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

QGstreamerVideoSink *QGstreamerMediaCapture::gstreamerVideoSink() const
{
    return gstVideoOutput ? gstVideoOutput->gstreamerVideoSink() : nullptr;
}


QT_END_NAMESPACE
