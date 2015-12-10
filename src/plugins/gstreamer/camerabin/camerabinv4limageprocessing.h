/****************************************************************************
**
** Copyright (C) 2015 Denis Shienkov <denis.shienkov@gmail.com>
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

#ifndef CAMERABINV4LIMAGEPROCESSINGCONTROL_H
#define CAMERABINV4LIMAGEPROCESSINGCONTROL_H

#include <qcamera.h>
#include <qcameraimageprocessingcontrol.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinV4LImageProcessing : public QCameraImageProcessingControl
{
    Q_OBJECT

public:
    CameraBinV4LImageProcessing(CameraBinSession *session);
    virtual ~CameraBinV4LImageProcessing();

    bool isParameterSupported(ProcessingParameter) const;
    bool isParameterValueSupported(ProcessingParameter parameter, const QVariant &value) const;
    QVariant parameter(ProcessingParameter parameter) const;
    void setParameter(ProcessingParameter parameter, const QVariant &value);

public slots:
    void updateParametersInfo(QCamera::Status cameraStatus);

private:
    struct SourceParameterValueInfo {
        SourceParameterValueInfo()
            : cid(0)
        {
        }

        qint32 defaultValue;
        qint32 minimumValue;
        qint32 maximumValue;
        quint32 cid; // V4L control id
    };

    static qreal scaledImageProcessingParameterValue(
            qint32 sourceValue, const SourceParameterValueInfo &sourceValueInfo);
    static qint32 sourceImageProcessingParameterValue(
            qreal scaledValue, const SourceParameterValueInfo &valueRange);
private:
    CameraBinSession *m_session;
    QMap<ProcessingParameter, SourceParameterValueInfo> m_parametersInfo;
};

QT_END_NAMESPACE

#endif // CAMERABINV4LIMAGEPROCESSINGCONTROL_H
