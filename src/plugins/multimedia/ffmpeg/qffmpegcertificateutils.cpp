// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegcertificateutils_p.h"

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

QByteArray derToPem(QByteArrayView der)
{
    const QByteArray base64 = QByteArray(der.data(), der.size()).toBase64();

    constexpr qsizetype lineLength = 64;

    QByteArray pem = "-----BEGIN CERTIFICATE-----\n";
    for (qsizetype pos = 0; pos < base64.size(); pos += lineLength) {
        pem += base64.sliced(pos, std::min(lineLength, base64.size() - pos));
        pem += '\n';
    }
    pem += "-----END CERTIFICATE-----\n";

    return pem;
}

QByteArray certificatesToPemBundle(const QList<QByteArray> &derCertificates)
{
    QByteArray bundle;
    for (const QByteArray &der : derCertificates)
        bundle += derToPem(der);

    return bundle;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
