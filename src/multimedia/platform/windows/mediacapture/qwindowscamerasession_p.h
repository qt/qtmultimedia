/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QWINDOWSCAMERASESSION_H
#define QWINDOWSCAMERASESSION_H

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

#include <qcamera.h>
#include <qmediaencodersettings.h>

QT_BEGIN_NAMESPACE

class QVideoSink;
class QWindowsCameraReader;
class QWindowsCameraExposure;
class QWindowsCameraFocus;
class QWindowsCameraImageProcessing;

class QWindowsCameraSession : public QObject
{
    Q_OBJECT
public:
    explicit QWindowsCameraSession(QObject *parent = 0);
    ~QWindowsCameraSession();

    bool isActive() const;
    void setActive(bool active);

    QImageEncoderSettings imageSettings() const;
    void setImageSettings(const QImageEncoderSettings &settings);

    bool isReadyForCapture() const;
    void setReadyForCapture(bool ready);

    void setActiveCamera(const QCameraInfo &info);

    int capture(const QString &fileName);

    void setVideoSink(QVideoSink *surface);

    QWindowsCameraFocus *focusControl();
    QWindowsCameraExposure *exposureControl();
    QWindowsCameraImageProcessing *imageProcessingControl();

Q_SIGNALS:
    void activeChanged(bool);
    void readyForCaptureChanged(bool);

private Q_SLOTS:
    void handleStreamStarted();
    void handleStreamStopped();

private:
    bool m_active = false;
    bool m_readyForCapture = false;
    QCameraInfo m_activeCameraInfo;
    QWindowsCameraReader *m_cameraReader = nullptr;
    QWindowsCameraExposure *m_cameraExposure = nullptr;
    QWindowsCameraFocus *m_cameraFocus = nullptr;
    QWindowsCameraImageProcessing *m_cameraImageProcessing = nullptr;
    QImageEncoderSettings m_imageEncoderSettings;
};

QT_END_NAMESPACE

#endif // QWINDOWSCAMERASESSION_H
