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

#include <qtconfigmacros.h>
#include <qtypes.h>

#include <memory>
#include <functional>

QT_BEGIN_NAMESPACE

class QAVSampleBufferDelegateFrameHandler;
class QVideoFrame;
namespace QFFmpeg {
class HWAccel;
}

QT_END_NAMESPACE

@interface QAVFSampleBufferDelegate : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

- (instancetype)initWithFrameHandler:(std::function<void(const QVideoFrame &)>)handler;

- (void)captureOutput:(AVCaptureOutput *)captureOutput
        didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
               fromConnection:(AVCaptureConnection *)connection;

- (void)setHWAccel:(std::unique_ptr<QT_PREPEND_NAMESPACE(QFFmpeg::HWAccel)> &&)accel;

- (void)setVideoFormatFrameRate:(qreal)frameRate;

@end

#endif
