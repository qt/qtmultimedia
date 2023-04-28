// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QWASMVIDEOSINK_H
#define QWASMVIDEOSINK_H

#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class QVideoSink;
class QRhi;

class QWasmVideoSink : public QPlatformVideoSink
{
    Q_OBJECT

public:
    explicit QWasmVideoSink(QVideoSink *parent = 0);

    void setRhi(QRhi *) override;

private:
    QRhi *m_rhi = nullptr;
};

QT_END_NAMESPACE

#endif // QWASMVIDEOSINK_H
