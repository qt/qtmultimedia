// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIOOUTPUTGSTREAMER_H
#define QAUDIOOUTPUTGSTREAMER_H

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

#include <QtCore/qfile.h>
#include <QtCore/qtimer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qiodevice.h>
#include <QtCore/private/qringbuffer_p.h>

#include "qaudio.h"
#include "qaudiodevice.h"
#include <private/qaudiosystem_p.h>
#include <private/qmultimediautils_p.h>

#include <qgst_p.h>
#include <qgstpipeline_p.h>

QT_BEGIN_NAMESPACE

class QGstAppSrc;

class QGStreamerAudioSink
    : public QPlatformAudioSink,
      public QGstreamerBusMessageFilter
{
    friend class GStreamerOutputPrivate;
    Q_OBJECT

public:
    static QMaybe<QPlatformAudioSink *> create(const QAudioDevice &device, QObject *parent);
    ~QGStreamerAudioSink();

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    qsizetype bytesFree() const override;
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
    void bytesProcessedByAppSrc(int bytes);
    void needData();

private:
    QGStreamerAudioSink(const QAudioDevice &device, QGstAppSrc *appsrc, QGstElement audioconvert,
                        QGstElement volume, QObject *parent);

    void setState(QAudio::State state);
    void setError(QAudio::Error error);

    bool processBusMessage(const QGstreamerMessage &message) override;

    bool open();
    void close();
    qint64 write(const char *data, qint64 len);

private:
    QByteArray m_device;
    QAudioFormat m_format;
    QAudio::Error m_errorState = QAudio::NoError;
    QAudio::State m_deviceState = QAudio::StoppedState;
    QAudio::State m_suspendedInState = QAudio::SuspendedState;
    bool m_pullMode = true;
    bool m_opened = false;
    QIODevice *m_audioSource = nullptr;
    int m_bufferSize = 0;
    qint64 m_bytesProcessed = 0;
    QElapsedTimer m_timeStamp;
    qreal m_volume = 1.;
    QByteArray pushData;

    QGstPipeline gstPipeline;
    QGstElement gstOutput;
    QGstElement gstVolume;
    QGstElement gstAppSrc;
    QGstAppSrc *m_appSrc = nullptr;
};

class GStreamerOutputPrivate : public QIODevice
{
    friend class QGStreamerAudioSink;
    Q_OBJECT

public:
    GStreamerOutputPrivate(QGStreamerAudioSink *audio);
    virtual ~GStreamerOutputPrivate() {}

protected:
    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QGStreamerAudioSink *m_audioDevice;
};

QT_END_NAMESPACE

#endif
