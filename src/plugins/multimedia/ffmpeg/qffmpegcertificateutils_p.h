// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGCERTIFICATEUTILS_P_H
#define QFFMPEGCERTIFICATEUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qbytearrayview.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// Converts a single DER-encoded X.509 certificate into a PEM-encoded certificate block.
QByteArray derToPem(QByteArrayView der);

// Concatenates DER-encoded X.509 certificates into a single multi-certificate PEM bundle,
// suitable for use as a CA file (e.g. OpenSSL's SSL_CTX_load_verify_locations).
QByteArray certificatesToPemBundle(const QList<QByteArray> &derCertificates);

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QFFMPEGCERTIFICATEUTILS_P_H
