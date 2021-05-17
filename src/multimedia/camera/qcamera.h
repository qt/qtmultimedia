/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCAMERA_H
#define QCAMERA_H

#include <QtCore/qstringlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qsize.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>

#include <QtCore/qobject.h>

#include <QtMultimedia/qcameraimageprocessing.h>
#include <QtMultimedia/qcamerainfo.h>

#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE


class QCameraInfo;
class QPlatformMediaCaptureSession;
class QMediaCaptureSession;

class QCameraPrivate;
class Q_MULTIMEDIA_EXPORT QCamera : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QCamera::Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(QCameraImageProcessing* imageProcessing READ imageProcessing CONSTANT)
    Q_PROPERTY(QCameraInfo cameraInfo READ cameraInfo WRITE setCameraInfo NOTIFY cameraInfoChanged)
    Q_PROPERTY(Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(QCameraFormat cameraFormat READ cameraFormat WRITE setCameraFormat NOTIFY cameraFormatChanged)

    Q_PROPERTY(FocusMode focusMode READ focusMode WRITE setFocusMode)
    Q_PROPERTY(QPointF customFocusPoint READ customFocusPoint WRITE setCustomFocusPoint NOTIFY customFocusPointChanged)
    Q_PROPERTY(float focusDistance READ focusDistance WRITE setFocusDistance NOTIFY focusDistanceChanged)

    Q_PROPERTY(float minimumZoomFactor READ minimumZoomFactor NOTIFY minimumZoomFactorChanged)
    Q_PROPERTY(float maximumZoomFactor READ maximumZoomFactor NOTIFY maximumZoomFactorChanged)
    Q_PROPERTY(float zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(qreal aperture READ aperture NOTIFY apertureChanged)
    Q_PROPERTY(qreal shutterSpeed READ shutterSpeed NOTIFY shutterSpeedChanged)
    Q_PROPERTY(int isoSensitivity READ isoSensitivity NOTIFY isoSensitivityChanged)
    Q_PROPERTY(qreal exposureCompensation READ exposureCompensation WRITE setExposureCompensation NOTIFY exposureCompensationChanged)
    Q_PROPERTY(bool flashReady READ isFlashReady NOTIFY flashReady)
    Q_PROPERTY(QCamera::FlashMode flashMode READ flashMode WRITE setFlashMode)
    Q_PROPERTY(QCamera::TorchMode torchMode READ torchMode WRITE setTorchMode)
    Q_PROPERTY(QCamera::ExposureMode exposureMode READ exposureMode WRITE setExposureMode)

    Q_ENUMS(Status)
    Q_ENUMS(Error)

    Q_ENUMS(FocusMode)

    Q_ENUMS(FlashMode)
    Q_ENUMS(TorchMode)
    Q_ENUMS(ExposureMode)

public:
    enum Status {
        UnavailableStatus,
        InactiveStatus,
        StartingStatus,
        StoppingStatus,
        ActiveStatus
    };

    enum Error
    {
        NoError,
        CameraError
    };

    enum FocusMode {
        FocusModeAuto,
        FocusModeAutoNear,
        FocusModeAutoFar,
        FocusModeHyperfocal,
        FocusModeInfinity,
        FocusModeManual
    };

    enum FlashMode {
        FlashOff,
        FlashOn,
        FlashAuto
    };

    enum TorchMode {
        TorchOff,
        TorchOn,
        TorchAuto
    };

    enum ExposureMode {
        ExposureAuto,
        ExposureManual,
        ExposurePortrait,
        ExposureNight,
        ExposureSports,
        ExposureSnow,
        ExposureBeach,
        ExposureAction,
        ExposureLandscape,
        ExposureNightPortrait,
        ExposureTheatre,
        ExposureSunset,
        ExposureSteadyPhoto,
        ExposureFireworks,
        ExposureParty,
        ExposureCandlelight,
        ExposureBarcode
    };

    explicit QCamera(QObject *parent = nullptr);
    explicit QCamera(const QCameraInfo& cameraInfo, QObject *parent = nullptr);
    explicit QCamera(QCameraInfo::Position position, QObject *parent = nullptr);
    ~QCamera();

    bool isAvailable() const;
    bool isActive() const;

    Status status() const;

    QMediaCaptureSession *captureSession() const;

    QCameraInfo cameraInfo() const;
    void setCameraInfo(const QCameraInfo &cameraInfo);

    QCameraFormat cameraFormat() const;
    void setCameraFormat(const QCameraFormat &format);

    QCameraImageProcessing *imageProcessing() const;

    Error error() const;
    QString errorString() const;

    FocusMode focusMode() const;
    void setFocusMode(FocusMode mode);
    bool isFocusModeSupported(FocusMode mode) const;

    QPointF focusPoint() const;

    QPointF customFocusPoint() const;
    void setCustomFocusPoint(const QPointF &point);
    bool isCustomFocusPointSupported() const;

    void setFocusDistance(float d);
    float focusDistance() const;

    float minimumZoomFactor() const;
    float maximumZoomFactor() const;
    float zoomFactor() const;
    void setZoomFactor(float factor);

    FlashMode flashMode() const;
    bool isFlashModeSupported(FlashMode mode) const;
    bool isFlashReady() const;

    TorchMode torchMode() const;
    bool isTorchModeSupported(TorchMode mode) const;

    ExposureMode exposureMode() const;
    bool isExposureModeSupported(ExposureMode mode) const;

    qreal exposureCompensation() const;

    int isoSensitivity() const;
    qreal aperture() const;
    qreal shutterSpeed() const;

    int requestedIsoSensitivity() const;
    qreal requestedAperture() const;
    qreal requestedShutterSpeed() const;

    QList<int> supportedIsoSensitivities(bool *continuous = nullptr) const;
    QList<qreal> supportedApertures(bool *continuous = nullptr) const;
    QList<qreal> supportedShutterSpeeds(bool *continuous = nullptr) const;

public Q_SLOTS:
    void setActive(bool active);
    void start() { setActive(true); }
    void stop() { setActive(false); }

    void zoomTo(float zoom, float rate);

    void setFlashMode(FlashMode mode);
    void setTorchMode(TorchMode mode);
    void setExposureMode(ExposureMode mode);

    void setExposureCompensation(qreal ev);

    void setManualIsoSensitivity(int iso);
    void setAutoIsoSensitivity();

    void setManualAperture(qreal aperture);
    void setAutoAperture();

    void setManualShutterSpeed(qreal seconds);
    void setAutoShutterSpeed();

Q_SIGNALS:
    void activeChanged(bool);
    void statusChanged(QCamera::Status status);
    void errorChanged();
    void errorOccurred(QCamera::Error error, const QString &errorString);
    void cameraInfoChanged();
    void cameraFormatChanged();

    void focusModeChanged();
    void zoomFactorChanged(float);
    void minimumZoomFactorChanged(float);
    void maximumZoomFactorChanged(float);
    void focusDistanceChanged(float);
    void customFocusPointChanged();

    void flashReady(bool);

    void apertureChanged(qreal);
    void apertureRangeChanged();
    void shutterSpeedChanged(qreal speed);
    void shutterSpeedRangeChanged();
    void isoSensitivityChanged(int);
    void exposureCompensationChanged(qreal);

private:
    void setCaptureSession(QMediaCaptureSession *session);
    friend class QMediaCaptureSession;
    Q_DISABLE_COPY(QCamera)
    Q_DECLARE_PRIVATE(QCamera)
    Q_PRIVATE_SLOT(d_func(), void _q_error(int, const QString &))
    friend class QCameraInfo;
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QCamera, Status)
Q_MEDIA_ENUM_DEBUG(QCamera, Error)

#endif  // QCAMERA_H
