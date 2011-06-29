/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmfactivate.h"

#include <QtCore/qglobal.h>

#include <Mferror.h>


// IMFAttributes

HRESULT QMFActivate::GetItem(REFGUID guidKey, PROPVARIANT *pValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pValue);

    return MF_E_ATTRIBUTENOTFOUND;
}

HRESULT QMFActivate::GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE *pType)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pType);

    return MF_E_ATTRIBUTENOTFOUND;
}

HRESULT QMFActivate::CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL *pbResult)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(Value);
    Q_UNUSED(pbResult);

    return E_NOTIMPL;
}

HRESULT QMFActivate::Compare(IMFAttributes *pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL *pbResult)
{
    Q_UNUSED(pTheirs);
    Q_UNUSED(MatchType);
    Q_UNUSED(pbResult);

    return E_NOTIMPL;
}

HRESULT QMFActivate::GetUINT32(REFGUID guidKey, UINT32 *punValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(punValue);

    return MF_E_ATTRIBUTENOTFOUND;
}

HRESULT QMFActivate::GetUINT64(REFGUID guidKey, UINT64 *punValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(punValue);

    return MF_E_ATTRIBUTENOTFOUND;
}

HRESULT QMFActivate::GetDouble(REFGUID guidKey, double *pfValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pfValue);

    return MF_E_ATTRIBUTENOTFOUND;
}

HRESULT QMFActivate::GetGUID(REFGUID guidKey, GUID *pguidValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pguidValue);

    return MF_E_ATTRIBUTENOTFOUND;
}

HRESULT QMFActivate::GetStringLength(REFGUID guidKey, UINT32 *pcchLength)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pcchLength);

    return MF_E_ATTRIBUTENOTFOUND; 
}

HRESULT QMFActivate::GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32 *pcchLength)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pwszValue);
    Q_UNUSED(cchBufSize);
    Q_UNUSED(pcchLength);

    return MF_E_ATTRIBUTENOTFOUND; 
}

HRESULT QMFActivate::GetAllocatedString(REFGUID guidKey, LPWSTR *ppwszValue, UINT32 *pcchLength)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(ppwszValue);
    Q_UNUSED(pcchLength);

    return MF_E_ATTRIBUTENOTFOUND; 
}

HRESULT QMFActivate::GetBlobSize(REFGUID guidKey, UINT32 *pcbBlobSize)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pcbBlobSize);

    return MF_E_ATTRIBUTENOTFOUND; 
}

HRESULT QMFActivate::GetBlob(REFGUID guidKey, UINT8 *pBuf, UINT32 cbBufSize, UINT32 *pcbBlobSize)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pBuf);
    Q_UNUSED(cbBufSize);
    Q_UNUSED(pcbBlobSize);

    return MF_E_ATTRIBUTENOTFOUND; 
}

HRESULT QMFActivate::GetAllocatedBlob(REFGUID guidKey, UINT8 **ppBuf, UINT32 *pcbSize)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(ppBuf);
    Q_UNUSED(pcbSize);

    return MF_E_ATTRIBUTENOTFOUND; 
}

HRESULT QMFActivate::GetUnknown(REFGUID guidKey, REFIID riid, LPVOID *ppv)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(riid);
    Q_UNUSED(ppv);

    return MF_E_ATTRIBUTENOTFOUND; 
}

HRESULT QMFActivate::SetItem(REFGUID guidKey, REFPROPVARIANT Value)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(Value);

    return E_NOTIMPL;
}

HRESULT QMFActivate::DeleteItem(REFGUID guidKey)
{
    Q_UNUSED(guidKey);

    return E_NOTIMPL; 
}

HRESULT QMFActivate::DeleteAllItems()
{
    return E_NOTIMPL; 
}

HRESULT QMFActivate::SetUINT32(REFGUID guidKey, UINT32 unValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(unValue);

    return E_NOTIMPL;
}

HRESULT QMFActivate::SetUINT64(REFGUID guidKey, UINT64 unValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(unValue);

    return E_NOTIMPL; 
}

HRESULT QMFActivate::SetDouble(REFGUID guidKey, double fValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(fValue);

    return E_NOTIMPL; 
}

HRESULT QMFActivate::SetGUID(REFGUID guidKey, REFGUID guidValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(guidValue);

    return E_NOTIMPL; 
}

HRESULT QMFActivate::SetString(REFGUID guidKey, LPCWSTR wszValue)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(wszValue);

    return E_NOTIMPL; 
}

HRESULT QMFActivate::SetBlob(REFGUID guidKey, const UINT8 *pBuf, UINT32 cbBufSize)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pBuf);
    Q_UNUSED(cbBufSize);

    return E_NOTIMPL; 
}

HRESULT QMFActivate::SetUnknown(REFGUID guidKey, IUnknown *pUnknown)
{
    Q_UNUSED(guidKey);
    Q_UNUSED(pUnknown);

    return E_NOTIMPL; 
}

HRESULT QMFActivate::LockStore()
{
    m_mutex.lock();

    return S_OK;
}

HRESULT QMFActivate::UnlockStore()
{
    m_mutex.unlock();

    return S_OK;
}

HRESULT QMFActivate::GetCount(UINT32 *pcItems)
{
    if (!pcItems) {
        return E_POINTER;
    } else {
        *pcItems = 0;

        return S_OK;
    }
}

HRESULT QMFActivate::GetItemByIndex(UINT32 unIndex, GUID *pguidKey, PROPVARIANT *pValue)
{
    Q_UNUSED(unIndex);
    Q_UNUSED(pguidKey);
    Q_UNUSED(pValue);

    return MF_E_ATTRIBUTENOTFOUND; 
}

HRESULT QMFActivate::CopyAllItems(IMFAttributes *pDest)
{
    Q_UNUSED(pDest);

    return MF_E_ATTRIBUTENOTFOUND; 
}
