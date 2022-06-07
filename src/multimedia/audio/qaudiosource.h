// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QAUDIOINPUT_H
#define QAUDIOINPUT_H

#include <QtCore/qiodevice.h>

#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiodevice.h>


QT_BEGIN_NAMESPACE

class QPlatformAudioSource;

class Q_MULTIMEDIA_EXPORT QAudioSource : public QObject
{
    Q_OBJECT

public:
    explicit QAudioSource(const QAudioFormat &format = QAudioFormat(), QObject *parent = nullptr);
    explicit QAudioSource(const QAudioDevice &audioDeviceInfo, const QAudioFormat &format = QAudioFormat(), QObject *parent = nullptr);
    ~QAudioSource();

    bool isNull() const { return !d; }

    QAudioFormat format() const;

    void start(QIODevice *device);
    QIODevice* start();

    void stop();
    void reset();
    void suspend();
    void resume();

    void setBufferSize(qsizetype bytes);
    qsizetype bufferSize() const;

    qsizetype bytesAvailable() const;

    void setVolume(qreal volume);
    qreal volume() const;

    qint64 processedUSecs() const;
    qint64 elapsedUSecs() const;

    QAudio::Error error() const;
    QAudio::State state() const;

Q_SIGNALS:
    void stateChanged(QAudio::State state);

private:
    Q_DISABLE_COPY(QAudioSource)

    QPlatformAudioSource *d;
};

QT_END_NAMESPACE

#endif // QAUDIOINPUT_H
