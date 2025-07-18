// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFSAMPLEBUFFERDELEGATE_P_H
#define QAVFSAMPLEBUFFERDELEGATE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtypes.h>

#include <QtMultimedia/private/qvideotransformation_p.h>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE

class QAVSampleBufferDelegateFrameHandler;
class QVideoFrame;
namespace QFFmpeg {
class HWAccel;
}

QT_END_NAMESPACE

// This type is used by screencapture and camera-capture.
@interface QAVFSampleBufferDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

// These parameters are called during the captureOutput callback.
//
// The handler parameter is called at the end, when the QVideoFrame is constructed and configured.
//
// The modifier is called to modify the frame before it is finally sent to the handler.
// This can be used to i.e add metadata to the frame.
- (instancetype)initWithFrameHandler:(std::function<void(const QVideoFrame &)>)handler;

// Allows the object to update the QVideoFrame metadata based on rotatation and mirroring.
// This does NOT rotate the pixel buffer.
- (void)setTransformationProvider:(std::function<VideoTransformation()>)provider;

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
               fromConnection:(AVCaptureConnection *)connection;

- (void)setHWAccel:(std::unique_ptr<QT_PREPEND_NAMESPACE(QFFmpeg::HWAccel)> &&)accel;

- (void)setVideoFormatFrameRate:(qreal)frameRate;

@end

#endif
