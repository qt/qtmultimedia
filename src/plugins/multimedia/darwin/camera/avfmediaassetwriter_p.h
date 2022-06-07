// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFMEDIAASSETWRITER_H
#define AVFMEDIAASSETWRITER_H

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

#include "avfcamerautility_p.h"
#include "qmediaformat.h"

#include <QtCore/qglobal.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFMediaEncoder;
class AVFCameraService;

QT_END_NAMESPACE

@interface QT_MANGLE_NAMESPACE(AVFMediaAssetWriter) : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate,
                                                               AVCaptureAudioDataOutputSampleBufferDelegate>
- (id)initWithDelegate:(QT_PREPEND_NAMESPACE(AVFMediaEncoder) *)delegate;

- (bool)setupWithFileURL:(NSURL *)fileURL
        cameraService:(QT_PREPEND_NAMESPACE(AVFCameraService) *)service
        audioSettings:(NSDictionary *)audioSettings
        videoSettings:(NSDictionary *)videoSettings
        fileFormat:(QMediaFormat::FileFormat)fileFormat
        transform:(CGAffineTransform)transform;

// This to be called from the recorder control's thread:
- (void)start;
- (void)stop;
- (void)pause;
- (void)resume;
// This to be called from the recorder control's dtor:
- (void)abort;
- (qint64)durationInMs;

@end

#endif // AVFMEDIAASSETWRITER_H
