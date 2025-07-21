// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFCAPTUREPHOTOOUTPUTDELEGATE_P_H
#define QAVFCAPTUREPHOTOOUTPUTDELEGATE_P_H

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

#include <QtCore/qtconfigmacros.h>

#include <QtFFmpegMediaPluginImpl/private/qavfstillphotonotifier_p.h>

#import <AVFoundation/AVFoundation.h>

// This class allows us to track the progress of an on-going camera still photo capture
@interface QT_MANGLE_NAMESPACE(QAVFCapturePhotoOutputDelegate) : NSObject <AVCapturePhotoCaptureDelegate>

- (instancetype) init:(AVCaptureDevice *)device;

- (QT_PREPEND_NAMESPACE(QFFmpeg::QAVFStillPhotoNotifier) &)notifier;

- (void) captureOutput:(AVCapturePhotoOutput *)output
    didFinishProcessingPhoto:(AVCapturePhoto *)photo
    error:(NSError *) error;

@end

#endif // QAVFCAPTUREPHOTOOUTPUTDELEGATE_P_H
