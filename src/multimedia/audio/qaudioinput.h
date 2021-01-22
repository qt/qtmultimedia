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


#ifndef QAUDIOINPUT_H
#define QAUDIOINPUT_H

#include <QtCore/qiodevice.h>

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmultimedia.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qaudiodeviceinfo.h>


QT_BEGIN_NAMESPACE



class QAbstractAudioInput;

class Q_MULTIMEDIA_EXPORT QAudioInput : public QObject
{
    Q_OBJECT

public:
    explicit QAudioInput(const QAudioFormat &format = QAudioFormat(), QObject *parent = nullptr);
    explicit QAudioInput(const QAudioDeviceInfo &audioDeviceInfo, const QAudioFormat &format = QAudioFormat(), QObject *parent = nullptr);
    ~QAudioInput();

    QAudioFormat format() const;

    void start(QIODevice *device);
    QIODevice* start();

    void stop();
    void reset();
    void suspend();
    void resume();

    void setBufferSize(int bytes);
    int bufferSize() const;

    int bytesReady() const;
    int periodSize() const;

    void setNotifyInterval(int milliSeconds);
    int notifyInterval() const;

    void setVolume(qreal volume);
    qreal volume() const;

    qint64 processedUSecs() const;
    qint64 elapsedUSecs() const;

    QAudio::Error error() const;
    QAudio::State state() const;

Q_SIGNALS:
    void stateChanged(QAudio::State state);
    void notify();

private:
    Q_DISABLE_COPY(QAudioInput)

    QAbstractAudioInput* d;
};

QT_END_NAMESPACE

#endif // QAUDIOINPUT_H
