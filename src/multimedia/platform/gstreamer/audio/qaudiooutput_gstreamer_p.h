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
#include "qaudiodeviceinfo.h"
#include <private/qaudiosystem_p.h>

#include <private/qgst_p.h>

QT_BEGIN_NAMESPACE

class QGstAppSrc;

class QGStreamerAudioOutput
    : public QAbstractAudioOutput
{
    friend class GStreamerOutputPrivate;
    Q_OBJECT

public:
    QGStreamerAudioOutput(const QByteArray &device);
    ~QGStreamerAudioOutput();

    void start(QIODevice *device) override;
    QIODevice *start() override;
    void stop() override;
    void reset() override;
    void suspend() override;
    void resume() override;
    int bytesFree() const override;
    int periodSize() const override;
    void setBufferSize(int value) override;
    int bufferSize() const override;
    qint64 processedUSecs() const override;
    qint64 elapsedUSecs() const override;
    QAudio::Error error() const override;
    QAudio::State state() const override;
    void setFormat(const QAudioFormat &format) override;
    QAudioFormat format() const override;

    void setVolume(qreal volume) override;
    qreal volume() const override;

    void setCategory(const QString &category) override;
    QString category() const override;

private:
    void setState(QAudio::State state);
    void setError(QAudio::Error error);

    static gboolean busMessage(GstBus *bus, GstMessage *msg, gpointer user_data);

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
    QRingBuffer m_buffer;
    QTimer m_periodTimer;
    int m_bufferSize = 0;
    QElapsedTimer m_clockStamp;
    qint64 m_totalTimeValue = 0;
    QElapsedTimer m_timeStamp;
    qint64 m_elapsedTimeOffset = 0;
    QString m_category;
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
    friend class QGStreamerAudioOutput;
    Q_OBJECT

public:
    GStreamerOutputPrivate(QGStreamerAudioOutput *audio);
    virtual ~GStreamerOutputPrivate() {}

protected:
    qint64 readData(char *data, qint64 len) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QGStreamerAudioOutput *m_audioDevice;
};

QT_END_NAMESPACE

#endif
