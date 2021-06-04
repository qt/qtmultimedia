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
#include <QtCore/private/qringbuffer_p.h>

#include "qaudio.h"
#include "qaudiodevice.h"
#include <private/qaudiosystem_p.h>

#include <private/qgst_p.h>
#include <private/qgstpipeline_p.h>

QT_BEGIN_NAMESPACE

class QGstAppSrc;

class QGStreamerAudioSink
    : public QPlatformAudioSink,
      public QGstreamerBusMessageFilter
{
    friend class GStreamerOutputPrivate;
    Q_OBJECT

public:
    QGStreamerAudioSink(const QAudioDevice &device);
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
    bool m_pullMode = true;
    bool m_opened = false;
    QIODevice *m_audioSource = nullptr;
    QTimer m_periodTimer;
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
