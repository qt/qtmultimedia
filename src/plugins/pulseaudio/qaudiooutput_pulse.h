/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
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
#include <QtCore/qdatetime.h>

#include "qaudio.h"
#include "qaudiodeviceinfo.h"
#include "qaudiosystem.h"

#include <pulse/pulseaudio.h>

QT_BEGIN_NAMESPACE

class QPulseAudioOutput : public QAbstractAudioOutput
{
    friend class OutputPrivate;
    Q_OBJECT

public:
    QPulseAudioOutput(const QByteArray &device);
    ~QPulseAudioOutput();

    void start(QIODevice *device);
    QIODevice *start();
    void stop();
    void reset();
    void suspend();
    void resume();
    int bytesFree() const;
    int periodSize() const;
    void setBufferSize(int value);
    int bufferSize() const;
    void setNotifyInterval(int milliSeconds);
    int notifyInterval() const;
    qint64 processedUSecs() const;
    qint64 elapsedUSecs() const;
    QAudio::Error error() const;
    QAudio::State state() const;
    void setFormat(const QAudioFormat &format);
    QAudioFormat format() const;

public:
    void streamUnderflowCallback();

private:
    bool open();
    void close();
    qint64 write(const char *data, qint64 len);

private Q_SLOTS:
    bool deviceReady();
    void userFeed();

private:
    QByteArray m_device;
    QByteArray m_streamName;
    QAudioFormat m_format;
    QAudio::Error m_errorState;
    QAudio::State m_deviceState;
    bool m_pullMode;
    bool m_opened;
    QIODevice *m_audioSource;
    int m_bytesAvailable;
    QTimer m_periodTimer;
    pa_stream *m_stream;
    int m_notifyInterval;
    int m_periodSize;
    int m_bufferSize;
    QTime m_clockStamp;
    qint64 m_totalTimeValue;
    QTimer *m_tickTimer;
    char *m_audioBuffer;
    QTime m_timeStamp;
    qint64 m_elapsedTimeOffset;
    bool m_resuming;
};

class OutputPrivate : public QIODevice
{
    friend class QPulseAudioOutput;
    Q_OBJECT

public:
    OutputPrivate(QPulseAudioOutput *audio);
    virtual ~OutputPrivate() {}

protected:
    qint64 readData(char *data, qint64 len);
    qint64 writeData(const char *data, qint64 len);

private:
    QPulseAudioOutput *m_audioDevice;
};

QT_END_NAMESPACE

#endif
