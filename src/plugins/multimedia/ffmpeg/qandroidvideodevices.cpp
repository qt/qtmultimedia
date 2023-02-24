// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidvideodevices_p.h"

#include <private/qcameradevice_p.h>

#include <QtCore/QLoggingCategory>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qandroidextras_p.h>
#include <QtCore/qcoreapplication_platform.h>
#include <QJniEnvironment>
#include <jni.h>

static Q_LOGGING_CATEGORY(qLCAndroidVideoDevices, "qt.multimedia.ffmpeg.android.videoDevices")

Q_DECLARE_JNI_CLASS(QtVideoDeviceManager,
                    "org/qtproject/qt/android/multimedia/QtVideoDeviceManager");
Q_DECLARE_JNI_TYPE(StringArray, "[Ljava/lang/String;")
Q_DECLARE_JNI_CLASS(AndroidImageFormat, "android/graphics/ImageFormat");

QCameraFormat createCameraFormat(int width, int height, int fpsMin, int fpsMax)
{
    QCameraFormatPrivate *format = new QCameraFormatPrivate();

    format->resolution = { width, height };

    format->minFrameRate = fpsMin;
    format->maxFrameRate = fpsMax;

    format->pixelFormat = QVideoFrameFormat::PixelFormat::Format_YUV420P;

    return format->create();
}

QList<QCameraDevice> QAndroidVideoDevices::findVideoDevices()
{
    QList<QCameraDevice> devices;

    QJniObject deviceManager(QtJniTypes::className<QtJniTypes::QtVideoDeviceManager>(),
                             QNativeInterface::QAndroidApplication::context());

    if (!deviceManager.isValid()) {
        qCWarning(qLCAndroidVideoDevices) << "Failed to connect to Qt Video Device Manager.";
        return devices;
    }

    QJniObject cameraIdList = deviceManager.callMethod<QtJniTypes::StringArray>("getCameraIdList");

    QJniEnvironment jniEnv;
    int numCameras = jniEnv->GetArrayLength(cameraIdList.object<jarray>());
    if (jniEnv.checkAndClearExceptions())
        return devices;

    for (int cameraIndex = 0; cameraIndex < numCameras; cameraIndex++) {

        QJniObject cameraIdObject =
                jniEnv->GetObjectArrayElement(cameraIdList.object<jobjectArray>(), cameraIndex);
        if (jniEnv.checkAndClearExceptions())
            continue;

        jstring cameraId = cameraIdObject.object<jstring>();

        QCameraDevicePrivate *info = new QCameraDevicePrivate;
        info->id = cameraIdObject.toString().toUtf8();

        info->orientation = deviceManager.callMethod<jint>("getSensorOrientation", cameraId);

        int facing = deviceManager.callMethod<jint>("getLensFacing", cameraId);

        const int LENS_FACING_FRONT = 0;
        const int LENS_FACING_BACK = 1;
        const int LENS_FACING_EXTERNAL = 2;

        switch (facing) {
        case LENS_FACING_EXTERNAL:
        case LENS_FACING_BACK:
            info->position = QCameraDevice::BackFace;
            info->description = QString("Rear Camera: %1").arg(cameraIndex);
            break;
        case LENS_FACING_FRONT:
            info->position = QCameraDevice::FrontFace;
            info->description = QString("Front Camera: %1").arg(cameraIndex);
            break;
        }

        QJniObject fpsRangesObject =
                deviceManager.callMethod<QtJniTypes::StringArray>("getFpsRange", cameraId);
        jobjectArray fpsRanges = fpsRangesObject.object<jobjectArray>();

        int numRanges = jniEnv->GetArrayLength(fpsRanges);
        if (jniEnv.checkAndClearExceptions())
            continue;

        int maxFps = 0, minFps = 0;

        for (int rangeIndex = 0; rangeIndex < numRanges; rangeIndex++) {

            QJniObject rangeString = jniEnv->GetObjectArrayElement(fpsRanges, rangeIndex);
            if (jniEnv.checkAndClearExceptions())
                continue;

            QString range = rangeString.toString();

            range = range.remove("[");
            range = range.remove("]");

            auto split = range.split(",");

            int min = split[0].toInt();
            int max = split[1].toInt();

            if (max > maxFps) {
                maxFps = max;
                minFps = min;
            }
        }

        const static int imageFormat =
                QJniObject::getStaticField<QtJniTypes::AndroidImageFormat, jint>("YUV_420_888");

        QJniObject sizesObject = deviceManager.callMethod<QtJniTypes::StringArray>(
                "getStreamConfigurationsSizes", cameraId, imageFormat);

        jobjectArray streamSizes = sizesObject.object<jobjectArray>();
        int numSizes = jniEnv->GetArrayLength(streamSizes);
        if (jniEnv.checkAndClearExceptions())
            continue;

        for (int sizesIndex = 0; sizesIndex < numSizes; sizesIndex++) {

            QJniObject sizeStringObject = jniEnv->GetObjectArrayElement(streamSizes, sizesIndex);
            if (jniEnv.checkAndClearExceptions())
                continue;

            QString sizeString = sizeStringObject.toString();

            auto split = sizeString.split("x");

            int width = split[0].toInt();
            int height = split[1].toInt();

            info->videoFormats.append(createCameraFormat(width, height, minFps, maxFps));
        }

        devices.push_back(info->create());
    }

    return devices;
}
