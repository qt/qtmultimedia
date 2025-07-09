// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QWFM_SUPPORT_P_H
#define QWFM_SUPPORT_P_H

#include <QtMultimedia/qtmultimediaexports.h>
#include <QtCore/qtconfigmacros.h>
#include <QtCore/qspan.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/private/qcomptr_p.h>

#include <mfobjects.h>

QT_BEGIN_NAMESPACE

namespace QWMF {

template <typename Functor>
using IMFBufferReaderReturnType =
        decltype(std::declval<Functor>()(std::declval<QSpan<BYTE>>(), std::declval<QSpan<BYTE>>()));

template <typename Functor>
[[nodiscard]]
auto withLockedBuffer(IMFMediaBuffer *buffer, Functor &&f)
        -> q23::expected<IMFBufferReaderReturnType<Functor>, HRESULT>
{
    if (!buffer)
        return q23::unexpected{ E_POINTER };

    BYTE *data = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;

    HRESULT hr = buffer->Lock(&data, &maxLength, &currentLength);
    if (FAILED(hr))
        return q23::unexpected{ hr };

    auto unlockGuard = qScopeGuard([buffer]() {
        buffer->Unlock();
    });

    if constexpr (std::is_void_v<IMFBufferReaderReturnType<Functor>>) {
        f(QSpan{ data, currentLength }, QSpan{ data, maxLength });
        return {};
    } else
        return f(QSpan{ data, currentLength }, QSpan{ data, maxLength });
}

template <typename Functor>
[[nodiscard]]
auto withLockedBuffer(const ComPtr<IMFMediaBuffer> &buffer, Functor &&f)
        -> q23::expected<IMFBufferReaderReturnType<Functor>, HRESULT>
{
    return withLockedBuffer(buffer.Get(), std::forward<Functor>(f));
}

} // namespace QWMF

QT_END_NAMESPACE

#endif // QWFM_SUPPORT_P_H
