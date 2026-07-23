// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef GSTREAMERINTERFACE_H
#define GSTREAMERINTERFACE_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QtMM semi-private API (SPI), with limited compatibility guarantees.
// Usage of this API may make your code source and binary incompatible with future versions of Qt.
//

#include <QtMultimedia/qvideoframe.h>

// NOLINTBEGIN (bugprone-reserved-identifier)
typedef struct _GstPipeline GstPipeline;
typedef struct _GstElement GstElement;
typedef struct _GstBuffer GstBuffer;
typedef struct _GstVideoInfo GstVideoInfo;
typedef struct _GstVideoInfoDmaDrm GstVideoInfoDmaDrm;
// NOLINTEND (bugprone-reserved-identifier)

QT_BEGIN_NAMESPACE

class QAudioDevice;
class QCamera;
class QMediaPlayer;
class QMediaCaptureSession;
class QObject;

class Q_MULTIMEDIA_EXPORT QGStreamerInterface
{
public:
    virtual ~QGStreamerInterface();

    static QGStreamerInterface *instance();

    QT_DEPRECATED_X("Use QGStreamerVideoSource and QMediaCaptureSession::setNativeVideoSource() instead.")
    virtual QCamera *makeCustomGStreamerCamera(const QByteArray &gstBinDescription, QObject *parent) = 0;

    QT_DEPRECATED_X("Use QGStreamerVideoSource and QMediaCaptureSession::setNativeVideoSource() instead.")
    virtual QCamera *makeCustomGStreamerCamera(GstElement *element, QObject *parent) = 0;

    virtual QAudioDevice makeCustomGStreamerAudioInput(const QByteArray &gstreamerPipeline) = 0;
    virtual QAudioDevice makeCustomGStreamerAudioOutput(const QByteArray &gstreamerPipeline) = 0;

    virtual GstPipeline *gstPipeline(QMediaPlayer *) = 0;
    virtual GstPipeline *gstPipeline(QMediaCaptureSession *) = 0;

    // does not transfer ownership
    virtual GstBuffer *gstBuffer(const QVideoFrame &frame) = 0;

    // NOTE: Ownership of GstBuffer is not transferred
    virtual QVideoFrame createFrameFromGstBuffer(GstBuffer *buffer,
                                                 const GstVideoInfo &videoInfo) = 0;
    virtual QVideoFrame createFrameFromGstBuffer(GstBuffer *buffer,
                                                 const GstVideoInfoDmaDrm &videoInfo) = 0;
};

using QGStreamerPlatformSpecificInterface = QGStreamerInterface;

QT_END_NAMESPACE

#endif // GSTREAMERINTERFACE_H
