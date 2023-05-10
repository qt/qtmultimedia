// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidcamera_p.h"
#include "qandroidcamerasession_p.h"
#include "qandroidcapturesession_p.h"
#include "qandroidmediacapturesession_p.h"
#include <qmediadevices.h>
#include <qcameradevice.h>
#include <qtimer.h>
#include "qandroidmultimediautils_p.h"

QT_BEGIN_NAMESPACE

QAndroidCamera::QAndroidCamera(QCamera *camera)
    : QPlatformCamera(camera)
{
    Q_ASSERT(camera);
}

QAndroidCamera::~QAndroidCamera()
{
}

void QAndroidCamera::setActive(bool active)
{
    if (m_cameraSession) {
        m_cameraSession->setActive(active);
    } else {
        isPendingSetActive = active;
    }
}

bool QAndroidCamera::isActive() const
{
    return m_cameraSession ? m_cameraSession->isActive() : false;
}

void QAndroidCamera::setCamera(const QCameraDevice &camera)
{
    m_cameraDev = camera;

    if (m_cameraSession) {
        int id = 0;
        auto cameras = QMediaDevices::videoInputs();
        for (int i = 0; i < cameras.size(); ++i) {
            if (cameras.at(i) == camera) {
                id = i;
                break;
            }
        }
        if (id != m_cameraSession->getSelectedCameraId()) {
            m_cameraSession->setSelectedCameraId(id);
            reactivateCameraSession();
        }
    }
}

void QAndroidCamera::reactivateCameraSession()
{
    if (m_cameraSession->isActive()) {
        if (m_service->captureSession() &&
                m_service->captureSession()->state() == QMediaRecorder::RecordingState) {
            m_service->captureSession()->stop();
            qWarning() << "Changing camera during recording not supported";
        }
        m_cameraSession->setActive(false);
        m_cameraSession->setActive(true);
    }
}

bool QAndroidCamera::setCameraFormat(const QCameraFormat &format)
{
    m_cameraFormat = format;

    if (m_cameraSession)
        m_cameraSession->setCameraFormat(m_cameraFormat);

    return true;
}

void QAndroidCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QAndroidMediaCaptureSession *captureSession = static_cast<QAndroidMediaCaptureSession *>(session);
    if (m_service == captureSession)
        return;

    m_service = captureSession;
    if (!m_service) {
        disconnect(m_cameraSession,nullptr,this,nullptr);
        m_cameraSession = nullptr;
        return;
    }

    m_cameraSession = m_service->cameraSession();
    Q_ASSERT(m_cameraSession);
    if (!m_cameraFormat.isNull())
        m_cameraSession->setCameraFormat(m_cameraFormat);

    setCamera(m_cameraDev);

    connect(m_cameraSession, &QAndroidCameraSession::activeChanged, this, &QAndroidCamera::activeChanged);
    connect(m_cameraSession, &QAndroidCameraSession::error, this, &QAndroidCamera::error);
    connect(m_cameraSession, &QAndroidCameraSession::opened, this, &QAndroidCamera::onCameraOpened);

    if (isPendingSetActive) {
        setActive(true);
        isPendingSetActive = false;
    }
}

void QAndroidCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (!m_cameraSession || !m_cameraSession->camera())
        return;

    if (isFocusModeSupported(mode)) {
        QString focusMode;

        switch (mode) {
        case QCamera::FocusModeHyperfocal:
            focusMode = QLatin1String("edof");
            break;
        case QCamera::FocusModeInfinity: // not 100%, but close
            focusMode = QLatin1String("infinity");
            break;
        case QCamera::FocusModeManual:
            focusMode = QLatin1String("fixed");
            break;
        case QCamera::FocusModeAutoNear:
            focusMode = QLatin1String("macro");
            break;
        case QCamera::FocusModeAuto:
        case QCamera::FocusModeAutoFar:
            if (1) { // ###?
                focusMode = QLatin1String("continuous-video");
            } else {
                focusMode = QLatin1String("continuous-picture");
            }
            break;
        }

        m_cameraSession->camera()->setFocusMode(focusMode);

        // reset focus position
        m_cameraSession->camera()->cancelAutoFocus();

        focusModeChanged(mode);
    }
}

bool QAndroidCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    return (m_cameraSession && m_cameraSession->camera()) ? m_supportedFocusModes.contains(mode) : false;
}

void QAndroidCamera::onCameraOpened()
{
    Q_ASSERT(m_cameraSession);
    connect(m_cameraSession->camera(), &AndroidCamera::previewSizeChanged, this, &QAndroidCamera::setCameraFocusArea);

    m_supportedFocusModes.clear();
    m_continuousPictureFocusSupported = false;
    m_continuousVideoFocusSupported = false;
    m_focusPointSupported = false;

    QStringList focusModes = m_cameraSession->camera()->getSupportedFocusModes();
    for (int i = 0; i < focusModes.size(); ++i) {
        const QString &focusMode = focusModes.at(i);
        if (focusMode == QLatin1String("continuous-picture")) {
            m_supportedFocusModes << QCamera::FocusModeAuto;
            m_continuousPictureFocusSupported = true;
        } else if (focusMode == QLatin1String("continuous-video")) {
            m_supportedFocusModes << QCamera::FocusModeAuto;
            m_continuousVideoFocusSupported = true;
        } else if (focusMode == QLatin1String("edof")) {
            m_supportedFocusModes << QCamera::FocusModeHyperfocal;
        } else if (focusMode == QLatin1String("fixed")) {
            m_supportedFocusModes << QCamera::FocusModeManual;
        } else if (focusMode == QLatin1String("infinity")) {
            m_supportedFocusModes << QCamera::FocusModeInfinity;
        } else if (focusMode == QLatin1String("macro")) {
            m_supportedFocusModes << QCamera::FocusModeAutoNear;
        }
    }

    if (m_cameraSession->camera()->getMaxNumFocusAreas() > 0)
        m_focusPointSupported = true;

    auto m = focusMode();
    if (!m_supportedFocusModes.contains(m))
        m = QCamera::FocusModeAuto;

    setFocusMode(m);
    setCustomFocusPoint(focusPoint());

    if (m_cameraSession->camera()->isZoomSupported()) {
        m_zoomRatios = m_cameraSession->camera()->getZoomRatios();
        qreal maxZoom = m_zoomRatios.last() / qreal(100);
        maximumZoomFactorChanged(maxZoom);
        zoomTo(1, -1);
    } else {
        m_zoomRatios.clear();
        maximumZoomFactorChanged(1.0);
    }

    m_minExposureCompensationIndex = m_cameraSession->camera()->getMinExposureCompensation();
    m_maxExposureCompensationIndex = m_cameraSession->camera()->getMaxExposureCompensation();
    m_exposureCompensationStep = m_cameraSession->camera()->getExposureCompensationStep();
    exposureCompensationRangeChanged(m_minExposureCompensationIndex*m_exposureCompensationStep,
                                     m_maxExposureCompensationIndex*m_exposureCompensationStep);

    m_supportedExposureModes.clear();
    QStringList sceneModes = m_cameraSession->camera()->getSupportedSceneModes();
    if (!sceneModes.isEmpty()) {
        for (int i = 0; i < sceneModes.size(); ++i) {
            const QString &sceneMode = sceneModes.at(i);
            if (sceneMode == QLatin1String("auto"))
                m_supportedExposureModes << QCamera::ExposureAuto;
            else if (sceneMode == QLatin1String("beach"))
                m_supportedExposureModes << QCamera::ExposureBeach;
            else if (sceneMode == QLatin1String("night"))
                m_supportedExposureModes << QCamera::ExposureNight;
            else if (sceneMode == QLatin1String("portrait"))
                m_supportedExposureModes << QCamera::ExposurePortrait;
            else if (sceneMode == QLatin1String("snow"))
                m_supportedExposureModes << QCamera::ExposureSnow;
            else if (sceneMode == QLatin1String("sports"))
                m_supportedExposureModes << QCamera::ExposureSports;
            else if (sceneMode == QLatin1String("action"))
                m_supportedExposureModes << QCamera::ExposureAction;
            else if (sceneMode == QLatin1String("landscape"))
                m_supportedExposureModes << QCamera::ExposureLandscape;
            else if (sceneMode == QLatin1String("night-portrait"))
                m_supportedExposureModes << QCamera::ExposureNightPortrait;
            else if (sceneMode == QLatin1String("theatre"))
                m_supportedExposureModes << QCamera::ExposureTheatre;
            else if (sceneMode == QLatin1String("sunset"))
                m_supportedExposureModes << QCamera::ExposureSunset;
            else if (sceneMode == QLatin1String("steadyphoto"))
                m_supportedExposureModes << QCamera::ExposureSteadyPhoto;
            else if (sceneMode == QLatin1String("fireworks"))
                m_supportedExposureModes << QCamera::ExposureFireworks;
            else if (sceneMode == QLatin1String("party"))
                m_supportedExposureModes << QCamera::ExposureParty;
            else if (sceneMode == QLatin1String("candlelight"))
                m_supportedExposureModes << QCamera::ExposureCandlelight;
            else if (sceneMode == QLatin1String("barcode"))
                m_supportedExposureModes << QCamera::ExposureBarcode;
        }
    }

    setExposureCompensation(exposureCompensation());
    setExposureMode(exposureMode());

    isFlashSupported = false;
    isFlashAutoSupported = false;
    isTorchSupported = false;

    QStringList flashModes = m_cameraSession->camera()->getSupportedFlashModes();
    for (int i = 0; i < flashModes.size(); ++i) {
        const QString &flashMode = flashModes.at(i);
        if (flashMode == QLatin1String("auto"))
            isFlashAutoSupported = true;
        else if (flashMode == QLatin1String("on"))
            isFlashSupported = true;
        else if (flashMode == QLatin1String("torch"))
            isTorchSupported = true;
    }

    setFlashMode(flashMode());

    m_supportedWhiteBalanceModes.clear();
    QStringList whiteBalanceModes = m_cameraSession->camera()->getSupportedWhiteBalance();
    for (int i = 0; i < whiteBalanceModes.size(); ++i) {
        const QString &wb = whiteBalanceModes.at(i);
        if (wb == QLatin1String("auto")) {
            m_supportedWhiteBalanceModes.insert(QCamera::WhiteBalanceAuto,
                                                QStringLiteral("auto"));
        } else if (wb == QLatin1String("cloudy-daylight")) {
            m_supportedWhiteBalanceModes.insert(QCamera::WhiteBalanceCloudy,
                                                QStringLiteral("cloudy-daylight"));
        } else if (wb == QLatin1String("daylight")) {
            m_supportedWhiteBalanceModes.insert(QCamera::WhiteBalanceSunlight,
                                                QStringLiteral("daylight"));
        } else if (wb == QLatin1String("fluorescent")) {
            m_supportedWhiteBalanceModes.insert(QCamera::WhiteBalanceFluorescent,
                                                QStringLiteral("fluorescent"));
        } else if (wb == QLatin1String("incandescent")) {
            m_supportedWhiteBalanceModes.insert(QCamera::WhiteBalanceTungsten,
                                                QStringLiteral("incandescent"));
        } else if (wb == QLatin1String("shade")) {
            m_supportedWhiteBalanceModes.insert(QCamera::WhiteBalanceShade,
                                                QStringLiteral("shade"));
        } else if (wb == QLatin1String("twilight")) {
            m_supportedWhiteBalanceModes.insert(QCamera::WhiteBalanceSunset,
                                                QStringLiteral("twilight"));
        } else if (wb == QLatin1String("warm-fluorescent")) {
            m_supportedWhiteBalanceModes.insert(QCamera::WhiteBalanceFlash,
                                                QStringLiteral("warm-fluorescent"));
        }
    }

}

//void QAndroidCameraFocusControl::onCameraCaptureModeChanged()
//{
//    if (m_cameraSession->camera() && m_focusMode == QCamera::FocusModeAudio) {
//        QString focusMode;
//        if ((m_cameraSession->captureMode().testFlag(QCamera::CaptureVideo) && m_continuousVideoFocusSupported)
//                || !m_continuousPictureFocusSupported) {
//            focusMode = QLatin1String("continuous-video");
//        } else {
//            focusMode = QLatin1String("continuous-picture");
//        }
//        m_cameraSession->camera()->setFocusMode(focusMode);
//        m_cameraSession->camera()->cancelAutoFocus();
//    }
//}

static QRect adjustedArea(const QRectF &area)
{
    // Qt maps focus points in the range (0.0, 0.0) -> (1.0, 1.0)
    // Android maps focus points in the range (-1000, -1000) -> (1000, 1000)
    // Converts an area in Qt coordinates to Android coordinates
    return QRect(-1000 + qRound(area.x() * 2000),
                 -1000 + qRound(area.y() * 2000),
                 qRound(area.width() * 2000),
                 qRound(area.height() * 2000))
        .intersected(QRect(-1000, -1000, 2000, 2000));
}

void QAndroidCamera::setCameraFocusArea()
{
    if (!m_cameraSession)
        return;

    QList<QRect> areas;
    auto focusPoint = customFocusPoint();
    if (QRectF(0., 0., 1., 1.).contains(focusPoint)) {
        // in FocusPointAuto mode, leave the area list empty
        // to let the driver choose the focus point.
        QSize viewportSize = m_cameraSession->camera()->previewSize();

        if (!viewportSize.isValid())
            return;

        // Set up a 50x50 pixel focus area around the focal point
        QSizeF focusSize(50.f / viewportSize.width(), 50.f / viewportSize.height());
        float x = qBound(qreal(0),
                         focusPoint.x() - (focusSize.width() / 2),
                         1.f - focusSize.width());
        float y = qBound(qreal(0),
                         focusPoint.y() - (focusSize.height() / 2),
                         1.f - focusSize.height());

        QRectF area(QPointF(x, y), focusSize);

        areas.append(adjustedArea(area));
    }
    m_cameraSession->camera()->setFocusAreas(areas);
}

void QAndroidCamera::zoomTo(float factor, float rate)
{
    Q_UNUSED(rate);

    if (zoomFactor() == factor)
        return;

    if (!m_cameraSession || !m_cameraSession->camera())
        return;

    factor = qBound(qreal(1), factor, maxZoomFactor());
    int validZoomIndex = qt_findClosestValue(m_zoomRatios, qRound(factor * 100));
    float newZoom = m_zoomRatios.at(validZoomIndex) / qreal(100);
    m_cameraSession->camera()->setZoom(validZoomIndex);
    zoomFactorChanged(newZoom);
}

void QAndroidCamera::setFlashMode(QCamera::FlashMode mode)
{
    if (!m_cameraSession || !m_cameraSession->camera())
        return;

    if (!isFlashModeSupported(mode))
        return;

    QString flashMode;
    if (mode == QCamera::FlashAuto)
        flashMode = QLatin1String("auto");
    else if (mode == QCamera::FlashOn)
        flashMode = QLatin1String("on");
    else // FlashOff
        flashMode = QLatin1String("off");

    m_cameraSession->camera()->setFlashMode(flashMode);
    flashModeChanged(mode);
}

bool QAndroidCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    if (!m_cameraSession || !m_cameraSession->camera())
        return false;
    switch (mode) {
    case QCamera::FlashOff:
        return true;
    case QCamera::FlashOn:
        return isFlashSupported;
    case QCamera::FlashAuto:
        return isFlashAutoSupported;
    }
}

bool QAndroidCamera::isFlashReady() const
{
    // Android doesn't have an API for that
    return true;
}

void QAndroidCamera::setTorchMode(QCamera::TorchMode mode)
{
    if (!m_cameraSession)
        return;
    auto *camera = m_cameraSession->camera();
    if (!camera || !isTorchSupported || mode == QCamera::TorchAuto)
        return;

    if (mode == QCamera::TorchOn) {
        camera->setFlashMode(QLatin1String("torch"));
    } else if (mode == QCamera::TorchOff) {
        // if torch was enabled, it first needs to be turned off before restoring the flash mode
        camera->setFlashMode(QLatin1String("off"));
        setFlashMode(flashMode());
    }
    torchModeChanged(mode);
}

bool QAndroidCamera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    if (!m_cameraSession || !m_cameraSession->camera())
        return false;
    switch (mode) {
    case QCamera::TorchOff:
        return true;
    case QCamera::TorchOn:
        return isTorchSupported;
    case QCamera::TorchAuto:
        return false;
    }
}

void QAndroidCamera::setExposureMode(QCamera::ExposureMode mode)
{
    if (exposureMode() == mode)
        return;

    if (!m_cameraSession || !m_cameraSession->camera())
        return;

    if (!m_supportedExposureModes.contains(mode))
        return;

    QString sceneMode;
    switch (mode) {
    case QCamera::ExposureAuto:
        sceneMode = QLatin1String("auto");
        break;
    case QCamera::ExposureSports:
        sceneMode = QLatin1String("sports");
        break;
    case QCamera::ExposurePortrait:
        sceneMode = QLatin1String("portrait");
        break;
    case QCamera::ExposureBeach:
        sceneMode = QLatin1String("beach");
        break;
    case QCamera::ExposureSnow:
        sceneMode = QLatin1String("snow");
        break;
    case QCamera::ExposureNight:
        sceneMode = QLatin1String("night");
        break;
    case QCamera::ExposureAction:
        sceneMode = QLatin1String("action");
        break;
    case QCamera::ExposureLandscape:
        sceneMode = QLatin1String("landscape");
        break;
    case QCamera::ExposureNightPortrait:
        sceneMode = QLatin1String("night-portrait");
        break;
    case QCamera::ExposureTheatre:
        sceneMode = QLatin1String("theatre");
        break;
    case QCamera::ExposureSunset:
        sceneMode = QLatin1String("sunset");
        break;
    case QCamera::ExposureSteadyPhoto:
        sceneMode = QLatin1String("steadyphoto");
        break;
    case QCamera::ExposureFireworks:
        sceneMode = QLatin1String("fireworks");
        break;
    case QCamera::ExposureParty:
        sceneMode = QLatin1String("party");
        break;
    case QCamera::ExposureCandlelight:
        sceneMode = QLatin1String("candlelight");
        break;
    case QCamera::ExposureBarcode:
        sceneMode = QLatin1String("barcode");
        break;
    default:
        sceneMode = QLatin1String("auto");
        mode = QCamera::ExposureAuto;
        break;
    }

    m_cameraSession->camera()->setSceneMode(sceneMode);
    exposureModeChanged(mode);
}

bool QAndroidCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    return m_supportedExposureModes.contains(mode);
}

void QAndroidCamera::setExposureCompensation(float bias)
{
    if (exposureCompensation() == bias || !m_cameraSession || !m_cameraSession->camera())
        return;

    int biasIndex = qRound(bias / m_exposureCompensationStep);
    biasIndex = qBound(m_minExposureCompensationIndex, biasIndex, m_maxExposureCompensationIndex);
    float comp = biasIndex * m_exposureCompensationStep;
    m_cameraSession->camera()->setExposureCompensation(biasIndex);
    exposureCompensationChanged(comp);
}

bool QAndroidCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    return m_supportedWhiteBalanceModes.contains(mode);
}

void QAndroidCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    if (!m_cameraSession)
        return;
    auto *camera = m_cameraSession->camera();
    if (!camera)
        return;
    QString wb = m_supportedWhiteBalanceModes.value(mode, QString());
    if (!wb.isEmpty()) {
        camera->setWhiteBalance(wb);
        whiteBalanceModeChanged(mode);
    }
}

QT_END_NAMESPACE

#include "moc_qandroidcamera_p.cpp"
