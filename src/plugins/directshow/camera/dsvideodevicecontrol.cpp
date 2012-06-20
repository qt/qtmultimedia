/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <QFile>

#include "dsvideodevicecontrol.h"
#include "dscamerasession.h"

#include <tchar.h>
#include <dshow.h>
#include <objbase.h>
#include <initguid.h>
#include <Ocidl.h>
#include <string.h>

extern const CLSID CLSID_VideoInputDeviceCategory;

QT_BEGIN_NAMESPACE

DSVideoDeviceControl::DSVideoDeviceControl(QObject *parent)
    : QVideoDeviceSelectorControl(parent)
{
    m_session = qobject_cast<DSCameraSession*>(parent);

    devices.clear();
    descriptions.clear();

    CoInitialize(NULL);
    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;
    // Create the System device enumerator
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
            CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
            reinterpret_cast<void**>(&pDevEnum));
    if(SUCCEEDED(hr)) {
        // Create the enumerator for the video capture category
        hr = pDevEnum->CreateClassEnumerator(
                CLSID_VideoInputDeviceCategory, &pEnum, 0);
        if (S_OK == hr) {
            pEnum->Reset();
            // go through and find all video capture devices
            IMoniker* pMoniker = NULL;
            while(pEnum->Next(1, &pMoniker, NULL) == S_OK) {
                IPropertyBag *pPropBag;
                hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
                        (void**)(&pPropBag));
                if(FAILED(hr)) {
                    pMoniker->Release();
                    continue; // skip this one
                }
                // Find the description
                WCHAR str[120];
                VARIANT varName;
                varName.vt = VT_BSTR;
                hr = pPropBag->Read(L"FriendlyName", &varName, 0);
                if(SUCCEEDED(hr)) {
                    wcsncpy(str, varName.bstrVal, sizeof(str)/sizeof(str[0]));
                    QString temp(QString::fromUtf16((unsigned short*)str));
                    devices.append(QString("ds:%1").arg(temp).toLocal8Bit().constData());
                    hr = pPropBag->Read(L"Description", &varName, 0);
                    wcsncpy(str, varName.bstrVal, sizeof(str)/sizeof(str[0]));
                    QString temp2(QString::fromUtf16((unsigned short*)str));
                    descriptions.append(temp2.toLocal8Bit().constData());
                }
                pPropBag->Release();
                pMoniker->Release();
            }
            pEnum->Release();
        }
        pDevEnum->Release();
    }
    CoUninitialize();

    selected = 0;
}

int DSVideoDeviceControl::deviceCount() const
{
    return devices.count();
}

QString DSVideoDeviceControl::deviceName(int index) const
{
    if(index >= 0 && index <= devices.count())
        return devices.at(index);

    return QString();
}

QString DSVideoDeviceControl::deviceDescription(int index) const
{
    if(index >= 0 && index <= descriptions.count())
        return descriptions.at(index);

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
    if(index >= 0 && index <= devices.count()) {
        if (m_session) {
            QString device = devices.at(index);
            if (device.startsWith("ds:"))
                device.remove(0,3);
            m_session->setDevice(device);
        }
        selected = index;
    }
}

QT_END_NAMESPACE
