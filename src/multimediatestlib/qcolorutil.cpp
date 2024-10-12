// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qcolorutil_p.h"

#include <QtGui/qcolor.h>
#include <QtGui/qvector3d.h>
#include <cmath>

QT_BEGIN_NAMESPACE

namespace {

// Poor man's RGB to YUV conversion with BT.709 coefficients
// from https://en.wikipedia.org/wiki/Y%E2%80%B2UV
QVector3D RGBToYUV(const QColor &c)
{
    const float R = c.redF();
    const float G = c.greenF();
    const float B = c.blueF();
    QVector3D yuv;
    yuv[0] = 0.2126f * R + 0.7152f * G + 0.0722f * B;
    yuv[1] = -0.09991f * R - 0.33609f * G + 0.436f * B;
    yuv[2] = 0.615f * R - 0.55861f * G - 0.05639f * B;
    return yuv;
}

} // namespace

bool fuzzyCompare(const QColor &lhs, const QColor &rhs, float tol)
{
    const QVector3D lhsYuv = RGBToYUV(lhs);
    const QVector3D rhsYuv = RGBToYUV(rhs);
    const float relativeLumaDiff =
            0.5f * std::abs((lhsYuv[0] - rhsYuv[0]) / (lhsYuv[0] + rhsYuv[0]));
    const float colorDiff = QVector3D::crossProduct(lhsYuv, rhsYuv).length();
    return colorDiff < tol && relativeLumaDiff < tol;
}

QT_END_NAMESPACE
