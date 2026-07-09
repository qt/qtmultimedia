// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimediautils_p.h"

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/quuid.h>

#include <cmath>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals;

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

QUrl qMediaFromUserInput(const QUrl &url)
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

Q_MULTIMEDIA_EXPORT VideoTransformation
qNormalizedSurfaceTransformation(const QVideoFrameFormat &format)
{
    VideoTransformation result;
    result.mirrorVertically(format.scanLineDirection() == QVideoFrameFormat::BottomToTop);
    result.rotate(format.rotation());
    result.mirrorHorizontally(format.isMirrored());
    return result;
}

VideoTransformation qNormalizedFrameTransformation(const QVideoFrame &frame,
                                                   VideoTransformation videoOutputTransformation)
{
    VideoTransformation result = qNormalizedSurfaceTransformation(frame.surfaceFormat());
    result.rotate(frame.rotation());
    result.mirrorHorizontally(frame.mirrored());
    result.rotate(videoOutputTransformation.rotation);
    result.mirrorHorizontally(videoOutputTransformation.mirroredHorizontallyAfterRotation);
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

VideoTransformationOpt qVideoTransformationFromMatrix(const QTransform &matrix)
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

    VideoTransformation result;

    // try detecting the best pair option to detect mirroring

    if (std::abs(cos1) + std::abs(cos2) > std::abs(sin1) + std::abs(sin2))
        result.mirroredHorizontallyAfterRotation = std::signbit(cos1) != std::signbit(cos2);
    else
        result.mirroredHorizontallyAfterRotation = std::signbit(sin1) != std::signbit(sin2);

    if (result.mirroredHorizontallyAfterRotation) {
        cos1 *= -1;
        sin1 *= -1;
    }

    const qreal maxDiscrepancy = 0.2;

    if (std::abs(cos1 - cos2) > maxDiscrepancy || std::abs(sin1 - sin2) > maxDiscrepancy)
        return {}; // the matrix is sheared too much, this is not supported currently

    const qreal angle = atan2(sin1 + sin2, cos1 + cos2);
    Q_ASSERT(!std::isnan(angle)); // checked upon scale validation

    result.rotation = qVideoRotationFromDegrees(qRound(angle / M_PI_2) * 90);
    return result;
}

namespace QtMultimediaPrivate {
q23::expected<QrcMedia, QString> qCopyQrcToTemporaryFile([[maybe_unused]] QFile &qrcFile,
                                                          [[maybe_unused]] const QUrl &qrcUrl)
{
#if QT_CONFIG(temporaryfile)
    std::unique_ptr<QTemporaryFile> tempFile;

    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Android) {
        tempFile = std::unique_ptr<QTemporaryFile>(QTemporaryFile::createNativeFile(qrcFile));
        if (!tempFile)
            return q23::unexpected(u"Failed to establish temporary file during playback"_s);

        // Use a temp path derived from the original resource path
        const QFileInfo mediaInfo(qrcUrl.path());
        const QString targetDirPath = QDir::tempPath() + mediaInfo.path();
        if (!QDir().mkpath(targetDirPath))
            return q23::unexpected(
                    u"Could not create a temporary directory: %1"_s.arg(targetDirPath));

        // Add a random suffix to avoid collisions
        const QString baseName = mediaInfo.completeBaseName() + QLatin1Char('_')
                + QUuid::createUuid().toString(QUuid::WithoutBraces);

        // Keep extension suffix
        const QString suffix = mediaInfo.suffix();

        const QString newName = targetDirPath + QLatin1Char('/') + baseName
                + (suffix.isEmpty() ? QString() : QLatin1Char('.') + suffix);

        if (!tempFile->rename(newName))
            return q23::unexpected(u"Could not rename temporary file to: %1"_s.arg(newName));
    } else {
        tempFile = std::make_unique<QTemporaryFile>();

        // Preserve original file extension, some back ends might not load the file if it doesn't
        // have an extension.
        const QString suffix = QFileInfo(qrcFile).suffix();
        if (!suffix.isEmpty())
            tempFile->setFileTemplate(tempFile->fileTemplate() + QLatin1Char('.') + suffix);

        if (!tempFile->open())
            return q23::unexpected(tempFile->errorString());

        // Copy the qrc data into the temporary file
        char buffer[4096];
        while (true) {
            qint64 len = qrcFile.read(buffer, sizeof(buffer));
            if (len < 1)
                break;
            tempFile->write(buffer, len);
        }
        tempFile->close();
    }

    QUrl url = QUrl::fromLocalFile(tempFile->fileName());
    return QrcMedia{
        std::move(url),
        std::move(tempFile),
    };
#else
    return q23::unexpected(u"Qt was built with -no-feature-temporaryfile: playback from "
                           "resource file is not supported!"_s);
#endif
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
