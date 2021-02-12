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

#include "qwindowsdevicemanager_p.h"
#include "qmediadevicemanager.h"
#include "qcamerainfo_p.h"
#include "qvarlengtharray.h"

#include "private/qwindowsaudioinput_p.h"
#include "private/qwindowsaudiooutput_p.h"
#include "private/qwindowsaudiodeviceinfo_p.h"

#include <mmsystem.h>
#include <mmddk.h>
#include "private/qwindowsaudioutils_p.h"

QT_BEGIN_NAMESPACE

QWindowsDeviceManager::QWindowsDeviceManager()
    : QPlatformMediaDeviceManager()
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

QList<QAudioDeviceInfo> QWindowsDeviceManager::audioInputs() const
{
    return availableDevices(QAudio::AudioInput);
}

QList<QAudioDeviceInfo> QWindowsDeviceManager::audioOutputs() const
{
    return availableDevices(QAudio::AudioOutput);
}

QList<QCameraInfo> QWindowsDeviceManager::videoInputs() const
{
    return {};
}

QAbstractAudioInput *QWindowsDeviceManager::createAudioInputDevice(const QAudioDeviceInfo &deviceInfo)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioInput(devInfo->waveId());
}

QAbstractAudioOutput *QWindowsDeviceManager::createAudioOutputDevice(const QAudioDeviceInfo &deviceInfo)
{
    const auto *devInfo = static_cast<const QWindowsAudioDeviceInfo *>(deviceInfo.handle());
    return new QWindowsAudioOutput(devInfo->waveId());
}

QT_END_NAMESPACE
