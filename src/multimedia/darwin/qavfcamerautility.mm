// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qavfcamerautility_p.h>
#include <QtMultimedia/private/qavfcameradebug_p.h>

#include <QtCore/qvector.h>
#include <private/qmultimediautils_p.h>
#include <private/qcameradevice_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>

#include <functional>
#include <algorithm>
#include <limits>
#include <tuple>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcCamera, "qt.multimedia.camera")

AVFPSRange qt_connection_framerates(AVCaptureConnection *videoConnection)
{
    Q_ASSERT(videoConnection);

    AVFPSRange newRange;
    // "The value in the videoMinFrameDuration is equivalent to the reciprocal
    // of the maximum framerate, the value in the videoMaxFrameDuration is equivalent
    // to the reciprocal of the minimum framerate."
    if (videoConnection.supportsVideoMinFrameDuration) {
        const CMTime cmMin = videoConnection.videoMinFrameDuration;
        if (CMTimeCompare(cmMin, kCMTimeInvalid)) { // Has some non-default value:
            if (const Float64 minSeconds = CMTimeGetSeconds(cmMin))
                newRange.second = 1. / minSeconds;
        }
    }

    if (videoConnection.supportsVideoMaxFrameDuration) {
        const CMTime cmMax = videoConnection.videoMaxFrameDuration;
        if (CMTimeCompare(cmMax, kCMTimeInvalid)) {
            if (const Float64 maxSeconds = CMTimeGetSeconds(cmMax))
                newRange.first = 1. / maxSeconds;
        }
    }

    return newRange;
}

namespace {

inline bool qt_area_sane(const QSize &size)
{
    return !size.isNull() && size.isValid()
           && std::numeric_limits<int>::max() / size.width() >= size.height();
}

template <template <typename...> class Comp> // std::less or std::greater (or std::equal_to)
struct ByResolution
{
    bool operator() (AVCaptureDeviceFormat *f1, AVCaptureDeviceFormat *f2)const
    {
        Q_ASSERT(f1 && f2);
        const QSize r1(qt_device_format_resolution(f1));
        const QSize r2(qt_device_format_resolution(f2));
        // use std::tuple for lexicograpical sorting:
        const Comp<std::tuple<int, int>> op = {};
        return op(std::make_tuple(r1.width(), r1.height()),
                  std::make_tuple(r2.width(), r2.height()));
    }
};

struct FormatHasNoFPSRange
{
    bool operator() (AVCaptureDeviceFormat *format) const
    {
        Q_ASSERT(format);
        return !format.videoSupportedFrameRateRanges || !format.videoSupportedFrameRateRanges.count;
    }
};

Float64 qt_find_min_framerate_distance(AVCaptureDeviceFormat *format, Float64 fps)
{
    Q_ASSERT(format && format.videoSupportedFrameRateRanges
             && format.videoSupportedFrameRateRanges.count);

    AVFrameRateRange *range = [format.videoSupportedFrameRateRanges objectAtIndex:0];
    Float64 distance = qAbs(range.maxFrameRate - fps);
    for (NSUInteger i = 1, e = format.videoSupportedFrameRateRanges.count; i < e; ++i) {
        range = [format.videoSupportedFrameRateRanges objectAtIndex:i];
        distance = qMin(distance, qAbs(range.maxFrameRate - fps));
    }

    return distance;
}

} // Unnamed namespace.

AVCaptureDeviceFormat *
qt_convert_to_capture_device_format(AVCaptureDevice *captureDevice,
                                    const QCameraFormat &cameraFormat,
                                    const std::function<bool(uint32_t)> &cvFormatValidator)
{
    const auto cameraFormatPrivate = QCameraFormatPrivate::handle(cameraFormat);
    if (!cameraFormatPrivate)
        return nil;

    const auto requiredCvPixFormat = QAVFHelpers::toCVPixelFormat(cameraFormatPrivate->pixelFormat,
                                                                  cameraFormatPrivate->colorRange);

    if (requiredCvPixFormat == CvPixelFormatInvalid)
        return nil;

    AVCaptureDeviceFormat *newFormat = nil;
    Float64 newFormatMaxFrameRate = {};
    NSArray<AVCaptureDeviceFormat *> *formats = captureDevice.formats;
    for (AVCaptureDeviceFormat *format in formats) {
        CMFormatDescriptionRef formatDesc = format.formatDescription;
        CMVideoDimensions dim = CMVideoFormatDescriptionGetDimensions(formatDesc);
        FourCharCode cvPixFormat = CMVideoFormatDescriptionGetCodecType(formatDesc);

        if (cvPixFormat != requiredCvPixFormat)
            continue;

        if (cameraFormatPrivate->resolution != QSize(dim.width, dim.height))
            continue;

        if (cvFormatValidator && !cvFormatValidator(cvPixFormat))
            continue;

        const float epsilon = 0.001f;
        for (AVFrameRateRange *frameRateRange in format.videoSupportedFrameRateRanges) {
            if (frameRateRange.minFrameRate >= cameraFormatPrivate->minFrameRate - epsilon
                && frameRateRange.maxFrameRate <= cameraFormatPrivate->maxFrameRate + epsilon
                && newFormatMaxFrameRate < frameRateRange.maxFrameRate) {
                newFormat = format;
                newFormatMaxFrameRate = frameRateRange.maxFrameRate;
            }
        }
    }
    return newFormat;
}

QVector<AVCaptureDeviceFormat *> qt_unique_device_formats(AVCaptureDevice *captureDevice, FourCharCode filter)
{
    // 'filter' is the format we prefer if we have duplicates.
    Q_ASSERT(captureDevice);

    QVector<AVCaptureDeviceFormat *> formats;

    if (!captureDevice.formats || !captureDevice.formats.count)
        return formats;

    formats.reserve(captureDevice.formats.count);
    for (AVCaptureDeviceFormat *format in captureDevice.formats) {
        const QSize resolution(qt_device_format_resolution(format));
        if (resolution.isNull() || !resolution.isValid())
            continue;
        formats << format;
    }

    if (!formats.size())
        return formats;

    std::sort(formats.begin(), formats.end(), ByResolution<std::less>());

    QSize size(qt_device_format_resolution(formats[0]));
    FourCharCode codec = CMVideoFormatDescriptionGetCodecType(formats[0].formatDescription);
    int last = 0;
    for (int i = 1; i < formats.size(); ++i) {
        const QSize nextSize(qt_device_format_resolution(formats[i]));
        if (nextSize == size) {
            if (codec == filter)
                continue;
            formats[last] = formats[i];
        } else {
            ++last;
            formats[last] = formats[i];
            size = nextSize;
        }
        codec = CMVideoFormatDescriptionGetCodecType(formats[i].formatDescription);
    }
    formats.resize(last + 1);

    return formats;
}

QSize qt_device_format_resolution(AVCaptureDeviceFormat *format)
{
    if (!format || !format.formatDescription)
        return QSize();

    const CMVideoDimensions res = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
    return QSize(res.width, res.height);
}

QSize qt_device_format_high_resolution(AVCaptureDeviceFormat *format)
{
    Q_ASSERT(format);
    QSize res;
#if defined(Q_OS_IOS)
    const CMVideoDimensions hrDim(format.highResolutionStillImageDimensions);
    res.setWidth(hrDim.width);
    res.setHeight(hrDim.height);
#endif
    return res;
}

QVector<AVFPSRange> qt_device_format_framerates(AVCaptureDeviceFormat *format)
{
    Q_ASSERT(format);

    QVector<AVFPSRange> qtRanges;

    if (!format.videoSupportedFrameRateRanges || !format.videoSupportedFrameRateRanges.count)
        return qtRanges;

    qtRanges.reserve(format.videoSupportedFrameRateRanges.count);
    for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges)
        qtRanges << AVFPSRange(range.minFrameRate, range.maxFrameRate);

    return qtRanges;
}

QSize qt_device_format_pixel_aspect_ratio(AVCaptureDeviceFormat *format)
{
    Q_ASSERT(format);

    if (!format.formatDescription) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "no format description found";
        return QSize();
    }

    const CMVideoDimensions res = CMVideoFormatDescriptionGetDimensions(format.formatDescription);
    const CGSize resPAR = CMVideoFormatDescriptionGetPresentationDimensions(format.formatDescription, true, false);

    if (qAbs(resPAR.width - res.width) < 1.) {
        // "Pixel aspect ratio is used to adjust the width, leaving the height alone."
        return QSize(1, 1);
    }

    if (!res.width || !resPAR.width)
        return QSize();

    auto frac = qRealToFraction(resPAR.width > res.width ? res.width / qreal(resPAR.width)
                                                         : resPAR.width / qreal(res.width));

    return QSize(frac.numerator, frac.denominator);
}

AVCaptureDeviceFormat *qt_find_best_resolution_match(AVCaptureDevice *captureDevice,
                                                     const QSize &request,
                                                     FourCharCode filter,
                                                     bool stillImage)
{
    Q_ASSERT(captureDevice);
    Q_ASSERT(!request.isNull() && request.isValid());

    if (!captureDevice.formats || !captureDevice.formats.count)
        return nullptr;

    QVector<AVCaptureDeviceFormat *> formats(qt_unique_device_formats(captureDevice, filter));

    for (int i = 0; i < formats.size(); ++i) {
        AVCaptureDeviceFormat *format = formats[i];
        if (qt_device_format_resolution(format) == request)
            return format;
        // iOS only (still images).
        if (stillImage && qt_device_format_high_resolution(format) == request)
            return format;
    }

    if (!qt_area_sane(request))
        return nullptr;

    typedef std::pair<QSize, AVCaptureDeviceFormat *> FormatPair;

    QVector<FormatPair> pairs; // default|HR sizes
    pairs.reserve(formats.size());

    for (int i = 0; i < formats.size(); ++i) {
        AVCaptureDeviceFormat *format = formats[i];
        const QSize res(qt_device_format_resolution(format));
        if (!res.isNull() && res.isValid() && qt_area_sane(res))
            pairs << FormatPair(res, format);
        const QSize highRes(qt_device_format_high_resolution(format));
        if (stillImage && !highRes.isNull() && highRes.isValid() && qt_area_sane(highRes))
            pairs << FormatPair(highRes, format);
    }

    if (!pairs.size())
        return nullptr;

    AVCaptureDeviceFormat *best = pairs[0].second;
    QSize next(pairs[0].first);
    int wDiff = qAbs(request.width() - next.width());
    int hDiff = qAbs(request.height() - next.height());
    const int area = request.width() * request.height();
    int areaDiff = qAbs(area - next.width() * next.height());
    for (int i = 1; i < pairs.size(); ++i) {
        next = pairs[i].first;
        const int newWDiff = qAbs(next.width() - request.width());
        const int newHDiff = qAbs(next.height() - request.height());
        const int newAreaDiff = qAbs(area - next.width() * next.height());

        if ((newWDiff < wDiff && newHDiff < hDiff)
            || ((newWDiff <= wDiff || newHDiff <= hDiff) && newAreaDiff <= areaDiff)) {
            wDiff = newWDiff;
            hDiff = newHDiff;
            best = pairs[i].second;
            areaDiff = newAreaDiff;
        }
    }

    return best;
}

AVCaptureDeviceFormat *qt_find_best_framerate_match(AVCaptureDevice *captureDevice,
                                                    FourCharCode filter,
                                                    Float64 fps)
{
    Q_ASSERT(captureDevice);
    Q_ASSERT(fps > 0.);

    const qreal epsilon = 0.1;

    QVector<AVCaptureDeviceFormat *>sorted(qt_unique_device_formats(captureDevice, filter));
    // Sort formats by their resolution in decreasing order:
    std::sort(sorted.begin(), sorted.end(), ByResolution<std::greater>());
    // We can use only formats with framerate ranges:
    sorted.erase(std::remove_if(sorted.begin(), sorted.end(), FormatHasNoFPSRange()), sorted.end());

    if (!sorted.size())
        return nil;

    for (int i = 0; i < sorted.size(); ++i) {
        AVCaptureDeviceFormat *format = sorted[i];
        for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges) {
            if (range.maxFrameRate - range.minFrameRate < epsilon) {
                // On OS X ranges are points (built-in camera).
                if (qAbs(fps - range.maxFrameRate) < epsilon)
                    return format;
            }

            if (fps >= range.minFrameRate && fps <= range.maxFrameRate)
                return format;
        }
    }

    Float64 distance = qt_find_min_framerate_distance(sorted[0], fps);
    AVCaptureDeviceFormat *match = sorted[0];
    for (int i = 1; i < sorted.size(); ++i) {
        const Float64 newDistance = qt_find_min_framerate_distance(sorted[i], fps);
        if (newDistance < distance) {
            distance = newDistance;
            match = sorted[i];
        }
    }

    return match;
}

AVFrameRateRange *qt_find_supported_framerate_range(AVCaptureDeviceFormat *format, Float64 fps)
{
    Q_ASSERT(format && format.videoSupportedFrameRateRanges
             && format.videoSupportedFrameRateRanges.count);

    const qreal epsilon = 0.1;

    for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges) {
        if (range.maxFrameRate - range.minFrameRate < epsilon) {
            // On OS X ranges are points (built-in camera).
            if (qAbs(fps - range.maxFrameRate) < epsilon)
                return range;
        }

        if (fps >= range.minFrameRate && fps <= range.maxFrameRate)
            return range;
    }

    AVFrameRateRange *match = [format.videoSupportedFrameRateRanges objectAtIndex:0];
    Float64 distance = qAbs(match.maxFrameRate - fps);
    for (NSUInteger i = 1, e = format.videoSupportedFrameRateRanges.count; i < e; ++i) {
        AVFrameRateRange *range = [format.videoSupportedFrameRateRanges objectAtIndex:i];
        const Float64 newDistance = qAbs(range.maxFrameRate - fps);
        if (newDistance < distance) {
            distance = newDistance;
            match = range;
        }
    }

    return match;
}

bool qt_format_supports_framerate(AVCaptureDeviceFormat *format, qreal fps)
{
    if (format && fps > qreal(0)) {
        const qreal epsilon = 0.1;
        for (AVFrameRateRange *range in format.videoSupportedFrameRateRanges) {
            if (fps >= range.minFrameRate - epsilon && fps <= range.maxFrameRate + epsilon)
                return true;
        }
    }

    return false;
}

bool qt_formats_are_equal(AVCaptureDeviceFormat *f1, AVCaptureDeviceFormat *f2)
{
    if (f1 == f2)
        return true;

    if (![f1.mediaType isEqualToString:f2.mediaType])
        return false;

    return CMFormatDescriptionEqual(f1.formatDescription, f2.formatDescription);
}

bool qt_set_active_format(AVCaptureDevice *captureDevice, AVCaptureDeviceFormat *format, bool preserveFps)
{
    static bool firstSet = true;

    if (!captureDevice || !format)
        return false;

    if (qt_formats_are_equal(captureDevice.activeFormat, format)) {
        if (firstSet) {
            // The capture device format is persistent. The first time we set a format, report that
            // it changed even if the formats are the same.
            // This prevents the session from resetting the format to the default value.
            firstSet = false;
            return true;
        }
        return false;
    }

    firstSet = false;

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qWarning("Failed to set active format (lock failed)");
        return false;
    }

    // Changing the activeFormat resets the frame rate.
    AVFPSRange fps;
    if (preserveFps)
        fps = qt_current_framerates(captureDevice, nil);

    captureDevice.activeFormat = format;

    if (preserveFps)
        qt_set_framerate_limits(captureDevice, nil, fps.first, fps.second);

    return true;
}

void qt_set_framerate_limits(AVCaptureConnection *videoConnection, qreal minFPS, qreal maxFPS)
{
    Q_ASSERT(videoConnection);

    if (minFPS < 0. || maxFPS < 0. || (maxFPS && maxFPS < minFPS)) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "invalid framerates (min, max):"
                       << minFPS << maxFPS;
        return;
    }

    CMTime minDuration = kCMTimeInvalid;
    if (maxFPS > 0.) {
        if (!videoConnection.supportsVideoMinFrameDuration)
            qCDebug(qLcCamera) << Q_FUNC_INFO << "maximum framerate is not supported";
        else
            minDuration = CMTimeMake(1, maxFPS);
    }
    if (videoConnection.supportsVideoMinFrameDuration)
        videoConnection.videoMinFrameDuration = minDuration;

    CMTime maxDuration = kCMTimeInvalid;
    if (minFPS > 0.) {
        if (!videoConnection.supportsVideoMaxFrameDuration)
            qCDebug(qLcCamera) << Q_FUNC_INFO << "minimum framerate is not supported";
        else
            maxDuration = CMTimeMake(1, minFPS);
    }
    if (videoConnection.supportsVideoMaxFrameDuration)
        videoConnection.videoMaxFrameDuration = maxDuration;
}

CMTime qt_adjusted_frame_duration(AVFrameRateRange *range, qreal fps)
{
    Q_ASSERT(range);
    Q_ASSERT(fps > 0.);

    if (range.maxFrameRate - range.minFrameRate < 0.1) {
        // Can happen on OS X.
        return range.minFrameDuration;
    }

    if (fps <= range.minFrameRate)
        return range.maxFrameDuration;
    if (fps >= range.maxFrameRate)
        return range.minFrameDuration;

    auto frac = qRealToFraction(1. / fps);
    return CMTimeMake(frac.numerator, frac.denominator);
}

void qt_set_framerate_limits(AVCaptureDevice *captureDevice, qreal minFPS, qreal maxFPS)
{
    Q_ASSERT(captureDevice);
    if (!captureDevice.activeFormat) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "no active capture device format";
        return;
    }

    if (minFPS < 0. || maxFPS < 0. || (maxFPS && maxFPS < minFPS)) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "invalid framerates (min, max):"
                       << minFPS << maxFPS;
        return;
    }

    CMTime minFrameDuration = kCMTimeInvalid;
    CMTime maxFrameDuration = kCMTimeInvalid;
    if (maxFPS || minFPS) {
        AVFrameRateRange *range = qt_find_supported_framerate_range(captureDevice.activeFormat,
                                                                    maxFPS ? maxFPS : minFPS);
        if (!range) {
            qCDebug(qLcCamera) << Q_FUNC_INFO << "no framerate range found, (min, max):"
                           << minFPS << maxFPS;
            return;
        }

        if (maxFPS)
            minFrameDuration = qt_adjusted_frame_duration(range, maxFPS);
        if (minFPS)
            maxFrameDuration = qt_adjusted_frame_duration(range, minFPS);
    }

    const AVFConfigurationLock lock(captureDevice);
    if (!lock) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to lock for configuration";
        return;
    }

    // While Apple's docs say kCMTimeInvalid will end in default
    // settings for this format, kCMTimeInvalid on OS X ends with a runtime
    // exception:
    // "The activeVideoMinFrameDuration passed is not supported by the device."
    // Instead, use the first item in the supported frame rates.
#ifdef Q_OS_IOS
    [captureDevice setActiveVideoMinFrameDuration:minFrameDuration];
    [captureDevice setActiveVideoMaxFrameDuration:maxFrameDuration];
#elif defined(Q_OS_MACOS)
    if (CMTimeCompare(minFrameDuration, kCMTimeInvalid) == 0
            && CMTimeCompare(maxFrameDuration, kCMTimeInvalid) == 0) {
        AVFrameRateRange *range = captureDevice.activeFormat.videoSupportedFrameRateRanges.firstObject;
        minFrameDuration = range.minFrameDuration;
        maxFrameDuration = range.maxFrameDuration;
    }

    if (CMTimeCompare(minFrameDuration, kCMTimeInvalid))
        [captureDevice setActiveVideoMinFrameDuration:minFrameDuration];

    if (CMTimeCompare(maxFrameDuration, kCMTimeInvalid))
        [captureDevice setActiveVideoMaxFrameDuration:maxFrameDuration];
#endif // Q_OS_MACOS
}

void qt_set_framerate_limits(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection,
                             qreal minFPS, qreal maxFPS)
{
    Q_UNUSED(videoConnection);
    Q_ASSERT(captureDevice);
    qt_set_framerate_limits(captureDevice, minFPS, maxFPS);
}

AVFPSRange qt_current_framerates(AVCaptureDevice *captureDevice, AVCaptureConnection *videoConnection)
{
    Q_UNUSED(videoConnection);
    Q_ASSERT(captureDevice);

    AVFPSRange fps;
    const CMTime minDuration = captureDevice.activeVideoMinFrameDuration;
    if (CMTimeCompare(minDuration, kCMTimeInvalid)) {
        if (const Float64 minSeconds = CMTimeGetSeconds(minDuration))
            fps.second = 1. / minSeconds; // Max FPS = 1 / MinDuration.
    }

    const CMTime maxDuration = captureDevice.activeVideoMaxFrameDuration;
    if (CMTimeCompare(maxDuration, kCMTimeInvalid)) {
        if (const Float64 maxSeconds = CMTimeGetSeconds(maxDuration))
            fps.first = 1. / maxSeconds; // Min FPS = 1 / MaxDuration.
    }

    return fps;
}

QList<AudioValueRange> qt_supported_sample_rates_for_format(int codecId)
{
    UInt32 format = codecId;
    UInt32 size;
    OSStatus err = AudioFormatGetPropertyInfo(
            kAudioFormatProperty_AvailableEncodeSampleRates,
            sizeof(format),
            &format,
            &size);

    if (err != noErr)
        return {};

    UInt32 numRanges = size / sizeof(AudioValueRange);
    QList<AudioValueRange> result;
    result.resize(numRanges);

    err = AudioFormatGetProperty(kAudioFormatProperty_AvailableEncodeSampleRates,
                                sizeof(format),
                                &format,
                                &size,
                                result.data());
    return err == noErr ? result : QList<AudioValueRange>{};
}

QList<AudioValueRange> qt_supported_bit_rates_for_format(int codecId)
{
    UInt32 format = codecId;
    UInt32 size;
    OSStatus err = AudioFormatGetPropertyInfo(
            kAudioFormatProperty_AvailableEncodeBitRates,
            sizeof(format),
            &format,
            &size);

    if (err != noErr)
        return {};

    UInt32 numRanges = size / sizeof(AudioValueRange);
    QList<AudioValueRange> result;
    result.resize(numRanges);

    err = AudioFormatGetProperty(kAudioFormatProperty_AvailableEncodeBitRates,
                                sizeof(format),
                                &format,
                                &size,
                                result.data());
    return err == noErr ? result : QList<AudioValueRange>{};
}

std::optional<QList<UInt32>> qt_supported_channel_counts_for_format(int codecId)
{
    AudioStreamBasicDescription sf = {};
    sf.mFormatID = codecId;
    UInt32 size;
    OSStatus err = AudioFormatGetPropertyInfo(
            kAudioFormatProperty_AvailableEncodeNumberChannels,
            sizeof(sf),
            &sf,
            &size);

    if (err != noErr)
        return std::nullopt;

    // From Apple's docs:
    // A value of 0xFFFFFFFF indicates that any number of channels may be encoded.
    if (int(size) == -1)
        return std::nullopt;

    UInt32 numCounts = size / sizeof(UInt32);
    QList<UInt32> channelCounts;
    channelCounts.resize(numCounts);

    err = AudioFormatGetProperty(kAudioFormatProperty_AvailableEncodeNumberChannels,
                                sizeof(sf),
                                &sf,
                                &size,
                                channelCounts.data());
    if (err == noErr)
        return channelCounts;
    else
        return std::nullopt;
}

QList<UInt32> qt_supported_channel_layout_tags_for_format(int codecId, int noChannels)
{
    AudioStreamBasicDescription sf = {};
    sf.mFormatID = codecId;
    sf.mChannelsPerFrame = noChannels;
    UInt32 size;
    OSStatus err = AudioFormatGetPropertyInfo(
            kAudioFormatProperty_AvailableEncodeChannelLayoutTags,
            sizeof(sf),
            &sf,
            &size);

    if (err != noErr)
        return {};

    UInt32 noTags = (UInt32)size / sizeof(UInt32);
    QList<AudioChannelLayoutTag> tagsArr;
    tagsArr.resize(noTags);

    err = AudioFormatGetProperty(kAudioFormatProperty_AvailableEncodeChannelLayoutTags,
                                sizeof(sf),
                                &sf,
                                &size,
                                tagsArr.data());
    if (err != noErr)
        return {};

    QList<UInt32> result;
    for (const AudioChannelLayoutTag &item : tagsArr)
        result.push_back(item);

    return result;
}

#ifdef Q_OS_IOS
int qt_ui_device_orientation_to_rotation_angle_degrees(UIDeviceOrientation orientation)
{
    switch (orientation) {
    case UIDeviceOrientationLandscapeLeft: return 0;
    case UIDeviceOrientationPortrait: return 90;
    case UIDeviceOrientationLandscapeRight: return 180;
    case UIDeviceOrientationPortraitUpsideDown: return 270;
    default: return 0;
    }
}
#endif

QT_END_NAMESPACE
