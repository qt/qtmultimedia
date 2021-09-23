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

#ifndef QWINDOWSAUDIOOUTPUT_H
#define QWINDOWSAUDIOOUTPUT_H

#include "qwindowsaudioutils_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmutex.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <private/qaudiosystem_p.h>

// For compat with 4.6
#if !defined(QT_WIN_CALLBACK)
#  if defined(Q_CC_MINGW)
#    define QT_WIN_CALLBACK CALLBACK __attribute__ ((force_align_arg_pointer))
#  else
#    define QT_WIN_CALLBACK CALLBACK
#  endif
#endif

QT_BEGIN_NAMESPACE

class QWindowsAudioSink : public QPlatformAudioSink
{
    Q_OBJECT
public:
    QWindowsAudioSink(int deviceId);
    ~QWindowsAudioSink();

    qint64 write( const char *data, qint64 len );

    void setFormat(const QAudioFormat& fmt) override;
    QAudioFormat format() const override;
    QIODevice* start() override;
    void start(QIODevice* device) override;
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
    void setVolume(qreal) override;
    qreal volume() const override;

    QIODevice* audioSource;
    QAudioFormat settings;
    QAudio::Error errorState;
    QAudio::State deviceState;

private slots:
    void feedback();
    bool deviceReady();

private:
    void pauseAndSleep();
    int m_deviceId;
    int bytesAvailable;
    qint64 elapsedTimeOffset;
    qint32 buffer_size;
    qint32 period_size;
    qint32 blocks_count;
    qint64 totalTimeValue;
    bool pullMode;
    qreal volumeCache;
    static void QT_WIN_CALLBACK waveOutProc( HWAVEOUT hWaveOut, UINT uMsg,
            DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 );

    mutable QMutex mutex;

    WAVEHDR* allocateBlocks(int size, int count);
    void freeBlocks(WAVEHDR* blockArray);
    bool open();
    void close();

    WAVEFORMATEXTENSIBLE wfx;
    HWAVEOUT hWaveOut;
    WAVEHDR* waveBlocks;
    int waveFreeBlockCount = 0;
    int waveCurrentBlock = 0;
    char *audioBuffer = nullptr;
};

class OutputPrivate : public QIODevice
{
    Q_OBJECT
public:
    OutputPrivate(QWindowsAudioSink* audio);
    ~OutputPrivate();

    qint64 readData( char* data, qint64 len) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    QWindowsAudioSink *audioDevice;
};

QT_END_NAMESPACE


#endif // QWINDOWSAUDIOOUTPUT_H
