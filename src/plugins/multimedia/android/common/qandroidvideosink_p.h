// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDVIDEOSINK_P_H
#define QANDROIDVIDEOSINK_P_H

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

#include <private/qtmultimediaglobal_p.h>
#include <private/qplatformvideosink_p.h>

#include <qvideosink.h>

QT_BEGIN_NAMESPACE

class QAndroidVideoSink
    : public QPlatformVideoSink
{
    Q_OBJECT
public:
    explicit QAndroidVideoSink(QVideoSink *parent = 0);
    ~QAndroidVideoSink();

    void setRhi(QRhi *rhi) override;

private:
    QRhi *m_rhi = nullptr;
};

QT_END_NAMESPACE

#endif
