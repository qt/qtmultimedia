// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QAUDIOINPUTGSTREAMER_H
#define QAUDIOINPUTGSTREAMER_H

#include <QtCore/qfile.h>
#include <QtCore/qtimer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qmutex.h>
#include <QtCore/qatomic.h>
#include <QtCore/private/qringbuffer_p.h>

#include "qaudio.h"
#include "qaudiodevice.h"
#include <private/qaudiosystem_p.h>

#include <qgstutils_p.h>
#include <qgstpipeline_p.h>
#include <gst/app/gstappsink.h>

QT_BEGIN_NAMESPACE

class GStreamerInputPrivate;

class QGStreamerAudioSource
    : public QPlatformAudioSource
{
    Q_OBJECT
    friend class GStreamerInputPrivate;
public:
    QGStreamerAudioSource(const QAudioDevice &device, QObject *parent);
    ~QGStreamerAudioSource();

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesReady() const override;
    void setBufferSize(qsizetype value) override;
    qsizetype bufferSize() const override;
    qint64 processedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat &format) override;
    QAudioFormat format() const override;

    void setVolume(qreal volume) override;
    qreal volume() const override;

private Q_SLOTS:
    void newDataAvailable(GstSample *sample);

private:
    void setState(QAudio::State state);
    void setError(QAudio::Error error);

    QGstElement createAppSink();
    static GstFlowReturn new_sample(GstAppSink *, gpointer user_data);
    static void eos(GstAppSink *, gpointer user_data);

    bool open();
    void close();

    static gboolean busMessage(GstBus *bus, GstMessage *msg, gpointer user_data);

    QAudioDevice m_info;
    qint64 m_bytesWritten = 0;
    QIODevice *m_audioSink = nullptr;
    QAudioFormat m_format;
    QAudio::Error m_errorState = QAudio::NoError;
    QAudio::State m_deviceState = QAudio::StoppedState;
    qreal m_volume = 1.;

    QRingBuffer m_buffer;
    QAtomicInteger<bool> m_pullMode = true;
    bool m_opened = false;
    int m_bufferSize = 0;
    qint64 m_elapsedTimeOffset = 0;
    QElapsedTimer m_timeStamp;
    QByteArray m_device;
    QByteArray m_tempBuffer;

    QGstElement gstInput;
    QGstPipeline gstPipeline;
    QGstElement gstVolume;
    QGstElement gstAppSink;
};

class GStreamerInputPrivate : public QIODevice
{
    Q_OBJECT
public:
    GStreamerInputPrivate(QGStreamerAudioSource *audio);
    ~GStreamerInputPrivate() {};

    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;
    qint64 bytesAvailable() const override;
    bool isSequential() const override { return true; }
private:
    QGStreamerAudioSource *m_audioDevice;
};

QT_END_NAMESPACE

#endif
