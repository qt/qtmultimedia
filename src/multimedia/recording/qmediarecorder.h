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

#ifndef QMEDIARECORDER_H
#define QMEDIARECORDER_H

#include <QtMultimedia/qmediaencoder.h>

QT_BEGIN_NAMESPACE

class QMediaRecorderPrivate;
class Q_MULTIMEDIA_EXPORT QMediaRecorder : public QMediaEncoderBase
{
    Q_OBJECT
    Q_PROPERTY(QMediaEncoderBase::State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QMediaEncoderBase::Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QUrl outputLocation READ outputLocation WRITE setOutputLocation)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(QMediaMetaData metaData READ metaData WRITE setMetaData NOTIFY metaDataChanged)
    Q_PROPERTY(QAudioDeviceInfo audioInput READ audioInput WRITE setAudioInput NOTIFY audioInputChanged)
    Q_PROPERTY(CaptureMode captureMode READ captureMode WRITE setCaptureMode NOTIFY captureModeChanged)
public:

    enum CaptureMode {
        AudioOnly,
        AudioAndVideo
    };

    QMediaRecorder(QObject *parent = nullptr, CaptureMode mode = AudioOnly);
    ~QMediaRecorder();

    bool isAvailable() const;

    CaptureMode captureMode() const;
    void setCaptureMode(CaptureMode mode);

    QCamera *camera() const;

    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl &location);

    QMediaEncoderBase::State state() const;
    QMediaEncoderBase::Status status() const;

    QMediaEncoderBase::Error error() const;
    QString errorString() const;

    qint64 duration() const;

    bool isMuted() const;
    qreal volume() const;

    void setEncoderSettings(const QMediaEncoderSettings &);
    QMediaEncoderSettings encoderSettings() const;

    QMediaMetaData metaData() const;
    void setMetaData(const QMediaMetaData &metaData);
    void addMetaData(const QMediaMetaData &metaData);

    QAudioDeviceInfo audioInput() const;

    QMediaCaptureSession *captureSession() const;

public Q_SLOTS:
    void record();
    void pause();
    void stop();
    void setMuted(bool muted);
    void setVolume(qreal volume);
    void setAudioInput(const QAudioDeviceInfo &device);

Q_SIGNALS:
    void stateChanged(QMediaRecorder::State state);
    void statusChanged(QMediaRecorder::Status status);
    void durationChanged(qint64 duration);
    void mutedChanged(bool muted);
    void volumeChanged(qreal volume);
    void audioInputChanged();
    void captureModeChanged();

    void error(QMediaRecorder::Error error);

    void metaDataChanged();

private:
    // This is here to flag an incompatibilities with Qt 5
    QMediaRecorder(QCamera *) = delete;

    QMediaRecorderPrivate *d_ptr;
    friend class QMediaCaptureSession;
    Q_DISABLE_COPY(QMediaRecorder)
    Q_DECLARE_PRIVATE(QMediaRecorder)
};

QT_END_NAMESPACE

#endif  // QMEDIARECORDER_H
