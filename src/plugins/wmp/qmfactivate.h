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

#ifndef QMFACTIVATE_H
#define QMFACTIVATE_H

#include <QtCore/qmutex.h>

#include <evr.h>

class QMFActivate : public IMFActivate
{
public:
    // IMFAttributes
    HRESULT STDMETHODCALLTYPE GetItem(REFGUID guidKey, PROPVARIANT *pValue);
    HRESULT STDMETHODCALLTYPE GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE *pType);
    HRESULT STDMETHODCALLTYPE CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL *pbResult);
    HRESULT STDMETHODCALLTYPE Compare(IMFAttributes *pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL *pbResult);
    HRESULT STDMETHODCALLTYPE GetUINT32(REFGUID guidKey, UINT32 *punValue);
    HRESULT STDMETHODCALLTYPE GetUINT64(REFGUID guidKey, UINT64 *punValue);
    HRESULT STDMETHODCALLTYPE GetDouble(REFGUID guidKey, double *pfValue);
    HRESULT STDMETHODCALLTYPE GetGUID(REFGUID guidKey, GUID *pguidValue);
    HRESULT STDMETHODCALLTYPE GetStringLength(REFGUID guidKey, UINT32 *pcchLength);
    HRESULT STDMETHODCALLTYPE GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32 *pcchLength);
    HRESULT STDMETHODCALLTYPE GetAllocatedString(REFGUID guidKey, LPWSTR *ppwszValue, UINT32 *pcchLength);
    HRESULT STDMETHODCALLTYPE GetBlobSize(REFGUID guidKey, UINT32 *pcbBlobSize);
    HRESULT STDMETHODCALLTYPE GetBlob(REFGUID guidKey, UINT8 *pBuf, UINT32 cbBufSize, UINT32 *pcbBlobSize);
    HRESULT STDMETHODCALLTYPE GetAllocatedBlob(REFGUID guidKey, UINT8 **ppBuf, UINT32 *pcbSize);
    HRESULT STDMETHODCALLTYPE GetUnknown(REFGUID guidKey, REFIID riid, LPVOID *ppv);
    HRESULT STDMETHODCALLTYPE SetItem(REFGUID guidKey, REFPROPVARIANT Value);
    HRESULT STDMETHODCALLTYPE DeleteItem(REFGUID guidKey);
    HRESULT STDMETHODCALLTYPE DeleteAllItems();
    HRESULT STDMETHODCALLTYPE SetUINT32(REFGUID guidKey, UINT32 unValue);
    HRESULT STDMETHODCALLTYPE SetUINT64(REFGUID guidKey, UINT64 unValue);
    HRESULT STDMETHODCALLTYPE SetDouble(REFGUID guidKey, double fValue);
    HRESULT STDMETHODCALLTYPE SetGUID(REFGUID guidKey, REFGUID guidValue);
    HRESULT STDMETHODCALLTYPE SetString(REFGUID guidKey, LPCWSTR wszValue);
    HRESULT STDMETHODCALLTYPE SetBlob(REFGUID guidKey, const UINT8 *pBuf, UINT32 cbBufSize);
    HRESULT STDMETHODCALLTYPE SetUnknown(REFGUID guidKey, IUnknown *pUnknown);
    HRESULT STDMETHODCALLTYPE LockStore();
    HRESULT STDMETHODCALLTYPE UnlockStore();
    HRESULT STDMETHODCALLTYPE GetCount(UINT32 *pcItems);
    HRESULT STDMETHODCALLTYPE GetItemByIndex(UINT32 unIndex, GUID *pguidKey, PROPVARIANT *pValue);
    HRESULT STDMETHODCALLTYPE CopyAllItems(IMFAttributes *pDest);

private:
    volatile LONG m_ref;
    QMutex m_mutex;
};


#endif
