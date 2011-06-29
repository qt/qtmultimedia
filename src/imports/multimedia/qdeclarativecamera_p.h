/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVECAMERA_H
#define QDECLARATIVECAMERA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qgraphicsvideoitem.h"
#include <QtCore/qbasictimer.h>
#include <QtDeclarative/qdeclarativeitem.h>
#include <QtCore/QTime>

#include <qcamera.h>
#include <qcameraimageprocessing.h>
#include <qcameraimagecapture.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QTimerEvent;
class QVideoSurfaceFormat;


class QDeclarativeCamera : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(State cameraState READ cameraState WRITE setCameraState NOTIFY cameraStateChanged)
    Q_PROPERTY(LockStatus lockStatus READ lockStatus NOTIFY lockStatusChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

    Q_PROPERTY(QString capturedImagePath READ capturedImagePath NOTIFY imageSaved)

    Q_PROPERTY(int iso READ isoSensitivity WRITE setManualIsoSensitivity NOTIFY isoSensitivityChanged)
    Q_PROPERTY(qreal shutterSpeed READ shutterSpeed NOTIFY shutterSpeedChanged)
    Q_PROPERTY(qreal aperture READ aperture NOTIFY apertureChanged)
    Q_PROPERTY(qreal exposureCompensation READ exposureCompensation WRITE setExposureCompensation NOTIFY exposureCompensationChanged)

    Q_PROPERTY(ExposureMode exposureMode READ exposureMode WRITE setExposureMode NOTIFY exposureModeChanged)
    Q_PROPERTY(int flashMode READ flashMode WRITE setFlashMode NOTIFY flashModeChanged)
    Q_PROPERTY(WhiteBalanceMode whiteBalanceMode READ whiteBalanceMode WRITE setWhiteBalanceMode NOTIFY whiteBalanceModeChanged)
    Q_PROPERTY(int manualWhiteBalance READ manualWhiteBalance WRITE setManualWhiteBalance NOTIFY manualWhiteBalanceChanged)

    Q_PROPERTY(QSize captureResolution READ captureResolution WRITE setCaptureResolution NOTIFY captureResolutionChanged)

    Q_PROPERTY(qreal opticalZoom READ opticalZoom WRITE setOpticalZoom NOTIFY opticalZoomChanged)
    Q_PROPERTY(qreal maximumOpticalZoom READ maximumOpticalZoom NOTIFY maximumOpticalZoomChanged)
    Q_PROPERTY(qreal digitalZoom READ digitalZoom WRITE setDigitalZoom NOTIFY digitalZoomChanged)
    Q_PROPERTY(qreal maximumDigitalZoom READ maximumDigitalZoom NOTIFY maximumDigitalZoomChanged)

    Q_ENUMS(State)
    Q_ENUMS(LockStatus)
    Q_ENUMS(Error)
    Q_ENUMS(FlashMode)
    Q_ENUMS(ExposureMode)
    Q_ENUMS(WhiteBalanceMode)
public:
    enum State
    {
        ActiveState = QCamera::ActiveState,
        LoadedState = QCamera::LoadedState,
        UnloadedState = QCamera::UnloadedState
    };

    enum LockStatus
    {
        Unlocked = QCamera::Unlocked,
        Searching = QCamera::Searching,
        Locked = QCamera::Locked
    };

    enum Error
    {
        NoError = QCamera::NoError,
        CameraError = QCamera::CameraError,
        InvalidRequestError = QCamera::InvalidRequestError,
        ServiceMissingError = QCamera::ServiceMissingError,
        NotSupportedFeatureError = QCamera::NotSupportedFeatureError
    };

    enum FlashMode {
        FlashAuto = 0x1,
        FlashOff = 0x2,
        FlashOn = 0x4,
        FlashRedEyeReduction  = 0x8,
        FlashFill = 0x10,
        FlashTorch = 0x20,
        FlashSlowSyncFrontCurtain = 0x40,
        FlashSlowSyncRearCurtain = 0x80,
        FlashManual = 0x100
    };

    enum ExposureMode {
        ExposureAuto = 0,
        ExposureManual = 1,
        ExposurePortrait = 2,
        ExposureNight = 3,
        ExposureBacklight = 4,
        ExposureSpotlight = 5,
        ExposureSports = 6,
        ExposureSnow = 7,
        ExposureBeach = 8,
        ExposureLargeAperture = 9,
        ExposureSmallAperture = 10,
        ExposureModeVendor = 1000
    };

    enum WhiteBalanceMode {
        WhiteBalanceAuto = 0,
        WhiteBalanceManual = 1,
        WhiteBalanceSunlight = 2,
        WhiteBalanceCloudy = 3,
        WhiteBalanceShade = 4,
        WhiteBalanceTungsten = 5,
        WhiteBalanceFluorescent = 6,
        WhiteBalanceIncandescent = 7,
        WhiteBalanceFlash = 8,
        WhiteBalanceSunset = 9,
        WhiteBalanceVendor = 1000
    };

    QDeclarativeCamera(QDeclarativeItem *parent = 0);
    ~QDeclarativeCamera();

    State cameraState() const;

    Error error() const;
    QString errorString() const;

    LockStatus lockStatus() const;

    QImage capturedImagePreview() const;
    QString capturedImagePath() const;

    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);

    int flashMode() const;
    ExposureMode exposureMode() const;
    qreal exposureCompensation() const;
    int isoSensitivity() const;
    qreal shutterSpeed() const;
    qreal aperture() const;

    WhiteBalanceMode whiteBalanceMode() const;
    int manualWhiteBalance() const;

    QSize captureResolution() const;

    qreal maximumOpticalZoom() const;
    qreal maximumDigitalZoom() const;

    qreal opticalZoom() const;
    qreal digitalZoom() const;

public Q_SLOTS:
    void start();
    void stop();

    void setCameraState(State state);

    void searchAndLock();
    void unlock();

    void captureImage();

    void setFlashMode(int);
    void setExposureMode(QDeclarativeCamera::ExposureMode);
    void setExposureCompensation(qreal ev);
    void setManualIsoSensitivity(int iso);

    void setWhiteBalanceMode(QDeclarativeCamera::WhiteBalanceMode mode) const;
    void setManualWhiteBalance(int colorTemp) const;

    void setCaptureResolution(const QSize &size);

    void setOpticalZoom(qreal);
    void setDigitalZoom(qreal);

Q_SIGNALS:
    void errorChanged();
    void error(QDeclarativeCamera::Error error, const QString &errorString);

    void cameraStateChanged(QDeclarativeCamera::State);

    void lockStatusChanged();

    void imageCaptured(const QString &preview);
    void imageSaved(const QString &path);
    void captureFailed(const QString &message);

    void isoSensitivityChanged(int);
    void apertureChanged(qreal);
    void shutterSpeedChanged(qreal);
    void exposureCompensationChanged(qreal);
    void exposureModeChanged(QDeclarativeCamera::ExposureMode);
    void flashModeChanged(int);

    void whiteBalanceModeChanged(QDeclarativeCamera::WhiteBalanceMode) const;
    void manualWhiteBalanceChanged(int) const;

    void captureResolutionChanged(const QSize&);

    void opticalZoomChanged(qreal);
    void digitalZoomChanged(qreal);
    void maximumOpticalZoomChanged(qreal);
    void maximumDigitalZoomChanged(qreal);

protected:
    void geometryChanged(const QRectF &geometry, const QRectF &);
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent * event);

private Q_SLOTS:
    void _q_updateState(QCamera::State);
    void _q_nativeSizeChanged(const QSizeF &size);
    void _q_error(int, const QString &);
    void _q_imageCaptured(int, const QImage&);
    void _q_imageSaved(int, const QString&);
    void _q_captureFailed(int, QCameraImageCapture::Error, const QString&);
    void _q_updateFocusZones();
    void _q_updateLockStatus(QCamera::LockType, QCamera::LockStatus, QCamera::LockChangeReason);
    void _q_updateImageSettings();
    void _q_applyPendingState();

private:
    Q_DISABLE_COPY(QDeclarativeCamera)
    QCamera *m_camera;
    QGraphicsVideoItem *m_viewfinderItem;

    QCameraExposure *m_exposure;
    QCameraFocus *m_focus;
    QCameraImageCapture *m_capture;

    QImage m_capturedImagePreview;
    QString m_capturedImagePath;
    QList <QGraphicsItem*> m_focusZones;
    QTime m_focusFailedTime;

    QImageEncoderSettings m_imageSettings;
    bool m_imageSettingsChanged;

    State m_pendingState;
    bool m_isStateSet;
    bool m_isValid;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QT_PREPEND_NAMESPACE(QDeclarativeCamera))

QT_END_HEADER

#endif
