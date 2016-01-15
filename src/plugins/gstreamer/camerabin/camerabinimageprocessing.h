/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#ifdef HAVE_GST_PHOTOGRAPHY
# include <gst/interfaces/photography.h>
# if !GST_CHECK_VERSION(1,0,0)
typedef GstWhiteBalanceMode GstPhotographyWhiteBalanceMode;
typedef GstColourToneMode GstPhotographyColorToneMode;
# endif
#endif

QT_BEGIN_NAMESPACE

#ifdef USE_V4L
class CameraBinV4LImageProcessing;
#endif

class CameraBinSession;

class CameraBinImageProcessing : public QCameraImageProcessingControl
{
    Q_OBJECT

public:
    CameraBinImageProcessing(CameraBinSession *session);
    virtual ~CameraBinImageProcessing();

    QCameraImageProcessing::WhiteBalanceMode whiteBalanceMode() const;
    bool setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode);
    bool isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const;

    bool isParameterSupported(ProcessingParameter) const;
    bool isParameterValueSupported(ProcessingParameter parameter, const QVariant &value) const;
    QVariant parameter(ProcessingParameter parameter) const;
    void setParameter(ProcessingParameter parameter, const QVariant &value);

#ifdef HAVE_GST_PHOTOGRAPHY
    void lockWhiteBalance();
    void unlockWhiteBalance();
#endif

private:
    bool setColorBalanceValue(const QString& channel, qreal value);
    void updateColorBalanceValues();

private:
    CameraBinSession *m_session;
    QMap<QCameraImageProcessingControl::ProcessingParameter, int> m_values;
#ifdef HAVE_GST_PHOTOGRAPHY
    QMap<GstPhotographyWhiteBalanceMode, QCameraImageProcessing::WhiteBalanceMode> m_mappedWbValues;
    QMap<QCameraImageProcessing::ColorFilter, GstPhotographyColorToneMode> m_filterMap;
#endif
    QCameraImageProcessing::WhiteBalanceMode m_whiteBalanceMode;

#ifdef USE_V4L
    CameraBinV4LImageProcessing *m_v4lImageControl;
#endif
};

QT_END_NAMESPACE

#endif // CAMERABINIMAGEPROCESSINGCONTROL_H
