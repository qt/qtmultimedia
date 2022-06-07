// Copyright (C) 2016 Research In Motion
// Copyright (C) 2021 The Qt Company
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQNXVIDFEOSINK_P_H
#define QQNXVIDFEOSINK_P_H

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

#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class QQnxWindowGrabber;
class QVideoSink;

class QQnxVideoSink : public QPlatformVideoSink
{
    Q_OBJECT
public:
    explicit QQnxVideoSink(QVideoSink *parent = 0);

    void setRhi(QRhi *) override;

    QRhi *rhi() const;

private:
    QRhi *m_rhi = nullptr;
};

QT_END_NAMESPACE

#endif
