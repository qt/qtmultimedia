// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimediautils_p.h"
#include "qvideoframe.h"
#include "qvideoframeformat.h"

#include <QtCore/qdir.h>

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

QSize qRotatedFrameSize(QSize size, int rotation)
{
    Q_ASSERT(rotation % 90 == 0);
    return rotation % 180 ? size.transposed() : size;
}

QSize qRotatedFrameSize(const QVideoFrame &frame)
{
    return qRotatedFrameSize(frame.size(), frame.rotation());
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

static void applyRotation(NormalizedFrameTransformation &transform, int degreesClockwise)
{
    if (degreesClockwise) {
        const int rotationIndex = degreesClockwise / 90;
        transform.rotationIndex += rotationIndex;
        if (transform.xMirrorredAfterRotation && rotationIndex % 2 != 0)
            transform.rotationIndex += 2;
    }
}

static void applyRotation(NormalizedFrameTransformation &transform, QtVideo::Rotation rotation)
{
    applyRotation(transform, qToUnderlying(rotation));
}

static void applyXMirror(NormalizedFrameTransformation &transform, bool mirror)
{
    transform.xMirrorredAfterRotation ^= mirror;
}

static void applyYMirror(NormalizedFrameTransformation &transform, bool mirror)
{
    if (mirror) {
        transform.xMirrorredAfterRotation ^= mirror;
        transform.rotationIndex += 2;
    }
}

static void fixTransformation(NormalizedFrameTransformation &transform)
{
    transform.rotationIndex %= 4;
    if (transform.rotationIndex < 0)
        transform.rotationIndex += 4;
    transform.rotation = QtVideo::Rotation(transform.rotationIndex * 90);
}

static void applySurfaceTransformation(NormalizedFrameTransformation &transform,
                                       const QVideoFrameFormat &format)
{
    applyYMirror(transform, format.scanLineDirection() == QVideoFrameFormat::BottomToTop);
    applyRotation(transform, format.rotation());
    applyXMirror(transform, format.isMirrored());
}

Q_MULTIMEDIA_EXPORT NormalizedFrameTransformation
qNormalizedSurfaceTransformation(const QVideoFrameFormat &format)
{
    NormalizedFrameTransformation result;
    applySurfaceTransformation(result, format);
    fixTransformation(result);
    return result;
}

NormalizedFrameTransformation qNormalizedFrameTransformation(const QVideoFrame &frame,
                                                             int additionalRotaton)
{
    NormalizedFrameTransformation result;
    applySurfaceTransformation(result, frame.surfaceFormat());
    applyRotation(result, frame.rotation());
    applyXMirror(result, frame.mirrored());
    applyRotation(result, additionalRotaton);
    fixTransformation(result);
    return result;
}

QT_END_NAMESPACE
