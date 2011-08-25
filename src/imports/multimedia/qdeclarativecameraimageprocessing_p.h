/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QDECLARATIVECAMERAIMAGEPROCESSING_H
#define QDECLARATIVECAMERAIMAGEPROCESSING_H

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

#include <qcamera.h>
#include <qcameraimageprocessing.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeCamera;

class QDeclarativeCameraImageProcessing : public QObject
{
    Q_OBJECT
    Q_ENUMS(WhiteBalanceMode)

    Q_PROPERTY(WhiteBalanceMode whiteBalanceMode READ whiteBalanceMode WRITE setWhiteBalanceMode NOTIFY whiteBalanceModeChanged)
    Q_PROPERTY(int manualWhiteBalance READ manualWhiteBalance WRITE setManualWhiteBalance NOTIFY manualWhiteBalanceChanged)
    Q_PROPERTY(int contrast READ contrast WRITE setContrast NOTIFY contrastChanged)
    Q_PROPERTY(int saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
    Q_PROPERTY(int sharpeningLevel READ sharpeningLevel WRITE setSharpeningLevel NOTIFY sharpeningLevelChanged)
    Q_PROPERTY(int denoisingLevel READ denoisingLevel WRITE setDenoisingLevel NOTIFY denoisingLevelChanged)

public:
    enum WhiteBalanceMode {
        WhiteBalanceAuto = QCameraImageProcessing::WhiteBalanceAuto,
        WhiteBalanceManual = QCameraImageProcessing::WhiteBalanceManual,
        WhiteBalanceSunlight = QCameraImageProcessing::WhiteBalanceSunlight,
        WhiteBalanceCloudy = QCameraImageProcessing::WhiteBalanceCloudy,
        WhiteBalanceShade = QCameraImageProcessing::WhiteBalanceShade,
        WhiteBalanceTungsten = QCameraImageProcessing::WhiteBalanceTungsten,
        WhiteBalanceFluorescent = QCameraImageProcessing::WhiteBalanceFluorescent,
        WhiteBalanceFlash = QCameraImageProcessing::WhiteBalanceFlash,
        WhiteBalanceSunset = QCameraImageProcessing::WhiteBalanceSunset,
        WhiteBalanceVendor = QCameraImageProcessing::WhiteBalanceVendor
    };

    ~QDeclarativeCameraImageProcessing();

    WhiteBalanceMode whiteBalanceMode() const;
    int manualWhiteBalance() const;

    int contrast() const;
    int saturation() const;
    int sharpeningLevel() const;
    int denoisingLevel() const;

public Q_SLOTS:
    void setWhiteBalanceMode(QDeclarativeCameraImageProcessing::WhiteBalanceMode mode) const;
    void setManualWhiteBalance(int colorTemp) const;

    void setContrast(int value);
    void setSaturation(int value);
    void setSharpeningLevel(int value);
    void setDenoisingLevel(int value);

Q_SIGNALS:
    void whiteBalanceModeChanged(QDeclarativeCameraImageProcessing::WhiteBalanceMode) const;
    void manualWhiteBalanceChanged(int) const;

    void contrastChanged(int);
    void saturationChanged(int);
    void sharpeningLevelChanged(int);
    void denoisingLevelChanged(int);

private:
    friend class QDeclarativeCamera;
    QDeclarativeCameraImageProcessing(QCamera *camera, QObject *parent = 0);

    QCameraImageProcessing *m_imageProcessing;
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QT_PREPEND_NAMESPACE(QDeclarativeCameraImageProcessing))

QT_END_HEADER

#endif
