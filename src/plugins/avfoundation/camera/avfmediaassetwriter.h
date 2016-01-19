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

#ifndef AVFMEDIAASSETWRITER_H
#define AVFMEDIAASSETWRITER_H

#include "avfcamerautility.h"

#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>
#include <QtCore/qmutex.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraService;

class AVFMediaAssetWriterDelegate
{
public:
    virtual ~AVFMediaAssetWriterDelegate();

    virtual void assetWriterStarted() = 0;
    virtual void assetWriterFailedToStart() = 0;
    virtual void assetWriterFailedToStop() = 0;
    virtual void assetWriterFinished() = 0;
};

typedef QAtomicInteger<bool> AVFAtomicBool;
typedef QAtomicInteger<qint64> AVFAtomicInt64;

QT_END_NAMESPACE

// TODO: any reasonable error handling requires smart pointers, otherwise it's getting crappy immediately.

@interface QT_MANGLE_NAMESPACE(AVFMediaAssetWriter) : NSObject<AVCaptureVideoDataOutputSampleBufferDelegate,
                                                               AVCaptureAudioDataOutputSampleBufferDelegate>
{
@private
    AVFCameraService *m_service;

    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVAssetWriterInput> m_cameraWriterInput;
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVCaptureDeviceInput> m_audioInput;
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVCaptureAudioDataOutput> m_audioOutput;
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVAssetWriterInput> m_audioWriterInput;

    // High priority serial queue for video output:
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<dispatch_queue_t> m_videoQueue;
    // Serial queue for audio output:
    QT_PREPEND_NAMESPACE(AVFScopedPointer)<dispatch_queue_t> m_audioQueue;
    // Queue to write sample buffers:
    __weak dispatch_queue_t m_writerQueue;

    QT_PREPEND_NAMESPACE(AVFScopedPointer)<AVAssetWriter> m_assetWriter;
    // Delegate's queue.
    __weak dispatch_queue_t m_delegateQueue;
    // TODO: QPointer??
    QT_PREPEND_NAMESPACE(AVFMediaAssetWriterDelegate) *m_delegate;

    bool m_setStartTime;
    QT_PREPEND_NAMESPACE(AVFAtomicBool) m_stopped;
    bool m_stoppedInternal;
    bool m_aborted;

    QT_PREPEND_NAMESPACE(QMutex) m_writerMutex;
@public
    QT_PREPEND_NAMESPACE(AVFAtomicInt64) m_durationInMs;
@private
    CMTime m_startTime;
    CMTime m_lastTimeStamp;
}

- (id)initWithQueue:(dispatch_queue_t)writerQueue
      delegate:(QT_PREPEND_NAMESPACE(AVFMediaAssetWriterDelegate) *)delegate
      delegateQueue:(dispatch_queue_t)delegateQueue;

- (bool)setupWithFileURL:(NSURL *)fileURL
        cameraService:(QT_PREPEND_NAMESPACE(AVFCameraService) *)service;

- (void)start;
- (void)stop;
// This to be called if control's dtor gets called,
// on the control's thread.
- (void)abort;

@end

#endif // AVFMEDIAASSETWRITER_H
