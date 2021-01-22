/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Ruslan Baratov
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef ANDROIDCAMERA_H
#define ANDROIDCAMERA_H

#include <qobject.h>
#include <QtCore/private/qjni_p.h>
#include <qsize.h>
#include <qrect.h>
#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

class QThread;

class AndroidCameraPrivate;
class AndroidSurfaceTexture;
class AndroidSurfaceHolder;

struct AndroidCameraInfo
{
    QByteArray name;
    QString description;
    QCamera::Position position;
    int orientation;
};
Q_DECLARE_TYPEINFO(AndroidCameraInfo, Q_MOVABLE_TYPE);

class AndroidCamera : public QObject
{
    Q_OBJECT
    Q_ENUMS(CameraFacing)
    Q_ENUMS(ImageFormat)
public:
    enum CameraFacing {
        CameraFacingBack = 0,
        CameraFacingFront = 1
    };

    enum ImageFormat { // same values as in android.graphics.ImageFormat Java class
        UnknownImageFormat = 0,
        RGB565 = 4,
        NV16 = 16,
        NV21 = 17,
        YUY2 = 20,
        JPEG = 256,
        YV12 = 842094169
    };

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

    QList<QSize> getSupportedPictureSizes();
    void setPictureSize(const QSize &size);
    void setJpegQuality(int quality);

    void startPreview();
    void stopPreview();
    void stopPreviewSynchronous();

    void takePicture();

    void setupPreviewFrameCallback();
    void notifyNewFrames(bool notify);
    void fetchLastPreviewFrame();
    QJNIObjectPrivate getCameraObject();

    static int getNumberOfCameras();
    static void getCameraInfo(int id, AndroidCameraInfo *info);
    static bool requestCameraPermission();

    static bool initJNI(JNIEnv *env);

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
    void pictureCaptured(const QByteArray &data);
    void lastPreviewFrameFetched(const QVideoFrame &frame);
    void newPreviewFrame(const QVideoFrame &frame);

private:
    AndroidCamera(AndroidCameraPrivate *d, QThread *worker);

    Q_DECLARE_PRIVATE(AndroidCamera)
    AndroidCameraPrivate *d_ptr;
    QScopedPointer<QThread> m_worker;
};

Q_DECLARE_METATYPE(AndroidCamera::ImageFormat)

QT_END_NAMESPACE

#endif // ANDROIDCAMERA_H
