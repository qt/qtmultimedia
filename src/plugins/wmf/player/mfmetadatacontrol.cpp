/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mfmetadatacontrol.h"
#include "mfplayerservice.h"
#include "Propkey.h"

//#define DEBUG_MEDIAFOUNDATION

MFMetaDataControl::MFMetaDataControl(QObject *parent)
    : QMetaDataReaderControl(parent)
    , m_metaData(0)
    , m_content(0)
{
}

MFMetaDataControl::~MFMetaDataControl()
{
    if (m_metaData)
        m_metaData->Release();
    if (m_content)
        m_content->Release();
}

bool MFMetaDataControl::isMetaDataAvailable() const
{
    return m_content || m_metaData;
}

QVariant MFMetaDataControl::metaData(const QString &key) const
{
    QVariant value;
    if (!isMetaDataAvailable())
        return value;

    int index = m_availableMetaDatas.indexOf(key);
    if (index < 0)
        return value;

    PROPVARIANT var;
    PropVariantInit(&var);
    HRESULT hr = S_FALSE;
    if (m_content)
        hr = m_content->GetValue(m_commonKeys[index], &var);
    else if (m_metaData)
        hr = m_metaData->GetProperty(reinterpret_cast<LPCWSTR>(m_commonNames[index].utf16()), &var);

    if (SUCCEEDED(hr))
        value = convertValue(var);

    PropVariantClear(&var);
    return value;
}

QVariant MFMetaDataControl::convertValue(const PROPVARIANT& var) const
{
    QVariant value;
    //form MSDN: http://msdn.microsoft.com/en-us/library/ff384862%28v=VS.85%29.aspx
    //it seems that those 4 types are enough for media foundation metadata
    //add more later if necessary
    switch (var.vt) {
    case VT_LPWSTR:
        value = QString::fromUtf16(reinterpret_cast<const ushort*>(var.pwszVal));
        break;
    case VT_UI4:
        value = uint(var.ulVal);
        break;
    case VT_UI8:
        value = qulonglong(var.uhVal.QuadPart);
        break;
    case VT_BOOL:
        value = bool(var.boolVal);
        break;
    }
    return value;
}

QStringList MFMetaDataControl::availableMetaData() const
{
    return m_availableMetaDatas;
}

void MFMetaDataControl::updateSource(IMFPresentationDescriptor* sourcePD, IMFMediaSource* mediaSource)
{
    if (m_metaData) {
        m_metaData->Release();
        m_metaData = 0;
    }

    if (m_content) {
        m_content->Release();
        m_content = 0;
    }

    m_availableMetaDatas.clear();
    m_commonKeys.clear();
    m_commonNames.clear();

    if (SUCCEEDED(MFGetService(mediaSource, MF_PROPERTY_HANDLER_SERVICE, IID_PPV_ARGS(&m_content)))) {
        DWORD cProps;
        if (SUCCEEDED(m_content->GetCount(&cProps))) {
            for (DWORD i = 0; i < cProps; i++)
            {
                PROPERTYKEY key;
                if (FAILED(m_content->GetAt(i, &key)))
                    continue;
                bool common = true;
                if (key == PKEY_Author) {
                    m_availableMetaDatas.push_back(QMediaMetaData::Author);
                } else if (key == PKEY_Title) {
                    m_availableMetaDatas.push_back(QMediaMetaData::Title);
                } else if (key == PKEY_ParentalRating) {
                    m_availableMetaDatas.push_back(QMediaMetaData::ParentalRating);
                } else if (key == PKEY_Comment) {
                    m_availableMetaDatas.push_back(QMediaMetaData::Description);
                } else if (key == PKEY_Copyright) {
                    m_availableMetaDatas.push_back(QMediaMetaData::Copyright);
                //TODO: add more common keys
                } else {
                    common = false;
                    //TODO: add more extended keys
                }
                if (common)
                    m_commonKeys.push_back(key);
            }
        } else {
            m_content->Release();
            m_content = NULL;
        }
    }

    if (!m_content) {
        //fallback to Vista approach
        IMFMetadataProvider *provider = NULL;
        if (SUCCEEDED(MFGetService(mediaSource, MF_METADATA_PROVIDER_SERVICE, IID_PPV_ARGS(&provider)))) {
            if (SUCCEEDED(provider->GetMFMetadata(sourcePD, 0, 0, &m_metaData))) {
                PROPVARIANT varNames;
                PropVariantInit(&varNames);
                if (SUCCEEDED(m_metaData->GetAllPropertyNames(&varNames)) && varNames.vt == (VT_VECTOR | VT_LPWSTR)) {
                    ULONG cElements = varNames.calpwstr.cElems;
                    for (ULONG i = 0; i < cElements; i++)
                    {
                        const WCHAR* sName = varNames.calpwstr.pElems[i];
#ifdef DEBUG_MEDIAFOUNDATION
                        qDebug() << "metadata: " << QString::fromUtf16(sName);
#endif
                        if (wcscmp(sName, L"Author") == 0) {
                            m_availableMetaDatas.push_back(QMediaMetaData::Author);
                        } else if (wcscmp(sName, L"Title") == 0) {
                            m_availableMetaDatas.push_back(QMediaMetaData::Title);
                        } else if (wcscmp(sName, L"Rating") == 0) {
                            m_availableMetaDatas.push_back(QMediaMetaData::ParentalRating);
                        } else if (wcscmp(sName, L"Description") == 0) {
                            m_availableMetaDatas.push_back(QMediaMetaData::Description);
                        } else if (wcscmp(sName, L"Copyright") == 0) {
                            m_availableMetaDatas.push_back(QMediaMetaData::Copyright);
                            //TODO: add more common keys
                        } else {
                            m_availableMetaDatas.push_back(QString::fromUtf16(reinterpret_cast<const ushort*>(sName)));
                        }
                        m_commonNames.push_back(QString::fromUtf16(reinterpret_cast<const ushort*>(sName)));
                    }
                }
                PropVariantClear(&varNames);
            } else {
                qWarning("Failed to get IMFMetadata");
            }
            provider->Release();
        } else {
            qWarning("Failed to get IMFMetadataProvider from source");
        }
    }

    emit metaDataChanged();
    emit metaDataAvailableChanged(m_metaData || m_content);
}
