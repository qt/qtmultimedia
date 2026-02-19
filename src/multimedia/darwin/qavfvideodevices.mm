// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qavfvideodevices_p.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qavfcamerautility_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/qset.h>
#include <QtCore/qspan.h>
#include <QtCore/qthread.h>

#include <vector>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcAvfVideoDevices, "qt.multimedia.avfvideodevices");

using namespace Qt::Literals::StringLiterals;

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

// Error message is not user-facing.
[[nodiscard]] q23::expected<AVFScopedPointer<AVCaptureDeviceDiscoverySession>, QString>
createAvCaptureDeviceDiscoverySession()
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

    if (@available(macOS 14, *)) {
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

    @try {
        // Create discovery session to discover all possible camera types of the system.
        // Both "hard" and "soft" types.
        AVCaptureDeviceDiscoverySession *discoverySession = [AVCaptureDeviceDiscoverySession
            discoverySessionWithDeviceTypes:discoveryDevices
                                  mediaType:AVMediaTypeVideo
                                   position:AVCaptureDevicePositionUnspecified];
        return AVFScopedPointer{ [discoverySession retain] };
    }
    @catch (NSException *e) {
        return q23::unexpected{
            u"Exception caught when trying to create AVCaptureDeviceDiscoverySession: "_s
            + QString::fromNSString(e.reason) };
    }
}

// Given a list of AVCaptureDevices, returns a list of all the QCameraDevices
// we want to expose to the user.
// Thread-safe
template <typename FormatChecker>
[[nodiscard]] QList<QCameraDevice>
qGenerateQCameraDevices(NSArray<AVCaptureDevice *> *videoDevices,
                        const FormatChecker &isCvPixelFormatSupported)
{
    QList<QCameraDevice> cameras;

    for (AVCaptureDevice *device in videoDevices) {
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

} // Unnamed namespace

// Can be called by any thread
QAVFVideoDevices::QAVFVideoDevices(
    QPlatformMediaIntegration *integration,
    std::function<bool(uint32_t)> &&isCvPixelFormatSupportedDelegate)
    : QPlatformVideoDevices(integration),
      m_isCvPixelFormatSupportedDelegate(std::move(isCvPixelFormatSupportedDelegate))
{
    Q_ASSERT(QCoreApplication::instance());
    moveToThread(QCoreApplication::instance()->thread());

    // Calling thread might not have any dispatch_queue or autorelease pool.
    QMacAutoReleasePool autoReleasePool;

    auto discoverySessionResult = createAvCaptureDeviceDiscoverySession();
    if (!discoverySessionResult) {
        qCWarning(qLcAvfVideoDevices) << discoverySessionResult.error();
        qWarning() << "Failed to establish camera device discovery session. "
                      "QMediaDevices::videoInputs() will not work.";
        return;
    }

    m_avDiscoverySession = std::move(*discoverySessionResult);
    m_avDiscoverySessionObserver = QMacKeyValueObserver(
        m_avDiscoverySession,
        @"devices",
        [this] {
            onAvCaptureDevicesChanged();
        });

    // Setup initial list of observed AVCaptureDevices.
    QMetaObject::invokeMethod(this, [this]{
        rebuildObserveredAvCaptureDevices();
    });
}

QAVFVideoDevices::~QAVFVideoDevices() = default;

// Can be called from any thread as result of QMediaDevices::videoInputs()
QList<QCameraDevice> QAVFVideoDevices::findVideoInputs() const
{
    if (!m_avDiscoverySession)
        return {};

    // This function can be called from any thread, including
    // threads with no dispatch_queue, so we need an autorelease pool.
    QMacAutoReleasePool autoReleasePool;

    NSArray<AVCaptureDevice *> *deviceList = m_avDiscoverySession.data().devices;
    Q_ASSERT(deviceList);

    return qGenerateQCameraDevices(deviceList, [this](uint32_t cvPixelFormat) {
        return isCvPixelFormatSupported(cvPixelFormat);
    });
}

bool QAVFVideoDevices::isCvPixelFormatSupported(uint32_t cvPixelFormat) const
{
    return !m_isCvPixelFormatSupportedDelegate || m_isCvPixelFormatSupportedDelegate(cvPixelFormat);
}

// Refreshes list of connected AVCaptureDevices and their key-value observers.
void QAVFVideoDevices::rebuildObserveredAvCaptureDevices()
{
    Q_ASSERT(QCoreApplication::instance()->thread()->isCurrentThread());

    m_observedAvCaptureDevices.clear();

    if (!m_avDiscoverySession)
        return;

    NSArray<AVCaptureDevice *> *deviceList = m_avDiscoverySession.data().devices;
    Q_ASSERT(deviceList);

    m_observedAvCaptureDevices.reserve(deviceList.count);

    for (AVCaptureDevice *captureDevice in deviceList) {
        AVFScopedPointer retainedDevice{ [captureDevice retain] };

        // When the suspended value changes, post an update job to QAVFVideoDevices.
        QMacKeyValueObserver observer(
            captureDevice,
            @"suspended",
            [this] {
                onAvCaptureDevicesChanged();
            });

        m_observedAvCaptureDevices.push_back({ std::move(retainedDevice), std::move(observer) });
    }
}

void QAVFVideoDevices::onAvCaptureDevicesChanged()
{
    // Callbacks can potentially get invoked in cocoa threads.
    // Post a job to the object's thread.
    QMetaObject::invokeMethod(this, [this] {
        rebuildObserveredAvCaptureDevices();
        onVideoInputsChanged();
    });
}

QT_END_NAMESPACE
