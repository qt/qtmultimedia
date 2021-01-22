/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QMULTIMEDIA_H
#define QMULTIMEDIA_H

#include <QtCore/qpair.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qstring.h>

#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

namespace QMultimedia
{
    enum SupportEstimate
    {
        NotSupported,
        MaybeSupported,
        ProbablySupported,
        PreferredService
    };

    enum EncodingQuality
    {
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality
    };

    enum EncodingMode
    {
        ConstantQualityEncoding,
        ConstantBitRateEncoding,
        AverageBitRateEncoding,
        TwoPassEncoding
    };

    enum AvailabilityStatus
    {
        Available,
        ServiceMissing,
        Busy,
        ResourceError
    };

}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QMultimedia::AvailabilityStatus)
Q_DECLARE_METATYPE(QMultimedia::SupportEstimate)
Q_DECLARE_METATYPE(QMultimedia::EncodingMode)
Q_DECLARE_METATYPE(QMultimedia::EncodingQuality)


#endif
