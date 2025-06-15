// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindows_propertystore_p.h"

#include <QtCore/qscopeguard.h>
#include <QtCore/qstring.h>
#include <QtCore/qdebug.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtMultimedia/private/qcomtaskresource_p.h>

#include <mmdeviceapi.h>
#include <propsys.h>
#include <propvarutil.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

PropertyStoreHelper::PropertyStoreHelper(ComPtr<IPropertyStore> props) : m_props(std::move(props))
{
}

q23::expected<PropertyStoreHelper, QString>
PropertyStoreHelper::open(const ComPtr<IMMDevice> &device)
{
    ComPtr<IPropertyStore> props;
    HRESULT hr = device->OpenPropertyStore(STGM_READ, props.GetAddressOf());
    if (!SUCCEEDED(hr)) {
        return q23::unexpected{ QSystemError::windowsComString(hr) };
    }
    return PropertyStoreHelper(std::move(props));
}

std::optional<QString> PropertyStoreHelper::getString(const PROPERTYKEY &property)
{
    PROPVARIANT variant;
    PropVariantInit(&variant);

    auto cleanup = qScopeGuard([&] {
        PropVariantClear(&variant);
    });

    if (!SUCCEEDED(m_props->GetValue(property, &variant)))
        return std::nullopt;

    QComTaskResource<WCHAR> str;
    HRESULT hr = PropVariantToStringAlloc(variant, str.address());
    if (SUCCEEDED(hr))
        return QString::fromWCharArray(variant.pwszVal);

    qWarning() << "PropertyStoreHelper::getString: PropVariantToStringAlloc failed"
               << QSystemError::windowsComString(hr);
    return std::nullopt;
}

std::optional<uint32_t> PropertyStoreHelper::getUInt32(const PROPERTYKEY &property)
{
    PROPVARIANT variant;
    PropVariantInit(&variant);

    if (!SUCCEEDED(m_props->GetValue(property, &variant)))
        return std::nullopt;

    ULONG ret;
    HRESULT hr = PropVariantToUInt32(variant, &ret);
    if (SUCCEEDED(hr))
        return uint32_t{ ret };

    qWarning() << "PropertyStoreHelper::getUInt32: PropVariantToUInt32 failed"
               << QSystemError::windowsComString(hr);
    return std::nullopt;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
