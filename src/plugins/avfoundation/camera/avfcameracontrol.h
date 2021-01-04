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

#ifndef AVFCAMERACONTROL_H
#define AVFCAMERACONTROL_H

#include <QtCore/qobject.h>

#include <QtMultimedia/qcameracontrol.h>

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;
@class AVCaptureDeviceFormat;
@class AVCaptureConnection;

class AVFCameraControl : public QCameraControl
{
Q_OBJECT
public:
    AVFCameraControl(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFCameraControl();

    QCamera::State state() const override;
    void setState(QCamera::State state) override;

    QCamera::Status status() const override;

    QCamera::CaptureModes captureMode() const override;
    void setCaptureMode(QCamera::CaptureModes) override;
    bool isCaptureModeSupported(QCamera::CaptureModes mode) const override;

    bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const override;

    QCamera::LockTypes supportedLocks() const override;

    QCamera::LockStatus lockStatus(QCamera::LockType lock) const override;

    void searchAndLock(QCamera::LockTypes locks) override;
    void unlock(QCamera::LockTypes locks) override;

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
    QList<QVideoFrame::PixelFormat> viewfinderPixelFormats() const;
    bool convertPixelFormatIfSupported(QVideoFrame::PixelFormat format, unsigned &avfFormat) const;
    bool applySettings(const QCameraViewfinderSettings &settings);
    QCameraViewfinderSettings requestedSettings() const;

    AVCaptureConnection *videoConnection() const;

private Q_SLOTS:
    void updateStatus();

private:
    friend class AVFCameraSession;
    AVFCameraSession *m_session;
    AVFCameraService *m_service;
    QCameraViewfinderSettings m_settings;

    QCamera::State m_state;
    QCamera::Status m_lastStatus;
    QCamera::CaptureModes m_captureMode;
};

QT_END_NAMESPACE

#endif
