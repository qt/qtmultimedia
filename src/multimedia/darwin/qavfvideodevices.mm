// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qavfvideodevices_p.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qavfcamerautility_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qset.h>

#import <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcAvfVideoDevices, "qt.multimedia.avfvideodevices");

namespace {
// Helper function to translate AVCaptureDevicePosition enum to QCameraDevice::Position enum.
[[nodiscard]] QCameraDevice::Position qAvfToQCameraDevicePosition(AVCaptureDevicePosition input)
{
    switch (input) {
    case AVCaptureDevicePositionFront:
        return QCameraDevice::Position::FrontFace;
    case AVCaptureDevicePositionBack:
        return QCameraDevice::Position::BackFace;
    default:
        return QCameraDevice::Position::UnspecifiedPosition;
    }
}

// Thread-safe
[[nodiscard]] QList<AVCaptureDevice*> qEnumerateAVCaptureDevices()
{
    // List of all capture device types that we want to discover. Seems that this is the
    // only way to discover all types. This filter is mandatory and has no "unspecified"
    // option like AVCaptureDevicePosition(Unspecified) has. Order of the list is important
    // because discovered devices will be in the same order and we want the first one found
    // to be our default device.
    NSArray *discoveryDevices = @[
#ifdef Q_OS_IOS
        AVCaptureDeviceTypeBuiltInTripleCamera,    // We always  prefer triple camera.
        AVCaptureDeviceTypeBuiltInDualCamera,      // If triple is not available, we prefer
                                                   // dual with wide + tele lens.
        AVCaptureDeviceTypeBuiltInDualWideCamera,  // Dual with wide and ultrawide is still
                                                   // better than single.
#endif
        AVCaptureDeviceTypeBuiltInWideAngleCamera, // This is the most common single camera type.
                                                   // We prefer that over tele and ultra-wide.
#ifdef Q_OS_IOS
        AVCaptureDeviceTypeBuiltInTelephotoCamera, // Cannot imagine how, but if only tele and
                                                   // ultrawide are available, we prefer tele.
        AVCaptureDeviceTypeBuiltInUltraWideCamera,
#endif
    ];

    if (@available(macOS 14, iOS 17, *)) {
        discoveryDevices = [discoveryDevices arrayByAddingObjectsFromArray: @[
            AVCaptureDeviceTypeExternal,
            AVCaptureDeviceTypeContinuityCamera
        ]];
    } else {
#ifdef Q_OS_MACOS
    QT_WARNING_PUSH
        QT_WARNING_DISABLE_DEPRECATED
            discoveryDevices = [discoveryDevices arrayByAddingObjectsFromArray: @[
                AVCaptureDeviceTypeExternalUnknown
            ]];
    QT_WARNING_POP
#endif
    }
    // Create discovery session to discover all possible camera types of the system.
    // Both "hard" and "soft" types.
    AVCaptureDeviceDiscoverySession *discoverySession = [AVCaptureDeviceDiscoverySession
        discoverySessionWithDeviceTypes:discoveryDevices
                              mediaType:AVMediaTypeVideo
                               position:AVCaptureDevicePositionUnspecified];
    QList<AVCaptureDevice*> avCaptureDevices;
    for (AVCaptureDevice* device in discoverySession.devices)
        avCaptureDevices.push_back(device);
    return avCaptureDevices;
}

// Given a list of AVCaptureDevices, returns a list of all the QCameraDevices
// we want to expose to the user.
// Thread-safe
[[nodiscard]] QList<QCameraDevice> qGenerateQCameraDevices(
    QList<AVCaptureDevice*> videoDevices,
    const std::function<bool(CvPixelFormat)>& isCvPixelFormatSupported)
{
    QList<QCameraDevice> cameras;

    for (AVCaptureDevice *device : videoDevices) {
        if ([device isSuspended])
            continue;

        auto info = std::make_unique<QCameraDevicePrivate>();
        if ([videoDevices[0].uniqueID isEqualToString:device.uniqueID])
            info->isDefault = true;
        info->id = QByteArray([[device uniqueID] UTF8String]);
        info->description = QString::fromNSString([device localizedName]);
        info->position = qAvfToQCameraDevicePosition([device position]);

        qCDebug(qLcAvfVideoDevices) << "Handling camera info" << info->description
            << (info->isDefault ? "(default)" : "");

        QSet<QSize> photoResolutions;
        QList<QCameraFormat> videoFormats;

        for (AVCaptureDeviceFormat *format in device.formats) {
            if (![format.mediaType isEqualToString:AVMediaTypeVideo])
                continue;

            const CMVideoDimensions dimensions =
                CMVideoFormatDescriptionGetDimensions(format.formatDescription);
            QSize resolution(dimensions.width, dimensions.height);
            photoResolutions.insert(resolution);

            float maxFrameRate = 0;
            float minFrameRate = 1.e6;

            const CvPixelFormat cvPixelFormat =
                CMVideoFormatDescriptionGetCodecType(format.formatDescription);

            // Don't expose formats if the media backend says we can't start a capture session
            // with it.
            if (!isCvPixelFormatSupported(cvPixelFormat))
                continue;

            const QVideoFrameFormat::PixelFormat pixelFormat =
                QAVFHelpers::fromCVPixelFormat(cvPixelFormat);
            const QVideoFrameFormat::ColorRange colorRange =
                QAVFHelpers::colorRangeForCVPixelFormat(cvPixelFormat);

            // Ignore pixel formats we can't handle
            if (pixelFormat == QVideoFrameFormat::Format_Invalid) {
                qCDebug(qLcAvfVideoDevices) << "ignore camera CV format" << cvPixelFormat
                    << "as no matching video format found";
                continue;
            }

            for (const AVFrameRateRange *frameRateRange in format.videoSupportedFrameRateRanges) {
                if (frameRateRange.minFrameRate < minFrameRate)
                    minFrameRate = frameRateRange.minFrameRate;
                if (frameRateRange.maxFrameRate > maxFrameRate)
                    maxFrameRate = frameRateRange.maxFrameRate;
            }

#ifdef Q_OS_IOS
            // From Apple's docs (iOS):
            // By default, AVCaptureStillImageOutput emits images with the same dimensions as
            // its source AVCaptureDevice instance’s activeFormat.formatDescription. However,
            // if you set this property to YES, the receiver emits still images at the capture
            // device’s highResolutionStillImageDimensions value.
            const QSize hrRes(qt_device_format_high_resolution(format));
            if (!hrRes.isNull() && hrRes.isValid())
                photoResolutions.insert(hrRes);
#endif

            qCDebug(qLcAvfVideoDevices) << "Add camera format. pixelFormat:" << pixelFormat
                << "colorRange:" << colorRange << "cvPixelFormat" << cvPixelFormat
                << "resolution:" << resolution << "frameRate: [" << minFrameRate
                << maxFrameRate << "]";

            auto *f = new QCameraFormatPrivate{ QSharedData(), pixelFormat,  resolution,
                                                minFrameRate,  maxFrameRate, colorRange };
            videoFormats << f->create();
        }
        if (videoFormats.isEmpty()) {
            // skip broken cameras without valid formats
            qCWarning(qLcAvfVideoDevices())
                << "Skip camera" << info->description << "without supported formats";
            continue;
        }
        info->videoFormats = videoFormats;
        info->photoResolutions = photoResolutions.values();

        cameras.append(info.release()->create());
    }

    return cameras;
}

} // Unnamed namespace.

QAVFVideoDevices::QAVFVideoDevices(
    QPlatformMediaIntegration *integration,
    std::function<bool(uint32_t)> &&isCvPixelFormatSupportedDelegate)
    : QPlatformVideoDevices(integration),
      m_isCvPixelFormatSupportedDelegate(std::move(isCvPixelFormatSupportedDelegate))
{
    m_deviceConnectedObserver = QMacNotificationObserver(
        nil,
        AVCaptureDeviceWasConnectedNotification,
        [this]() {
            // Ordinarily this callback should be invoked on the same thread as
            // the one that registered it, but this has been observed to not be
            // the case on iOS. So we post a job to the correct thread instead.
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    rebuildObserveredAvCaptureDevices();
                    QPlatformVideoDevices::onVideoInputsChanged();
                });
        });

    m_deviceDisconnectedObserver = QMacNotificationObserver(
        nil,
        AVCaptureDeviceWasDisconnectedNotification,
        [this]() {
            QMetaObject::invokeMethod(
                this,
                [this]() {
                    rebuildObserveredAvCaptureDevices();
                    QPlatformVideoDevices::onVideoInputsChanged();
                });
        });

    rebuildObserveredAvCaptureDevices();
}

QAVFVideoDevices::~QAVFVideoDevices()
{
    clearObservedAvCaptureDevices();
}

// Does NOT lock
void QAVFVideoDevices::clearObservedAvCaptureDevices() {
    for (ObservedAVCaptureDevice& observedDevice : m_observedAvCaptureDevices) {
        // Observer must be cleared before the AVCaptureDevice.
        observedDevice.observer = {};
        [observedDevice.avCaptureDevice release];
    }
    m_observedAvCaptureDevices.clear();
}

// Can be called from any thread as result of QMediaDevices::videoInputs()
QList<QCameraDevice> QAVFVideoDevices::findVideoInputs() const
{
    const QList<AVCaptureDevice*> avCaptureDevices = qEnumerateAVCaptureDevices();
    return qGenerateQCameraDevices(
        avCaptureDevices,
        [this](uint32_t cvPixelFormat) {
            return isCvPixelFormatSupported(cvPixelFormat);
        });
}

bool QAVFVideoDevices::isCvPixelFormatSupported(uint32_t cvPixelFormat) const
{
    return !m_isCvPixelFormatSupportedDelegate || m_isCvPixelFormatSupportedDelegate(cvPixelFormat);
}

// Refreshes list of connected AVCaptureDevices and their key-value observers.
// Thread-safe.
void QAVFVideoDevices::rebuildObserveredAvCaptureDevices()
{
    const QList<AVCaptureDevice*> avCaptureDevices = qEnumerateAVCaptureDevices();

    clearObservedAvCaptureDevices();

    for (AVCaptureDevice *captureDevice : avCaptureDevices) {
        ObservedAVCaptureDevice observedDevice;

        observedDevice.avCaptureDevice = captureDevice;
        [observedDevice.avCaptureDevice retain];

        // When the suspended value changes, post an update job to
        // QAVFVideoDevices.
        observedDevice.observer = QMacKeyValueObserver(
            observedDevice.avCaptureDevice,
            @"suspended",
            [this]() {
                // Callback can potentially run on another thread. Post a job to
                // our thread.
                QMetaObject::invokeMethod(
                    this,
                    [this]() {
                        rebuildObserveredAvCaptureDevices();
                        QPlatformVideoDevices::onVideoInputsChanged();
                    });
            });

        m_observedAvCaptureDevices.push_back(std::move(observedDevice));
    }
}

QT_END_NAMESPACE
