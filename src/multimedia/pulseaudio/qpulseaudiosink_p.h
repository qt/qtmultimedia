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

#ifndef QAUDIOOUTPUTPULSE_H
#define QAUDIOOUTPUTPULSE_H

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

#include "qaudio.h"
#include "qaudiodevice.h"
#include <private/qaudiosystem_p.h>

#include <pulse/pulseaudio.h>

QT_BEGIN_NAMESPACE

class QPulseAudioSink : public QPlatformAudioSink
{
    friend class PulseOutputPrivate;
    Q_OBJECT

public:
    QPulseAudioSink(const QByteArray &device);
    ~QPulseAudioSink();

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

    void streamUnderflowCallback();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    void setState(QAudio::State state);
    void setError(QAudio::Error error);
    void startReading();

    bool open();
    void close();
    qint64 write(const char *data, qint64 len);

private Q_SLOTS:
    void userFeed();
    void onPulseContextFailed();

private:
    pa_sample_spec m_spec = {};
    // calculate timing manually, as pulseaudio doesn't give us good enough data
    mutable timeval lastTimingInfo = {};

    mutable QList<qint64> latencyList; // last latency values

    QByteArray m_device;
    QByteArray m_streamName;
    QAudioFormat m_format;
    QBasicTimer m_tickTimer;

    QIODevice *m_audioSource = nullptr;
    pa_stream *m_stream = nullptr;
    char *m_audioBuffer = nullptr;

    qint64 m_totalTimeValue = 0;
    qint64 m_elapsedTimeOffset = 0;
    mutable qint64 averageLatency = 0; // average latency
    mutable qint64 lastProcessedUSecs = 0;
    qreal m_volume = 1.0;

    QAudio::Error m_errorState = QAudio::NoError;
    QAudio::State m_deviceState = QAudio::StoppedState;
    int m_periodSize = 0;
    int m_bufferSize = 0;
    int m_maxBufferSize = 0;
    int m_periodTime = 0;
    bool m_pullMode = true;
    bool m_opened = false;
    bool m_resuming = false;
};

class PulseOutputPrivate : public QIODevice
{
    friend class QPulseAudioSink;
    Q_OBJECT

public:
    PulseOutputPrivate(QPulseAudioSink *audio);
    virtual ~PulseOutputPrivate() {}

protected:
    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QPulseAudioSink *m_audioDevice;
};

QT_END_NAMESPACE

#endif
