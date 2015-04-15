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

#include <QDebug>
#include <QFile>
#include <qelapsedtimer.h>

#include "dsvideodevicecontrol.h"
#include "dscamerasession.h"

#include <tchar.h>
#include <dshow.h>
#include <objbase.h>
#include <initguid.h>
#include <ocidl.h>
#include <string.h>

extern const CLSID CLSID_VideoInputDeviceCategory;

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QList<DSVideoDeviceInfo>, deviceList)

DSVideoDeviceControl::DSVideoDeviceControl(QObject *parent)
    : QVideoDeviceSelectorControl(parent)
{
    m_session = qobject_cast<DSCameraSession*>(parent);
    selected = 0;
}

int DSVideoDeviceControl::deviceCount() const
{
    updateDevices();
    return deviceList->count();
}

QString DSVideoDeviceControl::deviceName(int index) const
{
    updateDevices();

    if (index >= 0 && index <= deviceList->count())
        return QString::fromUtf8(deviceList->at(index).first.constData());

    return QString();
}

QString DSVideoDeviceControl::deviceDescription(int index) const
{
    updateDevices();

    if (index >= 0 && index <= deviceList->count())
        return deviceList->at(index).second;

    return QString();
}

int DSVideoDeviceControl::defaultDevice() const
{
    return 0;
}

int DSVideoDeviceControl::selectedDevice() const
{
    return selected;
}

void DSVideoDeviceControl::setSelectedDevice(int index)
{
    updateDevices();

    if (index >= 0 && index < deviceList->count()) {
        if (m_session) {
            QString device = deviceList->at(index).first;
            if (device.startsWith("ds:"))
                device.remove(0,3);
            m_session->setDevice(device);
        }
        selected = index;
    }
}

const QList<DSVideoDeviceInfo> &DSVideoDeviceControl::availableDevices()
{
    updateDevices();
    return *deviceList;
}

void DSVideoDeviceControl::updateDevices()
{
    static QElapsedTimer timer;
    if (timer.isValid() && timer.elapsed() < 500) // ms
        return;

    deviceList->clear();

    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;
    // Create the System device enumerator
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
            CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
            reinterpret_cast<void**>(&pDevEnum));
    if (SUCCEEDED(hr)) {
        // Create the enumerator for the video capture category
        hr = pDevEnum->CreateClassEnumerator(
                CLSID_VideoInputDeviceCategory, &pEnum, 0);
        if (S_OK == hr) {
            pEnum->Reset();
            // go through and find all video capture devices
            IMoniker* pMoniker = NULL;
            IMalloc *mallocInterface = 0;
            CoGetMalloc(1, (LPMALLOC*)&mallocInterface);
            while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                BSTR strName = 0;
                hr = pMoniker->GetDisplayName(NULL, NULL, &strName);
                if (SUCCEEDED(hr)) {
                    QString output(QString::fromWCharArray(strName));
                    mallocInterface->Free(strName);

                    DSVideoDeviceInfo devInfo;
                    devInfo.first = output.toUtf8();

                    IPropertyBag *pPropBag;
                    hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)(&pPropBag));
                    if (SUCCEEDED(hr)) {
                        // Find the description
                        VARIANT varName;
                        varName.vt = VT_BSTR;
                        hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                        if (SUCCEEDED(hr)) {
                            output = QString::fromWCharArray(varName.bstrVal);
                        }
                        pPropBag->Release();
                    }
                    devInfo.second = output;

                    deviceList->append(devInfo);
                }
                pMoniker->Release();
            }
            mallocInterface->Release();
            pEnum->Release();
        }
        pDevEnum->Release();
    }

    timer.restart();
}

QT_END_NAMESPACE
