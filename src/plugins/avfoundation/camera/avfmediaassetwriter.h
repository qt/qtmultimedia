/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFMEDIAASSETWRITER_H
#define AVFMEDIAASSETWRITER_H

#include "avfcamerautility.h"

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFMediaRecorderControlIOS;
class AVFCameraService;

typedef QAtomicInteger<qint64> AVFAtomicInt64;

QT_END_NAMESPACE

@interface QT_MANGLE_NAMESPACE(AVFMediaAssetWriter) : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate,
                                                               AVCaptureAudioDataOutputSampleBufferDelegate>
{
@private
    AVFCameraService *m_service;

    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVAssetWriterInput> m_cameraWriterInput;
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVCaptureDeviceInput> m_audioInput;
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVCaptureAudioDataOutput> m_audioOutput;
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVAssetWriterInput> m_audioWriterInput;
    AVCaptureDevice *m_audioCaptureDevice;

    // Queue to write sample buffers:
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<dispatch_queue_t> m_writerQueue;
    // High priority serial queue for video output:
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<dispatch_queue_t> m_videoQueue;
    // Serial queue for audio output:
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<dispatch_queue_t> m_audioQueue;

    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVAssetWriter> m_assetWriter;

    QT_PREPEND_NAMESPACE(AVFMediaRecorderControlIOS) *m_delegate;

    bool m_setStartTime;

    QT_PREPEND_NAMESPACE(QAtomicInt) m_state;
@public
    QT_PREPEND_NAMESPACE(AVFAtomicInt64) m_durationInMs;
@private
    CMTime m_startTime;
    CMTime m_lastTimeStamp;

    NSDictionary *m_audioSettings;
    NSDictionary *m_videoSettings;
}

- (id)initWithDelegate:(QT_PREPEND_NAMESPACE(AVFMediaRecorderControlIOS) *)delegate;

- (bool)setupWithFileURL:(NSURL *)fileURL
        cameraService:(QT_PREPEND_NAMESPACE(AVFCameraService) *)service
        audioSettings:(NSDictionary *)audioSettings
        videoSettings:(NSDictionary *)videoSettings
        transform:(CGAffineTransform)transform;

// This to be called from the recorder control's thread:
- (void)start;
- (void)stop;
// This to be called from the recorder control's dtor:
- (void)abort;

@end

#endif // AVFMEDIAASSETWRITER_H
