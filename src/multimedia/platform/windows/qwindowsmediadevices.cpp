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

#include "qwindowsmediadevices_p.h"
#include "qmediadevices.h"
#include "qcamerainfo_p.h"
#include "qvarlengtharray.h"

#include "private/qwindowsaudioinput_p.h"
#include "private/qwindowsaudiooutput_p.h"
#include "private/qwindowsaudiodeviceinfo_p.h"
#include "private/qwindowsmultimediautils_p.h"

#include <private/mftvideo_p.h>

#include <mmsystem.h>
#include <mmddk.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <Mferror.h>
#include "private/qwindowsaudioutils_p.h"

QT_BEGIN_NAMESPACE

QWindowsMediaDevices::QWindowsMediaDevices()
    : QPlatformMediaDevices()
{
}

static QList<QAudioDeviceInfo> availableDevices(QAudio::Mode mode)
{
    Q_UNUSED(mode);

    QList<QAudioDeviceInfo> devices;
    //enumerate device fullnames through directshow api
    auto hrCoInit = CoInitialize(nullptr);
    ICreateDevEnum *pDevEnum = NULL;
    IEnumMoniker *pEnum = NULL;
    // Create the System device enumerator
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
                 CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
                 reinterpret_cast<void **>(&pDevEnum));

    unsigned long iNumDevs = mode == QAudio::AudioOutput ? waveOutGetNumDevs() : waveInGetNumDevs();
    if (SUCCEEDED(hr)) {
        // Create the enumerator for the audio input/output category
        if (pDevEnum->CreateClassEnumerator(
             mode == QAudio::AudioOutput ? CLSID_AudioRendererCategory : CLSID_AudioInputDeviceCategory,
             &pEnum, 0) == S_OK) {
            pEnum->Reset();
            // go through and find all audio devices
            IMoniker *pMoniker = NULL;
            while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                IPropertyBag *pPropBag;
                hr = pMoniker->BindToStorage(0,0,IID_IPropertyBag,
                     reinterpret_cast<void **>(&pPropBag));
                if (FAILED(hr)) {
                    pMoniker->Release();
                    continue; // skip this one
                }
                // Find if it is a wave device
                VARIANT var;
                VariantInit(&var);
                hr = pPropBag->Read(mode == QAudio::AudioOutput ? L"WaveOutID" : L"WaveInID", &var, 0);
                if (SUCCEEDED(hr)) {
                    LONG waveID = var.lVal;
                    if (waveID >= 0 && waveID < LONG(iNumDevs)) {
                        VariantClear(&var);
                        // Find the description
                        hr = pPropBag->Read(L"FriendlyName", &var, 0);
                        if (!SUCCEEDED(hr))
                            continue;
                        QString description = QString::fromWCharArray(var.bstrVal);

                        // Get the endpoint ID string for this waveOut device. This is required to be able to
                        // identify the device use the WMF APIs
                        size_t len = 0;
                        MMRESULT mmr = waveOutMessage((HWAVEOUT)IntToPtr(waveID),
                                                     DRV_QUERYFUNCTIONINSTANCEIDSIZE,
                                                     (DWORD_PTR)&len, 0);
                        if (mmr != MMSYSERR_NOERROR)
                            continue;
                        QVarLengthArray<WCHAR> id(len);
                        mmr = waveOutMessage((HWAVEOUT)IntToPtr(waveID),
                                             DRV_QUERYFUNCTIONINSTANCEID,
                                             (DWORD_PTR)id.data(),
                                             len);
                        if (mmr != MMSYSERR_NOERROR)
                            continue;
                        QByteArray strId = QString::fromWCharArray(id.data()).toUtf8();

                        devices.append((new QWindowsAudioDeviceInfo(strId, waveID, description, mode))->create());
                    }
                }

                pPropBag->Release();
                pMoniker->Release();
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
    if (SUCCEEDED(hrCoInit))
        CoUninitialize();

    return devices;
}

QList<QAudioDeviceInfo> QWindowsMediaDevices::audioInputs() const
{
    return availableDevices(QAudio::AudioInput);
}

QList<QAudioDeviceInfo> QWindowsMediaDevices::audioOutputs() const
{
    return availableDevices(QAudio::AudioOutput);
}

QList<QCameraInfo> QWindowsMediaDevices::videoInputs() const
{
    QList<QCameraInfo> cameras;
    auto hrCoInit = CoInitialize(nullptr);

    IMFAttributes *pAttributes = NULL;
    IMFActivate **ppDevices = NULL;

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (SUCCEEDED(hr)) {
        // Source type: video capture devices
        hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                                  MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

        if (SUCCEEDED(hr)) {
            // Enumerate devices.
            UINT32 count;
            hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
            if (SUCCEEDED(hr)) {
                // Iterate through devices.
                for (int index = 0; index < int(count); index++) {
                    QCameraInfoPrivate *info = new QCameraInfoPrivate;

                    IMFMediaSource *pSource = NULL;
                    IMFSourceReader *reader = NULL;

                    WCHAR *deviceName = NULL;
                    UINT32 deviceNameLength = 0;
                    UINT32 deviceIdLength = 0;
                    WCHAR *deviceId = NULL;

                    hr = ppDevices[index]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                                              &deviceName, &deviceNameLength);
                    if (SUCCEEDED(hr))
                        info->description = QString::fromWCharArray(deviceName);
                    CoTaskMemFree(deviceName);

                    hr = ppDevices[index]->GetAllocatedString(
                            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &deviceId,
                            &deviceIdLength);
                    if (SUCCEEDED(hr))
                        info->id = QString::fromWCharArray(deviceId).toUtf8();
                    CoTaskMemFree(deviceId);

                    // Create the media source object.
                    hr = ppDevices[index]->ActivateObject(
                            IID_PPV_ARGS(&pSource));
                    // Create the media source reader.
                    hr = MFCreateSourceReaderFromMediaSource(pSource, NULL, &reader);
                    if (SUCCEEDED(hr)) {
                        QList<QSize> photoResolutions;
                        QList<QCameraFormat> videoFormats;

                        DWORD dwMediaTypeIndex = 0;
                        IMFMediaType *mediaFormat = NULL;
                        GUID subtype = GUID_NULL;
                        HRESULT mediaFormatResult = S_OK;

                        UINT32 frameRateMin = 0u;
                        UINT32 frameRateMax = 0u;
                        UINT32 denominator = 0u;
                        DWORD index = 0u;
                        UINT32 width = 0u;
                        UINT32 height = 0u;

                        while (SUCCEEDED(mediaFormatResult)) {
                            // Loop through the supported formats for the video device
                            mediaFormatResult = reader->GetNativeMediaType(
                                    (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, dwMediaTypeIndex,
                                    &mediaFormat);
                            if (mediaFormatResult == MF_E_NO_MORE_TYPES)
                                break;
                            else if (SUCCEEDED(mediaFormatResult)) {
                                QVideoFrameFormat::PixelFormat pixelFormat = QVideoFrameFormat::Format_Invalid;
                                QSize resolution;
                                float minFr = .0;
                                float maxFr = .0;

                                if (SUCCEEDED(mediaFormat->GetGUID(MF_MT_SUBTYPE, &subtype)))
                                    pixelFormat = QWindowsMultimediaUtils::pixelFormatFromMediaSubtype(subtype);

                                if (SUCCEEDED(MFGetAttributeSize(mediaFormat, MF_MT_FRAME_SIZE, &width,
                                                        &height))) {
                                    resolution.rheight() = (int)height;
                                    resolution.rwidth() = (int)width;
                                    photoResolutions << resolution;
                                }

                                if (SUCCEEDED(MFGetAttributeRatio(mediaFormat, MF_MT_FRAME_RATE_RANGE_MIN,
                                                         &frameRateMin, &denominator)))
                                    minFr = qreal(frameRateMin) / denominator;
                                if (SUCCEEDED(MFGetAttributeRatio(mediaFormat, MF_MT_FRAME_RATE_RANGE_MAX,
                                                         &frameRateMax, &denominator)))
                                    maxFr = qreal(frameRateMax) / denominator;

                                auto *f = new QCameraFormatPrivate { QSharedData(), pixelFormat,
                                                                     resolution, minFr, maxFr };
                                videoFormats << f->create();
                            }
                            ++dwMediaTypeIndex;
                        }
                        if (mediaFormat)
                            mediaFormat->Release();

                        info->videoFormats = videoFormats;
                        info->photoResolutions = photoResolutions;
                    }
                    if (reader)
                        reader->Release();
                    cameras.append(info->create());
                }
            }
            for (DWORD i = 0; i < count; i++) {
                if (ppDevices[i])
                    ppDevices[i]->Release();
            }
            CoTaskMemFree(ppDevices);
        }
    }
    if (pAttributes)
        pAttributes->Release();
    if (SUCCEEDED(hrCoInit))
        CoUninitialize();

    return cameras;
}

QAbstractAudioInput *QWindowsMediaDevices::createAudioInputDevice(const QAudioDeviceInfo &deviceInfo)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioInput(devInfo->waveId());
}

QAbstractAudioOutput *QWindowsMediaDevices::createAudioOutputDevice(const QAudioDeviceInfo &deviceInfo)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioOutput(devInfo->waveId());
}

QT_END_NAMESPACE
