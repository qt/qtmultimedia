// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimediautils_p.h"
#include "qvideoframe.h"
#include "qvideoframeformat.h"

#include <QtCore/qdir.h>
#include <QtCore/qloggingcategory.h>

#include <cmath>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcMultimediaUtils, "qt.multimedia.utils");

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

QSize qRotatedFrameSize(QSize size, int rotation)
{
    Q_ASSERT(rotation % 90 == 0);
    return rotation % 180 ? size.transposed() : size;
}

QSize qRotatedFramePresentationSize(const QVideoFrame &frame)
{
    // For mirrored frames the rotation can be +/- 180 degrees,
    // but this inaccuracy doesn't impact on the result.
    const int rotation = qToUnderlying(frame.rotation()) + qToUnderlying(frame.surfaceFormat().rotation());
    return qRotatedFrameSize(frame.size(), rotation);
}

QUrl qMediaFromUserInput(QUrl url)
{
    return QUrl::fromUserInput(url.toString(), QDir::currentPath(), QUrl::AssumeLocalFile);
}

bool qIsAutoHdrEnabled()
{
    static const bool autoHdrEnabled = qEnvironmentVariableIntValue("QT_MEDIA_AUTO_HDR");

    return autoHdrEnabled;
}

QRhiSwapChain::Format qGetRequiredSwapChainFormat(const QVideoFrameFormat &format)
{
    constexpr auto sdrMaxLuminance = 100.0f;
    const auto formatMaxLuminance = format.maxLuminance();

    return formatMaxLuminance > sdrMaxLuminance ? QRhiSwapChain::HDRExtendedSrgbLinear
                                                : QRhiSwapChain::SDR;
}

bool qShouldUpdateSwapChainFormat(QRhiSwapChain *swapChain,
                                  QRhiSwapChain::Format requiredSwapChainFormat)
{
    if (!swapChain)
        return false;

    return qIsAutoHdrEnabled() && swapChain->format() != requiredSwapChainFormat
            && swapChain->isFormatSupported(requiredSwapChainFormat);
}

static void applyRotation(NormalizedVideoTransformation &transform, int degreesClockwise)
{
    if (degreesClockwise) {
        const int rotationIndex = degreesClockwise / 90;
        transform.rotationIndex += rotationIndex;
        if (transform.xMirrorredAfterRotation && rotationIndex % 2 != 0)
            transform.rotationIndex += 2;
    }
}

static void applyRotation(NormalizedVideoTransformation &transform, QtVideo::Rotation rotation)
{
    applyRotation(transform, qToUnderlying(rotation));
}

static void applyXMirror(NormalizedVideoTransformation &transform, bool mirror)
{
    transform.xMirrorredAfterRotation ^= mirror;
}

static void applyYMirror(NormalizedVideoTransformation &transform, bool mirror)
{
    if (mirror) {
        transform.xMirrorredAfterRotation ^= mirror;
        transform.rotationIndex += 2;
    }
}

static void fixTransformation(NormalizedVideoTransformation &transform)
{
    transform.rotationIndex %= 4;
    if (transform.rotationIndex < 0)
        transform.rotationIndex += 4;
    transform.rotation = QtVideo::Rotation(transform.rotationIndex * 90);
}

static void applySurfaceTransformation(NormalizedVideoTransformation &transform,
                                       const QVideoFrameFormat &format)
{
    applyYMirror(transform, format.scanLineDirection() == QVideoFrameFormat::BottomToTop);
    applyRotation(transform, format.rotation());
    applyXMirror(transform, format.isMirrored());
}

Q_MULTIMEDIA_EXPORT NormalizedVideoTransformation
qNormalizedSurfaceTransformation(const QVideoFrameFormat &format)
{
    NormalizedVideoTransformation result;
    applySurfaceTransformation(result, format);
    fixTransformation(result);
    return result;
}

NormalizedVideoTransformation qNormalizedFrameTransformation(const QVideoFrame &frame,
                                                             int additionalRotaton)
{
    NormalizedVideoTransformation result;
    applySurfaceTransformation(result, frame.surfaceFormat());
    applyRotation(result, frame.rotation());
    applyXMirror(result, frame.mirrored());
    applyRotation(result, additionalRotaton);
    fixTransformation(result);
    return result;
}

// Only accepts inputs divisible by 90.
// Invalid input returns no rotation.
QtVideo::Rotation qVideoRotationFromDegrees(int clockwiseDegrees)
{
    if (clockwiseDegrees % 90 != 0) {
        qCWarning(qLcMultimediaUtils) << "qVideoRotationFromAngle(int) received "
                                         "input not divisible by 90. Input was: "
                                      << clockwiseDegrees;
        return QtVideo::Rotation::None;
    }

    int newDegrees = clockwiseDegrees % 360;
    // Adjust negative rotations into positive ones.
    if (newDegrees < 0)
        newDegrees += 360;
    return static_cast<QtVideo::Rotation>(newDegrees);
}

NormalizedVideoTransformationOpt qVideoTransformationFromMatrix(const QTransform &matrix)
{
    const qreal absScaleX = std::hypot(matrix.m11(), matrix.m12());
    const qreal absScaleY = std::hypot(matrix.m21(), matrix.m22());

    if (qFuzzyIsNull(absScaleX) || qFuzzyIsNull(absScaleY))
        return {}; // the matrix is malformed

    qreal cos1 = matrix.m11() / absScaleX;
    qreal sin1 = matrix.m12() / absScaleX;

    // const: consider yScale > 0, as negative yScale can be compensated via negative xScale
    const qreal sin2 = -matrix.m21() / absScaleY;
    const qreal cos2 = matrix.m22() / absScaleY;

    NormalizedVideoTransformation result;

    // try detecting the best pair option to detect mirroring

    if (std::abs(cos1) + std::abs(cos2) > std::abs(sin1) + std::abs(sin2))
        result.xMirrorredAfterRotation = std::signbit(cos1) != std::signbit(cos2);
    else
        result.xMirrorredAfterRotation = std::signbit(sin1) != std::signbit(sin2);

    if (result.xMirrorredAfterRotation) {
        cos1 *= -1;
        sin1 *= -1;
    }

    const qreal maxDiscrepancy = 0.2;

    if (std::abs(cos1 - cos2) > maxDiscrepancy || std::abs(sin1 - sin2) > maxDiscrepancy)
        return {}; // the matrix is sheared too much, this is not supported currently

    const qreal angle = atan2(sin1 + sin2, cos1 + cos2);
    Q_ASSERT(!std::isnan(angle)); // checked upon scale validation

    result.rotationIndex = qRound(angle / M_PI_2);
    fixTransformation(result);
    return result;
}

QT_END_NAMESPACE
