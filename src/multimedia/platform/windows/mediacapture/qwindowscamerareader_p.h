/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
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

#ifndef QWINDOWSCAMERAREADER_H
#define QWINDOWSCAMERAREADER_H

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

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qsemaphore.h>
#include <qvideoframe.h>

QT_BEGIN_NAMESPACE

class QVideoSink;

class QWindowsCameraReader : public QObject,
        public IMFSourceReaderCallback,
        public IMFSinkWriterCallback
{
    Q_OBJECT
public:
    explicit QWindowsCameraReader(QObject *parent = nullptr);
    ~QWindowsCameraReader();

    QVideoSink *surface() const;
    void setSurface(QVideoSink *surface);

    //from IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //from IMFSourceReaderCallback
    STDMETHODIMP OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
                              DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample);
    STDMETHODIMP OnFlush(DWORD dwStreamIndex);
    STDMETHODIMP OnEvent(DWORD dwStreamIndex, IMFMediaEvent *pEvent);

    //from IMFSinkWriterCallback
    STDMETHODIMP OnFinalize(HRESULT hrStatus);
    STDMETHODIMP OnMarker(DWORD dwStreamIndex, LPVOID pvContext);

    bool activate(const QString &cameraId);
    void deactivate();

    bool startRecording(const QString &fileName, const GUID &container,
                        const GUID &videoFormat, UINT32 bitRate, UINT32 width,
                        UINT32 height, qreal frameRate);
    void stopRecording();
    bool pauseRecording();
    bool resumeRecording();

    UINT32 frameWidth() const;
    UINT32 frameHeight() const;
    qreal frameRate() const;

Q_SIGNALS:
    void streamingStarted();
    void streamingStopped();
    void recordingStarted();
    void recordingStopped();
    void durationChanged(qint64 duration);

private:
    void stopStreaming();

    long               m_cRef = 1;
    QMutex             m_mutex;
    QSemaphore         m_finalizeSemaphore;
    IMFMediaSource     *m_source = nullptr;
    IMFSourceReader    *m_sourceReader = nullptr;
    IMFMediaType       *m_sourceMediaType = nullptr;
    IMFSinkWriter      *m_sinkWriter = nullptr;
    DWORD              m_videoStreamIndex = 0;
    QVideoSink         *m_surface = nullptr;
    UINT32             m_frameWidth = 0;
    UINT32             m_frameHeight = 0;
    qreal              m_frameRate = 0.0;
    LONG               m_stride = 0;
    bool               m_active = false;
    bool               m_streaming = false;
    bool               m_recording = false;
    bool               m_firstFrame = false;
    bool               m_paused = false;
    bool               m_pauseChanging = false;
    QVideoFrameFormat::PixelFormat m_pixelFormat = QVideoFrameFormat::Format_Invalid;
    LONGLONG           m_timeOffset = 0;
    LONGLONG           m_pauseTime = 0;
};

QT_END_NAMESPACE

#endif//QWINDOWSCAMERAREADER_H
