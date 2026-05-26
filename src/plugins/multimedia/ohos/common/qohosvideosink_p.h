// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSVIDEOSINK_P_H
#define QOHOSVIDEOSINK_P_H

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

#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class QRhi;

class QOhosVideoSink : public QPlatformVideoSink
{
    Q_OBJECT
public:
    explicit QOhosVideoSink(QVideoSink *parent);
    ~QOhosVideoSink() override;

    void setRhi(QRhi *rhi) override;

    QRhi *rhi() const { return m_rhi; }

private:
    QRhi *m_rhi{ nullptr };
};

QT_END_NAMESPACE

#endif // QOHOSVIDEOSINK_P_H
