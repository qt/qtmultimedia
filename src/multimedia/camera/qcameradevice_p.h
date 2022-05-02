/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QCAMERAINFO_P_H
#define QCAMERAINFO_P_H

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

#include <QtMultimedia/qcameradevice.h>
#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

class QCameraFormatPrivate : public QSharedData
{
public:
    QVideoFrameFormat::PixelFormat pixelFormat;
    QSize resolution;
    float minFrameRate = 0;
    float maxFrameRate = 0;

    QCameraFormat create() { return QCameraFormat(this); }
};

class QCameraDevicePrivate : public QSharedData
{
public:
    QByteArray id;
    QString description;
    bool isDefault = false;
    QCameraDevice::Position position = QCameraDevice::UnspecifiedPosition;
    int orientation = 0;
    QList<QSize> photoResolutions;
    QList<QCameraFormat> videoFormats;

    QCameraDevice create() { return QCameraDevice(this); }
};

QT_END_NAMESPACE

#endif // QCAMERAINFO_P_H
