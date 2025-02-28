// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidvideodevices_p.h"
#include "qandroidvideoframebuffer_p.h"

#include <private/qcameradevice_p.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qandroidextras_p.h>
#include <QtCore/qcoreapplication_platform.h>
#include <QtCore/qjnienvironment.h>
#include <jni.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(qLCAndroidVideoDevices, "qt.multimedia.ffmpeg.android.videoDevices");

Q_DECLARE_JNI_CLASS(
    QtCameraAvailabilityListener,
    "org/qtproject/qt/android/multimedia/QtCameraAvailabilityListener");

QAndroidVideoDevices::QAndroidVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
    registerNativeMethods();

    m_javaCameraAvailabilityListener = QtJniTypes::QtCameraAvailabilityListener(
        QtAndroidPrivate::activity(),
        static_cast<jlong>(reinterpret_cast<size_t>(this)));
}

QAndroidVideoDevices::~QAndroidVideoDevices()
{
    m_javaCameraAvailabilityListener.callMethod<void>("cleanup");
}

QCameraFormat createCameraFormat(int width, int height, int fpsMin, int fpsMax)
{
    QCameraFormatPrivate *format = new QCameraFormatPrivate();

    format->resolution = { width, height };

    format->minFrameRate = fpsMin;
    format->maxFrameRate = fpsMax;

    format->pixelFormat = QVideoFrameFormat::PixelFormat::Format_YUV420P;

    return format->create();
}

QList<QCameraDevice> QAndroidVideoDevices::findVideoInputs() const
{
    QList<QCameraDevice> devices;

    QJniObject deviceManager(QtJniTypes::Traits<QtJniTypes::QtVideoDeviceManager>::className(),
                             QNativeInterface::QAndroidApplication::context());

    if (!deviceManager.isValid()) {
        qCWarning(qLCAndroidVideoDevices) << "Failed to connect to Qt Video Device Manager.";
        return devices;
    }

    const QJniArray cameraIdList = deviceManager.callMethod<QtJniTypes::String[]>("getCameraIdList");
    if (!cameraIdList.isValid())
        return devices;

    int cameraIndex = 0;
    for (const auto &cameraId : cameraIdList) {
        if (!cameraId.isValid())
            continue;

        auto info = std::make_unique<QCameraDevicePrivate>();
        info->id = cameraId.toString().toUtf8();

        info->orientation = deviceManager.callMethod<jint>("getSensorOrientation", cameraId);

        // Will be set to -1 if facing can not be determined.
        const int facing = deviceManager.callMethod<jint>("getLensFacing", cameraId);

        // Values grabbed from Android docs CameraCharacteristics.LENS_FACING
        constexpr int LENS_FACING_FRONT = 0;
        constexpr int LENS_FACING_BACK = 1;
        constexpr int LENS_FACING_EXTERNAL = 2;

        switch (facing) {
        case LENS_FACING_EXTERNAL:
            info->position = QCameraDevice::Position::UnspecifiedPosition;
            info->description = QStringLiteral(u"External Camera: %1").arg(cameraIndex);
            break;
        case LENS_FACING_BACK:
            info->position = QCameraDevice::Position::BackFace;
            info->description = QStringLiteral(u"Rear Camera: %1").arg(cameraIndex);
            break;
        case LENS_FACING_FRONT:
            info->position = QCameraDevice::Position::FrontFace;
            info->description = QStringLiteral(u"Front Camera: %1").arg(cameraIndex);
            break;
        default:
            info->position = QCameraDevice::Position::UnspecifiedPosition;
            info->description = QStringLiteral(u"Camera: %1").arg(cameraIndex);
            break;
        }
        ++cameraIndex;

        const auto fpsRanges = deviceManager.callMethod<QStringList>("getFpsRange", cameraId);

        int maxFps = 0, minFps = 0;
        for (auto range : fpsRanges) {
            range = range.remove(u"["_s);
            range = range.remove(u"]"_s);

            const auto split = range.split(u","_s);

            int min = split.at(0).toInt();
            int max = split.at(1).toInt();

            if (max > maxFps) {
                maxFps = max;
                minFps = min;
            }
        }

        const static int imageFormat =
                QJniObject::getStaticField<QtJniTypes::ImageFormat, jint>("YUV_420_888");

        const QStringList sizes = deviceManager.callMethod<QStringList>(
                "getStreamConfigurationsSizes", cameraId, imageFormat);

        if (sizes.isEmpty())
            continue;

        for (const auto &sizeString : sizes) {
            const auto split = sizeString.split(u"x"_s);

            int width = split.at(0).toInt();
            int height = split.at(1).toInt();

            info->videoFormats.append(createCameraFormat(width, height, minFps, maxFps));
        }


        devices.push_back(info.release()->create());
    }

    return devices;
}

// Called from main looper thread in Android
static void onCameraAvailableNative(
    JNIEnv*,
    jobject,
    jlong nativePtr)
{
    auto* videoDevices = reinterpret_cast<QAndroidVideoDevices*>(static_cast<size_t>(nativePtr));
    videoDevices->onVideoInputsChanged();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCameraAvailableNative)

// Called from main looper thread in Android
static void onCameraUnavailableNative(
    JNIEnv*,
    jobject,
    jlong nativePtr)
{
    auto* videoDevices = reinterpret_cast<QAndroidVideoDevices*>(static_cast<size_t>(nativePtr));
    videoDevices->onVideoInputsChanged();
}
Q_DECLARE_JNI_NATIVE_METHOD(onCameraUnavailableNative)

void QAndroidVideoDevices::registerNativeMethods() {
    QJniEnvironment().registerNativeMethods(
        QtJniTypes::Traits<QtJniTypes::QtCameraAvailabilityListener>::className(),
        {
            Q_JNI_NATIVE_METHOD(onCameraAvailableNative),
            Q_JNI_NATIVE_METHOD(onCameraUnavailableNative),
        });
}

QT_END_NAMESPACE
