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

#ifndef AVFCAMERAVIEWFINDERSETTINGSCONTROL_H
#define AVFCAMERAVIEWFINDERSETTINGSCONTROL_H

#include <QtMultimedia/qcameraviewfindersettingscontrol.h>
#include <QtMultimedia/qcameraviewfindersettings.h>
#include <QtMultimedia/qvideoframe.h>

#include <QtCore/qpointer.h>
#include <QtCore/qglobal.h>
#include <QtCore/qsize.h>

@class AVCaptureDevice;
@class AVCaptureVideoDataOutput;
@class AVCaptureConnection;
@class AVCaptureDeviceFormat;

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;

class AVFCameraViewfinderSettingsControl2 : public QCameraViewfinderSettingsControl2
{
    Q_OBJECT

    friend class AVFCameraSession;
    friend class AVFCameraViewfinderSettingsControl;
public:
    AVFCameraViewfinderSettingsControl2(AVFCameraService *service);

    QList<QCameraViewfinderSettings> supportedViewfinderSettings() const override;
    QCameraViewfinderSettings viewfinderSettings() const override;
    void setViewfinderSettings(const QCameraViewfinderSettings &settings) override;

    // "Converters":
    static QVideoFrame::PixelFormat QtPixelFormatFromCVFormat(unsigned avPixelFormat);
    static bool CVPixelFormatFromQtFormat(QVideoFrame::PixelFormat qtFormat, unsigned &conv);

private:
    void setResolution(const QSize &resolution);
    void setFramerate(qreal minFPS, qreal maxFPS, bool useActive);
    void setPixelFormat(QVideoFrame::PixelFormat newFormat);
    AVCaptureDeviceFormat *findBestFormatMatch(const QCameraViewfinderSettings &settings) const;
    QVector<QVideoFrame::PixelFormat> viewfinderPixelFormats() const;
    bool convertPixelFormatIfSupported(QVideoFrame::PixelFormat format, unsigned &avfFormat) const;
    bool applySettings(const QCameraViewfinderSettings &settings);
    QCameraViewfinderSettings requestedSettings() const;

    AVCaptureConnection *videoConnection() const;

    AVFCameraService *m_service;
    QCameraViewfinderSettings m_settings;
};

class AVFCameraViewfinderSettingsControl : public QCameraViewfinderSettingsControl
{
    Q_OBJECT
public:
    AVFCameraViewfinderSettingsControl(AVFCameraService *service);

    bool isViewfinderParameterSupported(ViewfinderParameter parameter) const override;
    QVariant viewfinderParameter(ViewfinderParameter parameter) const override;
    void setViewfinderParameter(ViewfinderParameter parameter, const QVariant &value) override;

private:
    void setResolution(const QVariant &resolution);
    void setAspectRatio(const QVariant &aspectRatio);
    void setFrameRate(const QVariant &fps, bool max);
    void setPixelFormat(const QVariant &pf);
    bool initSettingsControl() const;

    AVFCameraService *m_service;
    mutable QPointer<AVFCameraViewfinderSettingsControl2> m_settingsControl;
};

QT_END_NAMESPACE

#endif
