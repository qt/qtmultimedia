// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamerinterface.h"

#include <QtMultimedia/private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QGStreamerInterface
    \since 6.12

    \brief Provides GStreamer-specific integration points for Qt Multimedia.

    \inmodule QtMultimedia
    \ingroup multimedia

    \warning The QGStreamerInterface class offers limited compatibility guarantees.
    There are no source or binary compatibility guarantees for this API, meaning it
    is only guaranteed to work with the Qt version the application has been developed
    against. Incompatible changes are aimed to be kept at a minimum and will only be
    made in minor releases.

    To use this class in an application, link to \c Qt::MultimediaPrivate (if using
    CMake), and include the header
    \c{#include <QtMultimedia/spi/qgstreamerinterface.h>}.

    QGStreamerInterface is the entry point for customizing the GStreamer backend:
    wrapping custom pipeline elements as Qt audio and video devices, accessing the
    underlying \c GstPipeline of high-level Qt objects, and converting between
    \l QVideoFrame and GStreamer buffers.

    By default, the class is available in meta-qt6 builds. In custom builds
    with GStreamer support, the \c gstreamer_qt_api feature can enable it.

    QtMultimedia must be run with the \c gstreamer media backend to get a valid
    instance of the class.
*/


/*!
    Returns the GStreamer interface for the active Qt Multimedia backend, or
    \c nullptr if the GStreamer backend is not in use.
*/
QGStreamerInterface *QGStreamerInterface::instance()
{
    return QPlatformMediaIntegration::instance()->gstreamerInterface();
}

/*!
    \fn QGStreamerInterface::~QGStreamerInterface()

    Destroys the interface.
*/

/*!
    \fn QAudioDevice QGStreamerInterface::makeCustomGStreamerAudioInput(const QByteArray &gstreamerPipeline)

    Creates a custom audio input device from a GStreamer pipeline description.

    The \a gstreamerPipeline string is parsed as a GStreamer bin description. The
    resulting elements are connected as the audio source of a \l QMediaCaptureSession
    when the returned \l QAudioDevice is passed to \l QAudioInput and assigned with
    \l{QMediaCaptureSession::setAudioInput()}.

    Element names in the description can be used to locate nodes in the session
    pipeline via \c gst_bin_get_by_name() after the capture session is active.

    \code
    #include <QtMultimedia/spi/qgstreamerinterface.h>

    QMediaCaptureSession session;
    QAudioInput input{
        QGStreamerInterface::instance()->makeCustomGStreamerAudioInput(
            "audiotestsrc wave=2 freq=200 ! identity name=myConverter") };
    session.setAudioInput(&input);
    \endcode

    \sa makeCustomGStreamerAudioOutput(), QAudioInput, QMediaCaptureSession
*/

/*!
    \fn QAudioDevice QGStreamerInterface::makeCustomGStreamerAudioOutput(const QByteArray &gstreamerPipeline)

    Creates a custom audio output device from a GStreamer pipeline description.

    The \a gstreamerPipeline string is parsed as a GStreamer bin description. The
    resulting elements are connected as the audio sink of a \l QMediaCaptureSession
    when the returned \l QAudioDevice is passed to \l QAudioOutput and assigned with
    \l{QMediaCaptureSession::setAudioOutput()}.

    \sa makeCustomGStreamerAudioInput(), QAudioOutput, QMediaCaptureSession
*/

/*!
    \fn GstPipeline *QGStreamerInterface::gstPipeline(QMediaPlayer *player)

    Returns the underlying GStreamer pipeline for \a player, or \c nullptr if
    \a player is not backed by the GStreamer media backend.

    \warning The pipeline is still owned and driven by Qt Multimedia. Modifying its
    state or topology without coordinating with \l QMediaPlayer can cause undefined
    behavior.

    \sa QMediaPlayer
*/

/*!
    \fn GstPipeline *QGStreamerInterface::gstPipeline(QMediaCaptureSession *session)

    Returns the underlying GStreamer pipeline for \a session, or \c nullptr if
    \a session is not backed by the GStreamer media backend.

    \warning The pipeline is still owned and driven by Qt Multimedia. Modifying its
    state or topology without coordinating with the capture session can cause undefined
    behavior.

    \sa QMediaCaptureSession
*/

/*!
    \fn GstBuffer *QGStreamerInterface::gstBuffer(const QVideoFrame &frame)

    Returns the GStreamer buffer backing \a frame, or \c nullptr if \a frame does not
    hold GStreamer-backed video memory.

    Ownership of the returned buffer is \e not transferred.

    \sa createFrameFromGstBuffer()
*/

/*!
    \fn QVideoFrame QGStreamerInterface::createFrameFromGstBuffer(GstBuffer *buffer, const GstVideoInfo &videoInfo)

    Creates a \l QVideoFrame that wraps \a buffer using the format described by
    \a videoInfo.

    The created frame shares ownership of the specified \c GstBuffer with the caller.

    \sa createFrameFromGstBuffer(GstBuffer *, const GstVideoInfoDmaDrm &), gstBuffer()
*/

/*!
    \fn QVideoFrame QGStreamerInterface::createFrameFromGstBuffer(GstBuffer *buffer, const GstVideoInfoDmaDrm &videoInfo)

    Creates a \l QVideoFrame that wraps a DMA-BUF-backed \a buffer using the format
    described by \a videoInfo.

    Requires GStreamer 1.24 or later with DMA-BUF video format support. If unsupported,
    returns an invalid \l QVideoFrame and logs a warning.

    The created frame shares ownership of the specified \c GstBuffer with the caller.

    \sa createFrameFromGstBuffer(GstBuffer *, const GstVideoInfo &), gstBuffer()
*/

QGStreamerInterface::~QGStreamerInterface() = default;

QT_END_NAMESPACE
