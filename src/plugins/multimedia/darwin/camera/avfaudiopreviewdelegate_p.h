// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFAUDIOPREVIEWDELEGATE_P_H
#define AVFAUDIOPREVIEWDELEGATE_P_H

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

#include <QtCore/qglobal.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraSession;

QT_END_NAMESPACE

@interface AVFAudioPreviewDelegate : NSObject<AVCaptureAudioDataOutputSampleBufferDelegate>

- (id)init;
- (void)setupWithCaptureSession: (AVFCameraSession*)session
        audioOutputDevice: (NSString*)deviceId;
- (void)renderAudioSampleBuffer:(CMSampleBufferRef)sampleBuffer;
- (void)resetAudioPreviewDelegate;
- (void)setVolume: (float)volume;
- (void)setMuted: (bool)muted;

@end

#endif // AVFAUDIOPREVIEWDELEGATE_P_H
