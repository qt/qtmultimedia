/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "s60camerasettings.h"
#include "s60cameraconstants.h"

// S60 3.2 Platform
#ifdef USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER
#define POST_31_PLATFORM
#include <ecamadvancedsettings.h> // CCameraAdvancedSettings (inc. TValueInfo)
#endif // S60 3.2

// S60 5.0 or later
#ifdef USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER
#define POST_31_PLATFORM
#include <ecamadvsettings.h>    // CCameraAdvancedSettings
#include <ecam/ecamconstants.h> // TValueInfo
#endif // S60 5.0 or later

S60CameraSettings::S60CameraSettings(QObject *parent, CCameraEngine *engine) :
    QObject(parent),
#ifndef S60_31_PLATFORM // Post S60 3.1 Platforms
    m_advancedSettings(0),
    m_imageProcessingSettings(0),
#endif // S60_31_PLATFORM
    m_cameraEngine(engine),
    m_continuousFocusing(false)
{
}

S60CameraSettings::~S60CameraSettings()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        delete m_advancedSettings;
        m_advancedSettings = 0;
    }

    if (m_imageProcessingSettings) {
        delete m_imageProcessingSettings;
        m_imageProcessingSettings = 0;
    }
#endif // POST_31_PLATFORM
}

/*
 * This is Symbian NewL kind of consructor, but unlike Symbian version this
 * constructor will not leave, but instead it will return possible errors in
 * the error variable. This is to be able to write the class without deriving
 * it form CBase. Also CleanupStack is cleaned here if the ConstructL leaves.
 */
S60CameraSettings* S60CameraSettings::New(int &error, QObject *parent, CCameraEngine *engine)
{
    S60CameraSettings* self = new S60CameraSettings(parent, engine);
    if (!self) {
        error = KErrNoMemory;
        return 0;
    }

    TRAPD(err, self->ConstructL());
    if (err) {
        // Clean created object
        delete self;
        self = 0;
        error = err;
        return 0;
    }

    error = KErrNone;
    return self;
}

void S60CameraSettings::ConstructL()
{
#ifdef POST_31_PLATFORM
    if (!m_cameraEngine)
        User::Leave(KErrGeneral);
    // From now on it is safe to assume engine exists

    // If no AdvancedSettings is available, there's no benefit of S60CameraSettings
    // Leave if creation fails
    m_advancedSettings = CCamera::CCameraAdvancedSettings::NewL(*m_cameraEngine->Camera());
    CleanupStack::PushL(m_advancedSettings);

    // ImageProcessing module may not be supported, don't Leave
    TRAPD(err, m_imageProcessingSettings = CCamera::CCameraImageProcessing::NewL(*m_cameraEngine->Camera()));
    if (err == KErrNone && m_imageProcessingSettings) {
        CleanupStack::PushL(m_imageProcessingSettings);
    } else {
        if (err == KErrNotSupported)
            m_imageProcessingSettings = 0;
        else {
            // Leave with error
            if (!m_imageProcessingSettings)
                User::Leave(KErrNoMemory);
            else
                User::Leave(err);
        }
    }

    if (m_advancedSettings) {
        RArray<TInt> digitalZoomFactors;
        CleanupClosePushL(digitalZoomFactors);

        TValueInfo info = ENotActive;
        m_advancedSettings->GetDigitalZoomStepsL(digitalZoomFactors, info);

        for (int i = 0; i < digitalZoomFactors.Count(); ++i)
            m_supportedSymbianDigitalZoomFactors << digitalZoomFactors[i];

        CleanupStack::PopAndDestroy(); // RArray<TInt> digitalZoomFactors
    }

    // Pop objects from CleanupStack
    if (m_imageProcessingSettings)
        CleanupStack::Pop(m_imageProcessingSettings);
    CleanupStack::Pop(m_advancedSettings);

#else // S60 3.1
    // AdvancedSettings are not suppoted on S60 3.1 (There's no use for S60CameraSettings)
    User::Leave(KErrNotSupported);
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setFocusMode(QCameraFocus::FocusMode mode)
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        switch (mode) {
            case QCameraFocus::ManualFocus: // Manual focus mode
                m_advancedSettings->SetFocusMode(CCamera::CCameraAdvancedSettings::EFocusModeManual);
                m_continuousFocusing = false;
                break;
            case QCameraFocus::AutoFocus: // Single-shot AutoFocus mode
                m_advancedSettings->SetFocusMode(CCamera::CCameraAdvancedSettings::EFocusModeAuto);
                m_advancedSettings->SetFocusRange(CCamera::CCameraAdvancedSettings::EFocusRangeAuto);
                m_continuousFocusing = false;
                break;
            case QCameraFocus::HyperfocalFocus:
                m_advancedSettings->SetFocusMode(CCamera::CCameraAdvancedSettings::EFocusModeAuto);
                m_advancedSettings->SetFocusRange(CCamera::CCameraAdvancedSettings::EFocusRangeHyperfocal);
                m_continuousFocusing = false;
                break;
            case QCameraFocus::InfinityFocus:
                m_advancedSettings->SetFocusMode(CCamera::CCameraAdvancedSettings::EFocusModeAuto);
                m_advancedSettings->SetFocusRange(CCamera::CCameraAdvancedSettings::EFocusRangeInfinite);
                m_continuousFocusing = false;
                break;
            case QCameraFocus::ContinuousFocus:
                m_advancedSettings->SetFocusMode(CCamera::CCameraAdvancedSettings::EFocusModeAuto);
                m_advancedSettings->SetFocusRange(CCamera::CCameraAdvancedSettings::EFocusRangeAuto);
                m_continuousFocusing = true;
                break;
            case QCameraFocus::MacroFocus:
                m_advancedSettings->SetFocusMode(CCamera::CCameraAdvancedSettings::EFocusModeAuto);
                m_advancedSettings->SetFocusRange(CCamera::CCameraAdvancedSettings::EFocusRangeMacro);
                m_continuousFocusing = false;
                break;

            default:
                emit error(QCamera::NotSupportedFeatureError, tr("Requested focus mode is not supported."));
                break;
        }
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
#else // S60 3.1
    Q_UNUSED(mode);
    emit error(QCamera::NotSupportedFeatureError, tr("Settings focus mode is not supported."));
#endif // POST_31_PLATFORM
}

void S60CameraSettings::startFocusing()
{
#ifdef POST_31_PLATFORM
    // Setting AutoFocusType triggers the focusing on Symbian
    if (m_advancedSettings) {
        if (m_continuousFocusing)
            m_advancedSettings->SetAutoFocusType(CCamera::CCameraAdvancedSettings::EAutoFocusTypeContinuous);
        else
            m_advancedSettings->SetAutoFocusType(CCamera::CCameraAdvancedSettings::EAutoFocusTypeSingle);
    } else {
        emit error(QCamera::CameraError, tr("Unable to focus."));
    }
#endif // POST_31_PLATFORM
}

void S60CameraSettings::cancelFocusing()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings)
        m_advancedSettings->SetAutoFocusType(CCamera::CCameraAdvancedSettings::EAutoFocusTypeOff);
    else
        emit error(QCamera::CameraError, tr("Unable to cancel focusing."));
#endif // POST_31_PLATFORM
}

QCameraFocus::FocusMode S60CameraSettings::focusMode()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        // First request needed info
        CCamera::CCameraAdvancedSettings::TFocusMode mode = m_advancedSettings->FocusMode();
        CCamera::CCameraAdvancedSettings::TFocusRange range = m_advancedSettings->FocusRange();
        CCamera::CCameraAdvancedSettings::TAutoFocusType autoType = m_advancedSettings->AutoFocusType();

        switch (mode) {
        case CCamera::CCameraAdvancedSettings::EFocusModeManual:
        case CCamera::CCameraAdvancedSettings::EFocusModeFixed:
            return QCameraFocus::ManualFocus;

        case CCamera::CCameraAdvancedSettings::EFocusModeAuto:
            if (autoType == CCamera::CCameraAdvancedSettings::EAutoFocusTypeContinuous) {
                return QCameraFocus::ContinuousFocus;
            } else {
                // Single-shot focusing
                switch (range) {
                case CCamera::CCameraAdvancedSettings::EFocusRangeMacro:
                case CCamera::CCameraAdvancedSettings::EFocusRangeSuperMacro:
                    return QCameraFocus::MacroFocus;
                case CCamera::CCameraAdvancedSettings::EFocusRangeHyperfocal:
                    return QCameraFocus::HyperfocalFocus;
                case CCamera::CCameraAdvancedSettings::EFocusRangeInfinite:
                    return QCameraFocus::InfinityFocus;
                case CCamera::CCameraAdvancedSettings::EFocusRangeAuto:
                case CCamera::CCameraAdvancedSettings::EFocusRangeNormal:
                    return QCameraFocus::AutoFocus;

                default:
                    return QCameraFocus::AutoFocus;
                }
            }
        default:
            return QCameraFocus::AutoFocus; // Return automatic focusing
        }
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
#endif // POST_31_PLATFORM
    return QCameraFocus::AutoFocus; // Return automatic focusing
}

QCameraFocus::FocusModes S60CameraSettings::supportedFocusModes()
{
    QCameraFocus::FocusModes modes = 0;

#ifdef POST_31_PLATFORM
    TInt supportedModes = 0;
    TInt autoFocusTypes = 0;
    TInt supportedRanges = 0;

    if (m_advancedSettings) {
        supportedModes = m_advancedSettings->SupportedFocusModes();
        autoFocusTypes = m_advancedSettings->SupportedAutoFocusTypes();
        supportedRanges = m_advancedSettings->SupportedFocusRanges();

        if (supportedModes == 0 || autoFocusTypes == 0 || supportedRanges == 0)
            return modes;

        // EFocusModeAuto is the only supported on Symbian
        if (supportedModes & CCamera::CCameraAdvancedSettings::EFocusModeAuto) {
            // Check supported types (Single-shot Auto vs. Continuous)
            if (autoFocusTypes & CCamera::CCameraAdvancedSettings::EAutoFocusTypeSingle)
                modes |= QCameraFocus::AutoFocus;
            if (autoFocusTypes & CCamera::CCameraAdvancedSettings::EAutoFocusTypeContinuous)
                modes |= QCameraFocus::ContinuousFocus;

            // Check supported ranges (Note! Some are actually fixed focuses
            // even though the mode is Auto on Symbian)
            if (supportedRanges & CCamera::CCameraAdvancedSettings::EFocusRangeMacro)
                modes |= QCameraFocus::MacroFocus;
            if (supportedRanges & CCamera::CCameraAdvancedSettings::EFocusRangeHyperfocal)
                modes |= QCameraFocus::HyperfocalFocus;
            if (supportedRanges & CCamera::CCameraAdvancedSettings::EFocusRangeInfinite)
                modes |= QCameraFocus::InfinityFocus;
        }
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
#endif // POST_31_PLATFORM

    return modes;
}

qreal S60CameraSettings::opticalZoomFactorL() const
{
    // Not supported on Symbian
    return 1.0;
}

void S60CameraSettings::setOpticalZoomFactorL(const qreal zoomFactor)
{
    // Not supported on Symbian
    Q_UNUSED(zoomFactor);
}

QList<qreal> S60CameraSettings::supportedDigitalZoomFactors() const
{
    QList<qreal> zoomFactors;
    foreach (int factor, m_supportedSymbianDigitalZoomFactors)
        zoomFactors << qreal(factor) / KSymbianFineResolutionFactor;

    return zoomFactors;
}

qreal S60CameraSettings::digitalZoomFactorL() const
{
    qreal factor = 1.0;

#ifdef POST_31_PLATFORM
    int symbianFactor = 0;
    if (m_advancedSettings)
        symbianFactor = m_advancedSettings->DigitalZoom();
    else
        User::Leave(KErrNotSupported);

    if (symbianFactor != 0)
        factor = qreal(symbianFactor) / KSymbianFineResolutionFactor;
#endif // POST_31_PLATFORM

    return factor;
}

void S60CameraSettings::setDigitalZoomFactorL(const qreal zoomFactor)
{
#ifdef POST_31_PLATFORM
    int symbianFactor = zoomFactor * KSymbianFineResolutionFactor;

    // Find closest supported Symbian ZoomFactor if needed
    if (!m_supportedSymbianDigitalZoomFactors.contains(symbianFactor)) {
        int closestIndex = -1;
        int closestDiff = 1000000; // Sensible maximum
        for (int i = 0; i < m_supportedSymbianDigitalZoomFactors.count(); ++i) {
            int diff = abs(m_supportedSymbianDigitalZoomFactors.at(i) - symbianFactor);
            if (diff < closestDiff) {
                closestDiff = diff;
                closestIndex = i;
            }
        }
        if (closestIndex != -1)
            symbianFactor = m_supportedSymbianDigitalZoomFactors.at(closestIndex);
        else
            User::Leave(KErrGeneral);
    }
    if (m_advancedSettings)
        m_advancedSettings->SetDigitalZoom(symbianFactor);
    else
        User::Leave(KErrNotSupported);
#else // S60 3.1 Platform
    Q_UNUSED(zoomFactor);
    emit error(QCamera::NotSupportedFeatureError, tr("Settings digital zoom factor is not supported."));
#endif // POST_31_PLATFORM
}

// MCameraObserver2
void S60CameraSettings::HandleAdvancedEvent(const TECAMEvent& aEvent)
{
#ifdef POST_31_PLATFORM

    if (aEvent.iErrorCode != KErrNone) {
        switch (aEvent.iErrorCode) {
            case KErrECamCameraDisabled:
                emit error(QCamera::CameraError, tr("Unexpected camera error."));
                return;
            case KErrECamSettingDisabled:
                emit error(QCamera::CameraError, tr("Unexpected camera error."));
                return;
            case KErrECamParameterNotInRange:
                emit error(QCamera::NotSupportedFeatureError, tr("Requested value is not in supported range."));
                return;
            case KErrECamSettingNotSupported:
                emit error(QCamera::NotSupportedFeatureError, tr("Requested setting is not supported."));
                return;
            case KErrECamNotOptimalFocus:
                if (m_continuousFocusing)
                    emit focusStatusChanged(QCamera::Searching, QCamera::LockTemporaryLost);
                else
                    emit focusStatusChanged(QCamera::Unlocked, QCamera::LockFailed);
                return;
        }

        if (aEvent.iEventType == KUidECamEventCameraSettingFocusRange ||
            aEvent.iEventType == KUidECamEventCameraSettingAutoFocusType2) {
            emit focusStatusChanged(QCamera::Unlocked, QCamera::LockFailed);
            return;
        } else if (aEvent.iEventType == KUidECamEventCameraSettingIsoRate) {
            if (aEvent.iErrorCode == KErrNotSupported)
                emit error(QCamera::NotSupportedFeatureError, tr("Requested ISO value is not supported."));
            else
                emit error(QCamera::CameraError, tr("Setting ISO value failed."));
            return;
        } else if (aEvent.iEventType == KUidECamEventCameraSettingAperture) {
            if (aEvent.iErrorCode == KErrNotSupported)
                emit error(QCamera::NotSupportedFeatureError, tr("Requested aperture value is not supported."));
            else
                emit error(QCamera::CameraError, tr("Setting aperture value failed."));
            return;
        } else if (aEvent.iEventType == KUidECamEventCameraSettingExposureCompensation) {
            if (aEvent.iErrorCode == KErrNotSupported)
                emit error(QCamera::NotSupportedFeatureError, tr("Requested exposure compensation is not supported."));
            else
                emit error(QCamera::CameraError, tr("Setting exposure compensation failed."));
            return;
        } else if (aEvent.iEventType == KUidECamEventCameraSettingOpticalZoom ||
                   aEvent.iEventType == KUidECamEventCameraSettingDigitalZoom) {
            if (aEvent.iErrorCode == KErrNotSupported)
                return; // Discard
            else {
                emit error(QCamera::CameraError, tr("Setting zoom factor failed."));
                return;
            }
        } else if (aEvent.iEventType == KUidECamEventCameraSettingFocusMode) {
            if (aEvent.iErrorCode == KErrNotSupported)
                if (m_cameraEngine && m_cameraEngine->CurrentCameraIndex() != 0)
                    emit error(QCamera::NotSupportedFeatureError, tr("Focusing is not supported with this camera."));
                else
                    emit error(QCamera::NotSupportedFeatureError, tr("Requested focus mode is not supported."));
            else
                emit error(QCamera::CameraError, tr("Setting focus mode failed."));
            return;
        } else {
            emit error(QCamera::CameraError, tr("Unexpected camera error."));
            return;
        }
    }

    if (aEvent.iEventType == KUidECamEventCameraSettingExposureLock) {
        if (m_advancedSettings) {
            if (m_advancedSettings->ExposureLockOn())
                emit exposureStatusChanged(QCamera::Locked, QCamera::LockAcquired);
            else
                emit exposureStatusChanged(QCamera::Unlocked, QCamera::LockLost);
        }
        else
            emit exposureStatusChanged(QCamera::Unlocked, QCamera::LockLost);
    }
    else if (aEvent.iEventType == KUidECamEventCameraSettingAperture)
        emit apertureChanged();

    else if (aEvent.iEventType == KUidECamEventCameraSettingApertureRange)
        emit apertureRangeChanged();

    else if (aEvent.iEventType == KUidECamEventCameraSettingIsoRateType)
        emit isoSensitivityChanged();

    else if (aEvent.iEventType == KUidECamEventCameraSettingShutterSpeed)
        emit shutterSpeedChanged();

    else if (aEvent.iEventType == KUidECamEventCameraSettingExposureCompensationStep)
        emit evChanged();

    else if (aEvent.iEventType == KUidECamEventFlashReady)
        emit flashReady(true);

    else if (aEvent.iEventType == KUidECamEventFlashNotReady)
        emit flashReady(false);

    else if (aEvent.iEventType.iUid == KUidECamEventCameraSettingsOptimalFocusUidValue)
        emit focusStatusChanged(QCamera::Locked, QCamera::LockAcquired);

#else // S60 3.1 Platform
    Q_UNUSED(aEvent);
#endif // POST_31_PLATFORM
}

bool S60CameraSettings::isFlashReady()
{
    TBool isReady = false;
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        int flashErr = m_advancedSettings->IsFlashReady(isReady);
        if(flashErr != KErrNone) {
            if (flashErr != KErrNotSupported)
                emit error(QCamera::CameraError, tr("Unexpected error with flash."));
            return false;
        }
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
#endif
    return isReady;
}

QCameraExposure::MeteringMode S60CameraSettings::meteringMode()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        CCamera::CCameraAdvancedSettings::TMeteringMode mode = m_advancedSettings->MeteringMode();
        switch (mode) {
            case CCamera::CCameraAdvancedSettings::EMeteringModeCenterWeighted:
                return QCameraExposure::MeteringAverage;
            case CCamera::CCameraAdvancedSettings::EMeteringModeEvaluative:
                return QCameraExposure::MeteringMatrix;
            case CCamera::CCameraAdvancedSettings::EMeteringModeSpot:
                return QCameraExposure::MeteringSpot;

            default:
                return QCameraExposure::MeteringAverage;
        }
    }else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
        return QCameraExposure::MeteringAverage;
    }
#else // S60 3.1 Platform
    return QCameraExposure::MeteringAverage;
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setMeteringMode(QCameraExposure::MeteringMode mode)
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        switch(mode) {
            case QCameraExposure::MeteringAverage:
                m_advancedSettings->SetMeteringMode(CCamera::CCameraAdvancedSettings::EMeteringModeCenterWeighted);
                break;
            case QCameraExposure::MeteringMatrix:
                m_advancedSettings->SetMeteringMode(CCamera::CCameraAdvancedSettings::EMeteringModeEvaluative);
                break;
            case QCameraExposure::MeteringSpot:
                m_advancedSettings->SetMeteringMode(CCamera::CCameraAdvancedSettings::EMeteringModeSpot);
                break;
            default:
                break;
        }
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
#else // S60 3.1
    Q_UNUSED(mode);
    emit error(QCamera::NotSupportedFeatureError, tr("Setting metering mode is not supported."));
#endif // POST_31_PLATFORM
}

bool S60CameraSettings::isMeteringModeSupported(QCameraExposure::MeteringMode mode)
{
#ifdef POST_31_PLATFORM
    TInt supportedModes = 0;

    if (m_advancedSettings) {
        supportedModes = m_advancedSettings->SupportedMeteringModes();
        if (supportedModes == 0)
            return false;

        switch (mode) {
            case QCameraExposure::MeteringMatrix:
                if (supportedModes & CCamera::CCameraAdvancedSettings::EMeteringModeEvaluative)
                    return true;
                else
                    return false;
            case QCameraExposure::MeteringAverage:
                if (supportedModes & CCamera::CCameraAdvancedSettings::EMeteringModeCenterWeighted)
                    return true;
                else
                    return false;
            case QCameraExposure::MeteringSpot:
                if (supportedModes & CCamera::CCameraAdvancedSettings::EMeteringModeSpot)
                    return true;
                else
                    return false;

            default:
                return false;
        }
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
#else // S60 3.1
    Q_UNUSED(mode);
#endif // POST_31_PLATFORM

    return false;
}

int S60CameraSettings::isoSensitivity()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        CCamera::CCameraAdvancedSettings::TISORateType isoRateType;
        TInt param = 0;
        TInt isoRate = 0;
        TRAPD(err, m_advancedSettings->GetISORateL(isoRateType, param, isoRate));
        if (err)
            return 0;
        if (isoRate != KErrNotFound)
            return isoRate;
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
#endif // POST_31_PLATFORM
    return 0;
}

QList<int> S60CameraSettings::supportedIsoSensitivities()
{
    QList<int> isoSentitivities;
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        RArray<TInt> supportedIsoRates;
        CleanupClosePushL(supportedIsoRates);

        TRAPD(err, m_advancedSettings->GetSupportedIsoRatesL(supportedIsoRates));
        if (err != KErrNone) {
            if (err != KErrNotSupported) // Don's emit error if ISO is not supported
                emit error(QCamera::CameraError, tr("Failure while querying supported iso sensitivities."));
        } else {
            for (int i = 0; i < supportedIsoRates.Count(); ++i)
                isoSentitivities << supportedIsoRates[i];
        }
        CleanupStack::PopAndDestroy(); // RArray<TInt> supportedIsoRates
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }

    return isoSentitivities;
#else // S60 3.1 Platform
    return isoSentitivities;
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setManualIsoSensitivity(int iso)
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        TRAPD(err, m_advancedSettings->SetISORateL(CCamera::CCameraAdvancedSettings::EISOManual, iso));
        if (err)
            emit error(QCamera::CameraError, tr("Setting manual iso sensitivity failed."));
        return;
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
#else // S60 3.1 Platform
    Q_UNUSED(iso);
    emit error(QCamera::NotSupportedFeatureError, tr("Setting manual iso sensitivity is not supported."));
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setAutoIsoSensitivity()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        TRAPD(err, m_advancedSettings->SetISORateL(CCamera::CCameraAdvancedSettings::EISOAutoUnPrioritised, 0));
        if (err)
            emit error(QCamera::CameraError, tr("Setting auto iso sensitivity failed."));
        return;
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
#else // S60 3.1 Platform
    emit error(QCamera::NotSupportedFeatureError, tr("Setting auto iso sensitivity is not supported."));
#endif // POST_31_PLATFORM
}

qreal S60CameraSettings::aperture()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings)
        return qreal(m_advancedSettings->Aperture()) / KSymbianFineResolutionFactor;
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    return 0;
#else // S60 3.1 Platform
    return 0;
#endif // POST_31_PLATFORM
}

QList<qreal> S60CameraSettings::supportedApertures()
{
    QList<qreal> apertures;

#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        RArray<TInt> supportedApertures;
        TValueInfo info = ENotActive;

        TRAPD(err, m_advancedSettings->GetAperturesL(supportedApertures, info));
        if (err != KErrNone)
            if (err != KErrNotSupported)
                emit error(QCamera::CameraError, tr("Failure while querying supported apertures."));
        else {
            for (int i = 0; i < supportedApertures.Count(); i++) {
                qreal q = qreal(supportedApertures[i]) / KSymbianFineResolutionFactor;
                apertures.append(q);
            }
        }
        supportedApertures.Close();
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    return apertures;
#else // S60 3.1 Platform
    return apertures;
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setManualAperture(qreal aperture)
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        int symbianAperture = (aperture * KSymbianFineResolutionFactor); // KSymbianFineResolutionFactor = 100
        m_advancedSettings->SetAperture(symbianAperture);
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
#else // S60 3.1
    Q_UNUSED(aperture);
    emit error(QCamera::NotSupportedFeatureError, tr("Setting manual aperture is not supported."));
#endif // POST_31_PLATFORM
}

void S60CameraSettings::lockExposure(bool lock)
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        m_advancedSettings->SetExposureLockOn(lock);
        return;
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
#else // S60 3.1
    Q_UNUSED(lock);
    emit error(QCamera::NotSupportedFeatureError, tr("Locking exposure is not supported."));
#endif // POST_31_PLATFORM
}

bool S60CameraSettings::isExposureLocked()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings)
        return m_advancedSettings->ExposureLockOn();
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
#endif // POST_31_PLATFORM
    return false;
}

qreal S60CameraSettings::shutterSpeed()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        qreal shutterSpeed = qreal(m_advancedSettings->ShutterSpeed()) / 1000000.0;
        return shutterSpeed; // In seconds
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
    return 0;
#else // S60 3.1 Platform
    return 0;
#endif // POST_31_PLATFORM
}

QList<qreal> S60CameraSettings::supportedShutterSpeeds()
{
    QList<qreal> speeds;

#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        RArray<TInt> supportedSpeeds;
        TValueInfo info = ENotActive;

        TRAPD(err, m_advancedSettings->GetShutterSpeedsL(supportedSpeeds, info));
        if (err != KErrNone)
            if (err != KErrNotSupported)
                emit error(QCamera::CameraError, tr("Failure while querying supported shutter speeds."));
        else {
            for (int i = 0; i < supportedSpeeds.Count(); i++) {
                qreal q = qreal(supportedSpeeds[i]) / 1000000.0;
                speeds.append(q); // In seconds
            }
        }
        supportedSpeeds.Close();
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    return speeds;
#else // S60 3.1 Platform
    return speeds;
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setManualShutterSpeed(qreal speed)
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        TInt shutterSpeed = speed * 1000000; // From seconds to microseconds
        m_advancedSettings->SetShutterSpeed(shutterSpeed);
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
#else // S60 3.1
    emit error(QCamera::NotSupportedFeatureError, tr("Setting manual shutter speed is not supported."));
    Q_UNUSED(speed);
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setExposureCompensation(qreal ev)
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        TInt evStep = ev * KSymbianFineResolutionFactor;
        m_advancedSettings->SetExposureCompensationStep(evStep);
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
#else // S60 3.1 Platform
    Q_UNUSED(ev);
    emit error(QCamera::NotSupportedFeatureError, tr("Setting exposure compensation is not supported."));
#endif // POST_31_PLATFORM
}

qreal S60CameraSettings::exposureCompensation()
{
#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        TInt evStepSymbian = 0;
        m_advancedSettings->GetExposureCompensationStep(evStepSymbian);
        qreal evStep = evStepSymbian;
        evStep /= KSymbianFineResolutionFactor;
        return evStep;
    } else {
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    }
    return 0;
#else // S60 3.1 Platform
    return 0;
#endif // POST_31_PLATFORM
}

QList<qreal> S60CameraSettings::supportedExposureCompensationValues()
{
    QList<qreal> valueList;

#ifdef POST_31_PLATFORM
    if (m_advancedSettings) {
        RArray<TInt> evSteps;
        TValueInfo info;
        TRAPD(err, m_advancedSettings->GetExposureCompensationStepsL(evSteps, info));
        if (err) {
            if (err != KErrNotSupported)
                emit error(QCamera::CameraError, tr("Failure while querying supported exposure compensation values."));
            return valueList;
        }

        if (info == ENotActive || evSteps.Count() == 0) {
            // EV not supported, return empty list
            return valueList;
        }

        for (int i = 0; i < evSteps.Count(); ++i) {
            qreal appendValue = evSteps[i];
            appendValue /= KSymbianFineResolutionFactor;
            valueList.append(appendValue);
        }
    }
    else
        emit error(QCamera::CameraError, tr("Unexpected camera error."));
    return valueList;
#else // S60 3.1 Platform
    return valueList;
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setSharpeningLevel(int value)
{
#ifdef POST_31_PLATFORM
    if (m_imageProcessingSettings && isSharpeningSupported())
        m_imageProcessingSettings->SetTransformationValue(KUidECamEventImageProcessingAdjustSharpness, value);
    else
        emit error(QCamera::NotSupportedFeatureError, tr("Setting sharpening level is not supported."));
#else // S60 3.1
    Q_UNUSED(value);
    emit error(QCamera::NotSupportedFeatureError, tr("Setting sharpening level is not supported."));
#endif // POST_31_PLATFORM
}

bool S60CameraSettings::isSharpeningSupported() const
{
#ifdef POST_31_PLATFORM
    if (m_imageProcessingSettings) {
        RArray<TUid> suppTransforms;
        TRAPD(err, m_imageProcessingSettings->GetSupportedTransformationsL(suppTransforms));
        if (err)
            return false;

        if (suppTransforms.Find(KUidECamEventImageProcessingAdjustSharpness))
            return true;
    }
    return false;
#else // S60 3.1 Platform
    return false;
#endif // POST_31_PLATFORM
}

int S60CameraSettings::sharpeningLevel() const
{
#ifdef POST_31_PLATFORM
    if (m_imageProcessingSettings && isSharpeningSupported())
        return m_imageProcessingSettings->TransformationValue(KUidECamEventImageProcessingAdjustSharpness);
    else
        return 0;
#else // S60 3.1 Platform
    return 0;
#endif // POST_31_PLATFORM
}

void S60CameraSettings::setSaturation(int value)
{
#ifdef POST_31_PLATFORM
    if (m_imageProcessingSettings) {
        RArray<TUid> suppTransforms;
        TRAPD(err, m_imageProcessingSettings->GetSupportedTransformationsL(suppTransforms));
        if (err)
            if (err != KErrNotSupported)
                emit error(QCamera::CameraError, tr("Failure while querying supported transformations."));

        if (suppTransforms.Find(KUidECamEventtImageProcessingAdjustSaturation))
            m_imageProcessingSettings->SetTransformationValue(KUidECamEventtImageProcessingAdjustSaturation, value == -1 ? 0 : value*2-100);
        else
            emit error(QCamera::NotSupportedFeatureError, tr("Setting saturation is not supported."));
    }
    else
        emit error(QCamera::NotSupportedFeatureError, tr("Setting saturation is not supported."));
#else // S60 3.1
    Q_UNUSED(value);
    emit error(QCamera::NotSupportedFeatureError, tr("Setting saturation is not supported."));
#endif // POST_31_PLATFORM
}

int S60CameraSettings::saturation()
{
#ifdef POST_31_PLATFORM
    if (m_imageProcessingSettings) {
        RArray<TUid> suppTransforms;
        TRAPD(err, m_imageProcessingSettings->GetSupportedTransformationsL(suppTransforms));
        if (err)
            if (err != KErrNotSupported)
                emit error(QCamera::CameraError, tr("Failure while querying supported transformations."));

        if (suppTransforms.Find(KUidECamEventtImageProcessingAdjustSaturation))
            return m_imageProcessingSettings->TransformationValue(KUidECamEventtImageProcessingAdjustSaturation);
    }
    return 0;
#else // S60 3.1 Platform
    return 0;
#endif // POST_31_PLATFORM
}

// End of file
