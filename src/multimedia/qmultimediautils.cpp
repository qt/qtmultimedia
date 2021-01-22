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

#include "qmultimediautils_p.h"

QT_BEGIN_NAMESPACE

void qt_real_to_fraction(qreal value, int *numerator, int *denominator)
{
    if (!numerator || !denominator)
        return;

    const int dMax = 1000;
    int n1 = 0, d1 = 1, n2 = 1, d2 = 1;
    qreal mid = 0.;
    while (d1 <= dMax && d2 <= dMax) {
        mid = qreal(n1 + n2) / (d1 + d2);

        if (qAbs(value - mid) < 0.000001) {
            if (d1 + d2 <= dMax) {
                *numerator = n1 + n2;
                *denominator = d1 + d2;
                return;
            } else if (d2 > d1) {
                *numerator = n2;
                *denominator = d2;
                return;
            } else {
                *numerator = n1;
                *denominator = d1;
                return;
            }
        } else if (value > mid) {
            n1 = n1 + n2;
            d1 = d1 + d2;
        } else {
            n2 = n1 + n2;
            d2 = d1 + d2;
        }
    }

    if (d1 > dMax) {
        *numerator = n2;
        *denominator = d2;
    } else {
        *numerator = n1;
        *denominator = d1;
    }
}

QT_END_NAMESPACE
