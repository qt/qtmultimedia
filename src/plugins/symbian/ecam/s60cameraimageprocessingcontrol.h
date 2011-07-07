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

#ifndef S60CAMERAIMAGEPROCESSINGCONTROL_H
#define S60CAMERAIMAGEPROCESSINGCONTROL_H

#include <qcameraimageprocessing.h>
#include <qcameraimageprocessingcontrol.h>

#include "s60camerasettings.h"

QT_USE_NAMESPACE

class S60CameraService;
class S60ImageCaptureSession;

/*
 * Control for image processing related camera operations (inc. white balance).
 */
class S60CameraImageProcessingControl : public QCameraImageProcessingControl
{
    Q_OBJECT

public: // Constructors & Destructor

    S60CameraImageProcessingControl(QObject *parent = 0);
    S60CameraImageProcessingControl(S60ImageCaptureSession *session, QObject *parent = 0);
    ~S60CameraImageProcessingControl();

public: // QCameraImageProcessingControl

    // White Balance
    QCameraImageProcessing::WhiteBalanceMode whiteBalanceMode() const;
    void setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode);
    bool isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const;

    // Processing Parameter
    bool isProcessingParameterSupported(ProcessingParameter parameter) const;
    QVariant processingParameter(QCameraImageProcessingControl::ProcessingParameter parameter) const;
    void setProcessingParameter(QCameraImageProcessingControl::ProcessingParameter parameter, QVariant value);

private slots: // Internal Slots

    void resetAdvancedSetting();

private: // Internal operations - Implementing ProcessingParameter

    // Manual White Balance (Color Temperature)
    int manualWhiteBalance() const;
    void setManualWhiteBalance(int colorTemperature);

    // Contrast
    int contrast() const;
    void setContrast(int value);

    // Brightness
    int brightness() const;
    void setBrightness(int value);

    // Saturation
    int saturation() const;
    void setSaturation(int value);

    // Sharpening
    bool isSharpeningSupported() const;
    int sharpeningLevel() const;
    void setSharpeningLevel(int value);

    // Denoising
    bool isDenoisingSupported() const;
    int denoisingLevel() const;
    void setDenoisingLevel(int value);

private: // Data

    S60ImageCaptureSession  *m_session;
    S60CameraSettings       *m_advancedSettings;
};

#endif // S60CAMERAIMAGEPROCESSINGCONTROL_H
