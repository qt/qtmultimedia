// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimediautils_p.h"

QT_BEGIN_NAMESPACE

Fraction qRealToFraction(qreal value)
{
    int integral = int(floor(value));
    value -= qreal(integral);
    if (value == 0.)
        return {integral, 1};

    const int dMax = 1000;
    int n1 = 0, d1 = 1, n2 = 1, d2 = 1;
    qreal mid = 0.;
    while (d1 <= dMax && d2 <= dMax) {
        mid = qreal(n1 + n2) / (d1 + d2);

        if (qAbs(value - mid) < 0.000001) {
            break;
        } else if (value > mid) {
            n1 = n1 + n2;
            d1 = d1 + d2;
        } else {
            n2 = n1 + n2;
            d2 = d1 + d2;
        }
    }

    if (d1 + d2 <= dMax)
        return {n1 + n2 + integral * (d1 + d2), d1 + d2};
    else if (d2 < d1)
        return { n2 + integral * d2, d2 };
    else
        return { n1 + integral * d1, d1 };
}

QSize qCalculateFrameSize(QSize resolution, Fraction par)
{
    if (par.numerator == par.denominator || par.numerator < 1 || par.denominator < 1)
        return resolution;

    if (par.numerator > par.denominator)
        return { resolution.width() * par.numerator / par.denominator, resolution.height() };

    return { resolution.width(), resolution.height() * par.denominator / par.numerator };
}

QT_END_NAMESPACE
