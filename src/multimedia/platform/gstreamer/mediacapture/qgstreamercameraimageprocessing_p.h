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

#ifndef QGSTREAMERIMAGEPROCESSING_P_H
#define QGSTREAMERIMAGEPROCESSING_P_H

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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <qcamera.h>
#include <private/qplatformcameraimageprocessing_p.h>

#include <private/qgst_p.h>
#include <gst/video/colorbalance.h>

#if QT_CONFIG(gstreamer_photography)
# include <gst/interfaces/photography.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(linux_v4l)
class QGstreamerImageProcessingV4L2;
#endif

class QGstreamerCamera;

class QGstreamerImageProcessing : public QPlatformCameraImageProcessing
{
    Q_OBJECT

public:
    QGstreamerImageProcessing(QGstreamerCamera *camera);
    virtual ~QGstreamerImageProcessing();

    bool setWhiteBalanceMode(QCamera::WhiteBalanceMode mode);
    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const;

    bool isParameterSupported(ProcessingParameter) const override;
    bool isParameterValueSupported(ProcessingParameter parameter, const QVariant &value) const override;
    void setParameter(ProcessingParameter parameter, const QVariant &value) override;

    void update();

private:
    QGstreamerCamera *m_camera;
    QCamera::WhiteBalanceMode m_whiteBalanceMode = QCamera::WhiteBalanceAuto;

#if QT_CONFIG(linux_v4l)
    bool isV4L2Device = false;
    void initV4L2Controls();
    bool setV4L2Param(ProcessingParameter parameter, const QVariant &value);

public:
private:
    bool v4l2AutoWhiteBalanceSupported = false;
    bool v4l2ColorTemperatureSupported = false;
    qint32 v4l2MinColorTemp = 5600; // Daylight...
    qint32 v4l2MaxColorTemp = 5600;
    qint32 v4l2CurrentColorTemp = 5600;
#endif
};

QT_END_NAMESPACE

#endif // QGSTREAMERIMAGEPROCESSING_P_H
