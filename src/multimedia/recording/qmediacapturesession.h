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

#ifndef QMEDIACAPTURESESSION_H
#define QMEDIACAPTURESESSION_H

#include <QtCore/qobject.h>
#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

class QCamera;
class QAudioDeviceInfo;
class QCameraInfo;
class QCameraImageCapture; // ### rename to QMediaImageCapture
class QMediaEncoder;
class QPlatformMediaCaptureSession;
class QAbstractVideoSurface;

class QMediaCaptureSessionPrivate;
class Q_MULTIMEDIA_EXPORT QMediaCaptureSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAudioDeviceInfo audioInput READ audioInput WRITE setAudioInput NOTIFY audioInputChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QCamera *camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(QCameraImageCapture *imageCapture READ imageCapture WRITE setImageCapture NOTIFY imageCaptureChanged)
    Q_PROPERTY(QMediaEncoder *encoder READ encoder WRITE setEncoder NOTIFY encoderChanged)
public:
    explicit QMediaCaptureSession(QObject *parent = nullptr);
    ~QMediaCaptureSession();

    bool isAvailable() const;

    QAudioDeviceInfo audioInput() const; // ### Should use a QAudioDevice *
    void setAudioInput(const QAudioDeviceInfo &device);

    bool isMuted() const; // ### Should move to QAudioDevice
    void setMuted(bool muted);
    qreal volume() const;
    void setVolume(qreal volume);

    QCamera *camera() const;
    void setCamera(QCamera *camera);

    QCameraImageCapture *imageCapture();
    void setImageCapture(QCameraImageCapture *imageCapture);

    QMediaEncoder *encoder();
    void setEncoder(QMediaEncoder *recorder);

    void setVideoPreview(QObject *preview);
    void setVideoPreview(QAbstractVideoSurface *preview);

    QPlatformMediaCaptureSession *platformSession() const;

Q_SIGNALS:
    void audioInputChanged();
    void mutedChanged(bool muted);
    void volumeChanged(qreal volume);
    void cameraChanged();
    void imageCaptureChanged();
    void encoderChanged();

private:
    QMediaCaptureSessionPrivate *d_ptr;
    Q_DISABLE_COPY(QMediaCaptureSession)
    Q_DECLARE_PRIVATE(QMediaCaptureSession)
};

QT_END_NAMESPACE

#endif  // QMEDIACAPTURESESSION_H
