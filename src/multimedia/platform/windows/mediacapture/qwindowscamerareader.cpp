/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qwindowscamerareader_p.h"

#include "qwindowsmultimediautils_p.h"
#include <qvideosink.h>
#include <private/qmemoryvideobuffer_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

QWindowsCameraReader::QWindowsCameraReader(QObject *parent)
    : QObject(parent)
{
}

QWindowsCameraReader::~QWindowsCameraReader()
{
    stop();
}

HRESULT QWindowsCameraReader::start(const QString &cameraId)
{
    if (m_sourceReader) {
        m_sourceReader->Release();
        m_sourceReader = nullptr;
    }
    if (m_source) {
        m_source->Release();
        m_source = nullptr;
    }

    m_started = false;

    IMFAttributes *srcattr = nullptr;

    HRESULT hr = MFCreateAttributes(&srcattr, 2);
    if (SUCCEEDED(hr)) {

        hr = srcattr->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                              MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        if (SUCCEEDED(hr)) {

            hr = srcattr->SetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                                    (LPCWSTR)cameraId.utf16());
            if (SUCCEEDED(hr)) {

                // get the media source associated with the symbolic link
                hr = MFCreateDeviceSource(srcattr, &m_source);
                if (SUCCEEDED(hr)) {

                    IMFAttributes *rdrattr = nullptr;
                    hr = MFCreateAttributes(&rdrattr, 1);
                    if (SUCCEEDED(hr)) {

                        hr = rdrattr->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);
                        if (SUCCEEDED(hr)) {

                            hr = MFCreateSourceReaderFromMediaSource(m_source, rdrattr, &m_sourceReader);
                            if (SUCCEEDED(hr)) {

                                IMFMediaType *mediaType = NULL;
                                hr = m_sourceReader->GetCurrentMediaType(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &mediaType);
                                if (SUCCEEDED(hr)) {

                                    GUID subtype = GUID_NULL;
                                    hr = mediaType->GetGUID(MF_MT_SUBTYPE, &subtype);
                                    if (SUCCEEDED(hr)) {

                                        m_pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);

                                        if (m_pixelFormat == QVideoFrameFormat::Format_Invalid) {
                                            hr = S_FALSE;
                                        } else {

                                            // get the frame dimensions
                                            hr = MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &m_frameWidth, &m_frameHeight);
                                            if (SUCCEEDED(hr)) {

                                                // and the stride, which we need to convert the frame later
                                                hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, m_frameWidth, &m_stride);
                                                if (SUCCEEDED(hr)) {

                                                    hr = m_sourceReader->SetStreamSelection(DWORD(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
                                                    if (SUCCEEDED(hr)) {

                                                        // request the first frame
                                                        hr = m_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                                                                        0, nullptr, nullptr, nullptr, nullptr);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    mediaType->Release();
                                }
                            }
                        }
                        rdrattr->Release();
                    }
                }
            }
        }
        srcattr->Release();
    }
    return hr;
}

HRESULT QWindowsCameraReader::stop()
{
    if (m_sourceReader) {
        m_sourceReader->Release();
        m_sourceReader = nullptr;
    }
    if (m_source) {
        m_source->Release();
        m_source = nullptr;
    }
    return S_OK;
}

//from IUnknown
STDMETHODIMP QWindowsCameraReader::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (riid == IID_IMFSourceReaderCallback) {
        *ppvObject = static_cast<IMFSourceReaderCallback*>(this);
    } else if (riid == IID_IUnknown) {
        *ppvObject = static_cast<IUnknown*>(this);
    } else {
        *ppvObject =  nullptr;
        return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) QWindowsCameraReader::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) QWindowsCameraReader::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        this->deleteLater();
    }
    return cRef;
}

void QWindowsCameraReader::setSurface(QVideoSink *surface)
{
    m_surface = surface;
}

//from IMFSourceReaderCallback
STDMETHODIMP QWindowsCameraReader::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex,
                                                DWORD dwStreamFlags, LONGLONG llTimestamp,
                                                IMFSample *pSample)
{
    Q_UNUSED(hrStatus);
    Q_UNUSED(dwStreamIndex);
    Q_UNUSED(llTimestamp);

    if ((dwStreamFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) == MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) {
        qDebug() << "QWindowsCameraReader::OnReadSample - MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED";
    }

    if ((dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM) == MF_SOURCE_READERF_ENDOFSTREAM) {
        emit streamStopped();
        qDebug() << "QWindowsCameraReader::OnReadSample - MF_SOURCE_READERF_ENDOFSTREAM";
    } else {
        if (!m_started) {
            m_started = true;
            emit streamStarted();
        }
        if (pSample && m_surface) {

            IMFMediaBuffer *mediaBuffer = nullptr;
            if (SUCCEEDED(pSample->ConvertToContiguousBuffer(&mediaBuffer))) {

                DWORD bufLen = 0;
                BYTE *buffer = nullptr;

                if (SUCCEEDED(mediaBuffer->Lock(&buffer, nullptr, &bufLen))) {
                    auto bytes = QByteArray(reinterpret_cast<char*>(buffer), bufLen);

                    QVideoFrame frame(new QMemoryVideoBuffer(bytes, m_stride),
                                      QVideoFrameFormat(QSize(m_frameWidth, m_frameHeight), m_pixelFormat));

                    // WMF uses 100-nanosecond units, Qt uses microseconds
                    LONGLONG startTime = 0;
                    if (SUCCEEDED(pSample->GetSampleTime(&startTime))) {
                        frame.setStartTime(startTime * 0.1);

                        LONGLONG duration = -1;
                        if (SUCCEEDED(pSample->GetSampleDuration(&duration)))
                            frame.setEndTime((startTime + duration) * 0.1);
                    }

                    m_surface->newVideoFrame(frame);
                    mediaBuffer->Unlock();
                }
                mediaBuffer->Release();
            }
        }
        // request the next frame
        m_sourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                   0, nullptr, nullptr, nullptr, nullptr);
    }

    return S_OK;
}

STDMETHODIMP QWindowsCameraReader::OnFlush(DWORD)
{
    return S_OK;
}

STDMETHODIMP QWindowsCameraReader::OnEvent(DWORD, IMFMediaEvent*)
{
    return S_OK;
}

QT_END_NAMESPACE
