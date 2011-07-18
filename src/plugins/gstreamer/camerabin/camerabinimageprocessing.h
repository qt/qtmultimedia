/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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

#ifndef CAMERABINIMAGEPROCESSINGCONTROL_H
#define CAMERABINIMAGEPROCESSINGCONTROL_H

#include <qcamera.h>
#include <qcameraimageprocessingcontrol.h>

#include <gst/gst.h>
#include <glib.h>

#include <gst/interfaces/photography.h>
#include <gst/interfaces/colorbalance.h>

class CameraBinSession;

QT_USE_NAMESPACE

class CameraBinImageProcessing : public QCameraImageProcessingControl
{
    Q_OBJECT

public:
    CameraBinImageProcessing(CameraBinSession *session);
    virtual ~CameraBinImageProcessing();

    QCameraImageProcessing::WhiteBalanceMode whiteBalanceMode() const;
    void setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode);
    bool isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const;

    bool isProcessingParameterSupported(ProcessingParameter) const;
    QVariant processingParameter(ProcessingParameter parameter) const;
    void setProcessingParameter(ProcessingParameter parameter, QVariant value);

private:
    bool setColorBalanceValue(const QString& channel, int value);
    void updateColorBalanceValues();

private:
    CameraBinSession *m_session;
    QMap<QCameraImageProcessingControl::ProcessingParameter, int> m_values;
    QMap<GstWhiteBalanceMode, QCameraImageProcessing::WhiteBalanceMode> m_mappedWbValues;
};

#endif // CAMERABINIMAGEPROCESSINGCONTROL_H
