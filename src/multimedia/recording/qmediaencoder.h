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

#ifndef QMEDIAENCODER_H
#define QMEDIAENCODER_H

#include <QtCore/qobject.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmediaencodersettings.h>
#include <QtMultimedia/qmediaenumdebug.h>
#include <QtMultimedia/qmediametadata.h>

#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

class QUrl;
class QSize;
class QAudioFormat;
class QCamera;
class QCameraDevice;
class QMediaRecorderService;
class QAudioEncoderSettings;
class QVideoEncoderSettings;
class QAudioDevice;
class QMediaCaptureSession;

class Q_MULTIMEDIA_EXPORT QMediaEncoderBase : public QObject
{
    Q_OBJECT

public:
    QMediaEncoderBase(QObject *parent) : QObject(parent) {}
    enum State
    {
        StoppedState,
        RecordingState,
        PausedState
    };
    Q_ENUM(State)

    enum Status {
        UnavailableStatus,
        StoppedStatus,
        StartingStatus,
        RecordingStatus,
        PausedStatus,
        FinalizingStatus
    };
    Q_ENUM(Status)

    enum Error
    {
        NoError,
        ResourceError,
        FormatError,
        OutOfSpaceError
    };
    Q_ENUM(Error)
};

class QMediaEncoderPrivate;
class Q_MULTIMEDIA_EXPORT QMediaEncoder : public QMediaEncoderBase
{
    Q_OBJECT
    Q_PROPERTY(QMediaEncoderBase::State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QMediaEncoderBase::Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QUrl outputLocation READ outputLocation WRITE setOutputLocation)
    Q_PROPERTY(QUrl actualLocation READ actualLocation NOTIFY actualLocationChanged)
    Q_PROPERTY(QMediaMetaData metaData READ metaData WRITE setMetaData NOTIFY metaDataChanged)
    Q_PROPERTY(QMediaEncoderBase::Error error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    Q_PROPERTY(QMediaEncoderSettings encoderSettings READ encoderSettings WRITE setEncoderSettings NOTIFY encoderSettingsChanged)

public:
    QMediaEncoder(QObject *parent = nullptr);
    ~QMediaEncoder();

    bool isAvailable() const;

    QUrl outputLocation() const;
    bool setOutputLocation(const QUrl &location);

    QUrl actualLocation() const;

    QMediaEncoderBase::State state() const;
    QMediaEncoderBase::Status status() const;

    QMediaEncoderBase::Error error() const;
    QString errorString() const;

    qint64 duration() const;

    void setEncoderSettings(const QMediaEncoderSettings &);
    QMediaEncoderSettings encoderSettings() const;

    QMediaMetaData metaData() const;
    void setMetaData(const QMediaMetaData &metaData);
    void addMetaData(const QMediaMetaData &metaData);

    QMediaCaptureSession *captureSession() const;

public Q_SLOTS:
    void record();
    void pause();
    void stop();

Q_SIGNALS:
    void stateChanged(QMediaEncoder::State state);
    void statusChanged(QMediaEncoder::Status status);
    void durationChanged(qint64 duration);
    void actualLocationChanged(const QUrl &location);
    void encoderSettingsChanged();

    void errorOccurred(QMediaEncoder::Error error, const QString &errorString);
    void errorChanged();

    void metaDataChanged();

private:
    QMediaEncoderPrivate *d_ptr;
    friend class QMediaCaptureSession;
    void setCaptureSession(QMediaCaptureSession *session);
    Q_DISABLE_COPY(QMediaEncoder)
    Q_DECLARE_PRIVATE(QMediaEncoder)
    Q_PRIVATE_SLOT(d_func(), void _q_applySettings())
};

QT_END_NAMESPACE

Q_MEDIA_ENUM_DEBUG(QMediaEncoderBase, State)
Q_MEDIA_ENUM_DEBUG(QMediaEncoderBase, Status)
Q_MEDIA_ENUM_DEBUG(QMediaEncoderBase, Error)

#endif  // QMEDIAENCODER_H
