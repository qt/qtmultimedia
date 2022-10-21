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
class QAudioInput;
class QAudioOutput;
class QCameraDevice;
class QImageCapture; // ### rename to QMediaImageCapture
class QMediaRecorder;
class QPlatformMediaCaptureSession;
class QVideoSink;

class QMediaCaptureSessionPrivate;
class Q_MULTIMEDIA_EXPORT QMediaCaptureSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAudioInput *audioInput READ audioInput WRITE setAudioInput NOTIFY audioInputChanged)
    Q_PROPERTY(QAudioOutput *audioOutput READ audioOutput WRITE setAudioOutput NOTIFY audioOutputChanged)
    Q_PROPERTY(QCamera *camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(QImageCapture *imageCapture READ imageCapture WRITE setImageCapture NOTIFY imageCaptureChanged)
    Q_PROPERTY(QMediaRecorder *recorder READ recorder WRITE setRecorder NOTIFY recorderChanged)
    Q_PROPERTY(QObject *videoOutput READ videoOutput WRITE setVideoOutput NOTIFY videoOutputChanged)
public:
    explicit QMediaCaptureSession(QObject *parent = nullptr);
    ~QMediaCaptureSession();

    QAudioInput *audioInput() const;
    void setAudioInput(QAudioInput *input);

    QCamera *camera() const;
    void setCamera(QCamera *camera);

    QImageCapture *imageCapture();
    void setImageCapture(QImageCapture *imageCapture);

    QMediaRecorder *recorder();
    void setRecorder(QMediaRecorder *recorder);

    void setVideoOutput(QObject *output);
    QObject *videoOutput() const;

    void setVideoSink(QVideoSink *sink);
    QVideoSink *videoSink() const;

    void setAudioOutput(QAudioOutput *output);
    QAudioOutput *audioOutput() const;

    QPlatformMediaCaptureSession *platformSession() const;

Q_SIGNALS:
    void audioInputChanged();
    void cameraChanged();
    void imageCaptureChanged();
    void recorderChanged();
    void videoOutputChanged();
    void audioOutputChanged();

private:
    QMediaCaptureSessionPrivate *d_ptr;
    Q_DISABLE_COPY(QMediaCaptureSession)
    Q_DECLARE_PRIVATE(QMediaCaptureSession)
};

QT_END_NAMESPACE

#endif  // QMEDIACAPTURESESSION_H
