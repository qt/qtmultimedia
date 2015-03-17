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

    QList<QCameraViewfinderSettings> supportedViewfinderSettings() const Q_DECL_OVERRIDE;
    QCameraViewfinderSettings viewfinderSettings() const Q_DECL_OVERRIDE;
    void setViewfinderSettings(const QCameraViewfinderSettings &settings) Q_DECL_OVERRIDE;

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
    void applySettings();
    QCameraViewfinderSettings requestedSettings() const;
    // Aux. function to extract things like captureDevice, videoOutput, etc.
    bool updateAVFoundationObjects() const;

    AVFCameraService *m_service;
    mutable AVFCameraSession *m_session;
    QCameraViewfinderSettings m_settings;
    mutable AVCaptureDevice *m_captureDevice;
    mutable AVCaptureVideoDataOutput *m_videoOutput;
    mutable AVCaptureConnection *m_videoConnection;
};

class AVFCameraViewfinderSettingsControl : public QCameraViewfinderSettingsControl
{
    Q_OBJECT
public:
    AVFCameraViewfinderSettingsControl(AVFCameraService *service);

    bool isViewfinderParameterSupported(ViewfinderParameter parameter) const Q_DECL_OVERRIDE;
    QVariant viewfinderParameter(ViewfinderParameter parameter) const Q_DECL_OVERRIDE;
    void setViewfinderParameter(ViewfinderParameter parameter, const QVariant &value) Q_DECL_OVERRIDE;

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
