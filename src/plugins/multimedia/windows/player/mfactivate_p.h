// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef MFACTIVATE_H
#define MFACTIVATE_H

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

#include <mfidl.h>
#include <private/qcomobject_p.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template <>
struct QComObjectTraits<IMFActivate>
{
    static constexpr bool isGuidOf(REFIID riid) noexcept
    {
        return QComObjectTraits<IMFActivate, IMFAttributes>::isGuidOf(riid);
    }
};

} // namespace QtPrivate

class MFAbstractActivate : public QComObject<IMFActivate>
{
public:
    explicit MFAbstractActivate();

    //from IMFAttributes
    STDMETHODIMP GetItem(REFGUID guidKey, PROPVARIANT *pValue) override
    {
        return m_attributes->GetItem(guidKey, pValue);
    }

    STDMETHODIMP GetItemType(REFGUID guidKey, MF_ATTRIBUTE_TYPE *pType) override
    {
        return m_attributes->GetItemType(guidKey, pType);
    }

    STDMETHODIMP CompareItem(REFGUID guidKey, REFPROPVARIANT Value, BOOL *pbResult) override
    {
        return m_attributes->CompareItem(guidKey, Value, pbResult);
    }

    STDMETHODIMP Compare(IMFAttributes *pTheirs, MF_ATTRIBUTES_MATCH_TYPE MatchType, BOOL *pbResult) override
    {
        return m_attributes->Compare(pTheirs, MatchType, pbResult);
    }

    STDMETHODIMP GetUINT32(REFGUID guidKey, UINT32 *punValue) override
    {
        return m_attributes->GetUINT32(guidKey, punValue);
    }

    STDMETHODIMP GetUINT64(REFGUID guidKey, UINT64 *punValue) override
    {
        return m_attributes->GetUINT64(guidKey, punValue);
    }

    STDMETHODIMP GetDouble(REFGUID guidKey, double *pfValue) override
    {
        return m_attributes->GetDouble(guidKey, pfValue);
    }

    STDMETHODIMP GetGUID(REFGUID guidKey, GUID *pguidValue) override
    {
        return m_attributes->GetGUID(guidKey, pguidValue);
    }

    STDMETHODIMP GetStringLength(REFGUID guidKey, UINT32 *pcchLength) override
    {
        return m_attributes->GetStringLength(guidKey, pcchLength);
    }

    STDMETHODIMP GetString(REFGUID guidKey, LPWSTR pwszValue, UINT32 cchBufSize, UINT32 *pcchLength) override
    {
        return m_attributes->GetString(guidKey, pwszValue, cchBufSize, pcchLength);
    }

    STDMETHODIMP GetAllocatedString(REFGUID guidKey, LPWSTR *ppwszValue, UINT32 *pcchLength) override
    {
        return m_attributes->GetAllocatedString(guidKey, ppwszValue, pcchLength);
    }

    STDMETHODIMP GetBlobSize(REFGUID guidKey, UINT32 *pcbBlobSize) override
    {
        return m_attributes->GetBlobSize(guidKey, pcbBlobSize);
    }

    STDMETHODIMP GetBlob(REFGUID guidKey, UINT8 *pBuf, UINT32 cbBufSize, UINT32 *pcbBlobSize) override
    {
        return m_attributes->GetBlob(guidKey, pBuf, cbBufSize, pcbBlobSize);
    }

    STDMETHODIMP GetAllocatedBlob(REFGUID guidKey, UINT8 **ppBuf, UINT32 *pcbSize) override
    {
        return m_attributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
    }

    STDMETHODIMP GetUnknown(REFGUID guidKey, REFIID riid, LPVOID *ppv) override
    {
        return m_attributes->GetUnknown(guidKey, riid, ppv);
    }

    STDMETHODIMP SetItem(REFGUID guidKey, REFPROPVARIANT Value) override
    {
        return m_attributes->SetItem(guidKey, Value);
    }

    STDMETHODIMP DeleteItem(REFGUID guidKey) override
    {
        return m_attributes->DeleteItem(guidKey);
    }

    STDMETHODIMP DeleteAllItems() override
    {
        return m_attributes->DeleteAllItems();
    }

    STDMETHODIMP SetUINT32(REFGUID guidKey, UINT32 unValue) override
    {
        return m_attributes->SetUINT32(guidKey, unValue);
    }

    STDMETHODIMP SetUINT64(REFGUID guidKey, UINT64 unValue) override
    {
        return m_attributes->SetUINT64(guidKey, unValue);
    }

    STDMETHODIMP SetDouble(REFGUID guidKey, double fValue) override
     {
        return m_attributes->SetDouble(guidKey, fValue);
    }

    STDMETHODIMP SetGUID(REFGUID guidKey, REFGUID guidValue) override
    {
        return m_attributes->SetGUID(guidKey, guidValue);
    }

    STDMETHODIMP SetString(REFGUID guidKey, LPCWSTR wszValue) override
    {
        return m_attributes->SetString(guidKey, wszValue);
    }

    STDMETHODIMP SetBlob(REFGUID guidKey, const UINT8 *pBuf, UINT32 cbBufSize) override
    {
        return m_attributes->SetBlob(guidKey, pBuf, cbBufSize);
    }

    STDMETHODIMP SetUnknown(REFGUID guidKey, IUnknown *pUnknown) override
    {
        return m_attributes->SetUnknown(guidKey, pUnknown);
    }

    STDMETHODIMP LockStore() override
    {
        return m_attributes->LockStore();
    }

    STDMETHODIMP UnlockStore() override
    {
        return m_attributes->UnlockStore();
    }

    STDMETHODIMP GetCount(UINT32 *pcItems) override
    {
        return m_attributes->GetCount(pcItems);
    }

    STDMETHODIMP GetItemByIndex(UINT32 unIndex, GUID *pguidKey, PROPVARIANT *pValue) override
    {
        return m_attributes->GetItemByIndex(unIndex, pguidKey, pValue);
    }

    STDMETHODIMP CopyAllItems(IMFAttributes *pDest) override
    {
        return m_attributes->CopyAllItems(pDest);
    }

protected:
    // Destructor is not public. Caller should call Release.
    ~MFAbstractActivate() override;

private:
    IMFAttributes *m_attributes = nullptr;
};

QT_END_NAMESPACE

#endif // MFACTIVATE_H
