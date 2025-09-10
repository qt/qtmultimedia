// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Ruslan Baratov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDCAMERA_H
#define ANDROIDCAMERA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qobject.h>
#include <qsize.h>
#include <qrect.h>
#include <QtMultimedia/qcamera.h>
#include <QtCore/qjniobject.h>
#include <private/qcameradevice_p.h>

QT_BEGIN_NAMESPACE

class QThread;

class AndroidCameraPrivate;
class AndroidSurfaceTexture;
class AndroidSurfaceHolder;

class AndroidCamera : public QObject
{
    Q_OBJECT
public:
    enum CameraFacing {
        CameraFacingBack = 0,
        CameraFacingFront = 1
    };
    Q_ENUM(CameraFacing)

    enum ImageFormat { // same values as in android.graphics.ImageFormat Java class
        UnknownImageFormat = 0,
        RGB565 = 4,
        NV16 = 16,
        NV21 = 17,
        YUY2 = 20,
        JPEG = 256,
        YV12 = 842094169
    };
    Q_ENUM(ImageFormat)

    // http://developer.android.com/reference/android/hardware/Camera.Parameters.html#getSupportedPreviewFpsRange%28%29
    // "The values are multiplied by 1000 and represented in integers"
    struct FpsRange {
        int min;
        int max;

        FpsRange(): min(0), max(0) {}

        qreal getMinReal() const { return min / 1000.0; }
        qreal getMaxReal() const { return max / 1000.0; }

        static FpsRange makeFromQReal(qreal min, qreal max)
        {
            FpsRange range;
            range.min = static_cast<int>(min * 1000.0);
            range.max = static_cast<int>(max * 1000.0);
            return range;
        }
    };

    ~AndroidCamera();

    static AndroidCamera *open(int cameraId);

    int cameraId() const;

    bool lock();
    bool unlock();
    bool reconnect();
    void release();

    CameraFacing getFacing();
    int getNativeOrientation();

    QSize getPreferredPreviewSizeForVideo();
    QList<QSize> getSupportedPreviewSizes();

    QList<FpsRange> getSupportedPreviewFpsRange();

    FpsRange getPreviewFpsRange();
    void setPreviewFpsRange(FpsRange);

    ImageFormat getPreviewFormat();
    void setPreviewFormat(ImageFormat fmt);
    QList<ImageFormat> getSupportedPreviewFormats();

    QSize previewSize() const;
    QSize actualPreviewSize();
    void setPreviewSize(const QSize &size);
    bool setPreviewTexture(AndroidSurfaceTexture *surfaceTexture);
    bool setPreviewDisplay(AndroidSurfaceHolder *surfaceHolder);
    void setDisplayOrientation(int degrees);

    bool isZoomSupported();
    int getMaxZoom();
    QList<int> getZoomRatios();
    int getZoom();
    void setZoom(int value);

    QStringList getSupportedFlashModes();
    QString getFlashMode();
    void setFlashMode(const QString &value);

    QStringList getSupportedFocusModes();
    QString getFocusMode();
    void setFocusMode(const QString &value);

    int getMaxNumFocusAreas();
    QList<QRect> getFocusAreas();
    void setFocusAreas(const QList<QRect> &areas);

    void autoFocus();
    void cancelAutoFocus();

    bool isAutoExposureLockSupported();
    bool getAutoExposureLock();
    void setAutoExposureLock(bool toggle);

    bool isAutoWhiteBalanceLockSupported();
    bool getAutoWhiteBalanceLock();
    void setAutoWhiteBalanceLock(bool toggle);

    int getExposureCompensation();
    void setExposureCompensation(int value);
    float getExposureCompensationStep();
    int getMinExposureCompensation();
    int getMaxExposureCompensation();

    QStringList getSupportedSceneModes();
    QString getSceneMode();
    void setSceneMode(const QString &value);

    QStringList getSupportedWhiteBalance();
    QString getWhiteBalance();
    void setWhiteBalance(const QString &value);

    void setRotation(int rotation);
    int getRotation() const;

    QList<QCameraFormat> getSupportedFormats();
    QList<QSize> getSupportedPictureSizes();
    QList<QSize> getSupportedVideoSizes();
    void setPictureSize(const QSize &size);
    void setJpegQuality(int quality);

    void startPreview();
    void stopPreview();
    void stopPreviewSynchronous();

    void takePicture();

    void setupPreviewFrameCallback();
    void notifyNewFrames(bool notify);
    void fetchLastPreviewFrame();
    QJniObject getCameraObject();
    QJniObject getParametersObject();

    static int getNumberOfCameras();
    static void getCameraInfo(int id, QCameraDevicePrivate *info);
    static QVideoFrameFormat::PixelFormat QtPixelFormatFromAndroidImageFormat(AndroidCamera::ImageFormat);
    static AndroidCamera::ImageFormat AndroidImageFormatFromQtPixelFormat(QVideoFrameFormat::PixelFormat);
    static bool requestCameraPermission();

    static bool registerNativeMethods();
Q_SIGNALS:
    void previewSizeChanged();
    void previewStarted();
    void previewFailedToStart();
    void previewStopped();

    void autoFocusStarted();
    void autoFocusComplete(bool success);

    void whiteBalanceChanged();

    void takePictureFailed();
    void pictureExposed();
    void pictureCaptured(const QByteArray &frame, QVideoFrameFormat::PixelFormat format, QSize size, int bytesPerLine);
    void lastPreviewFrameFetched(const QVideoFrame &frame);
    void newPreviewFrame(const QVideoFrame &frame);

private:
    AndroidCamera(AndroidCameraPrivate *d, QThread *worker);

    Q_DECLARE_PRIVATE(AndroidCamera)
    AndroidCameraPrivate *d_ptr;
    QScopedPointer<QThread> m_worker;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(AndroidCamera::ImageFormat)

#endif // ANDROIDCAMERA_H
