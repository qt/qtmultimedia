/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef AVFIMAGEENCODERCONTROL_H
#define AVFIMAGEENCODERCONTROL_H

#include <QtMultimedia/qmediaencodersettings.h>
#include <QtMultimedia/qimageencodercontrol.h>

#include <QtCore/qglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qlist.h>

@class AVCaptureDeviceFormat;

QT_BEGIN_NAMESPACE

class AVFCameraService;

class AVFImageEncoderControl : public QImageEncoderControl
{
    Q_OBJECT

    friend class AVFCameraSession;
public:
    AVFImageEncoderControl(AVFCameraService *service);

    QStringList supportedImageCodecs() const override;
    QString imageCodecDescription(const QString &codecName) const override;
    QList<QSize> supportedResolutions(const QImageEncoderSettings &settings,
                                      bool *continuous) const override;
    QImageEncoderSettings imageSettings() const override;
    void setImageSettings(const QImageEncoderSettings &settings) override;

    QImageEncoderSettings requestedSettings() const;

private:
    AVFCameraService *m_service;
    QImageEncoderSettings m_settings;

    bool applySettings();
    bool videoCaptureDeviceIsValid() const;
};

QSize qt_image_high_resolution(AVCaptureDeviceFormat *fomat);

QT_END_NAMESPACE

#endif
