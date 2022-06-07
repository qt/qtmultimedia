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

#ifndef QWINDOWSAUDIOINPUT_H
#define QWINDOWSAUDIOINPUT_H

#include "qwindowsaudioutils_p.h"

#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmutex.h>

#include <QtMultimedia/qaudio.h>
#include <QtMultimedia/qaudiodevice.h>
#include <private/qaudiosystem_p.h>


QT_BEGIN_NAMESPACE


// For compat with 4.6
#if !defined(QT_WIN_CALLBACK)
#  if defined(Q_CC_MINGW)
#    define QT_WIN_CALLBACK CALLBACK __attribute__ ((force_align_arg_pointer))
#  else
#    define QT_WIN_CALLBACK CALLBACK
#  endif
#endif

class QWindowsAudioSource : public QPlatformAudioSource
{
    Q_OBJECT
public:
    QWindowsAudioSource(int deviceId);
    ~QWindowsAudioSource();

    qint64 read(char* data, qint64 len);

    void setFormat(const QAudioFormat& fmt) override;
    QAudioFormat format() const override;
    QIODevice* start() override;
    void start(QIODevice* device) override;
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
    void setVolume(qreal volume) override;
    qreal volume() const override;

    QIODevice* audioSource;
    QAudioFormat settings;
    QAudio::Error errorState;
    QAudio::State deviceState;

private:
    qint32 buffer_size;
    qint32 period_size;
    qint32 header;
    int m_deviceId;
    int bytesAvailable;
    qint64 elapsedTimeOffset;
    qint64 totalTimeValue;
    bool pullMode;
    bool resuming;
    WAVEFORMATEXTENSIBLE wfx;
    HWAVEIN hWaveIn;
    MMRESULT result;
    WAVEHDR* waveBlocks;
    volatile bool finished;
    volatile int waveFreeBlockCount;
    int waveBlockOffset;

    QMutex mutex;
    static void QT_WIN_CALLBACK waveInProc( HWAVEIN hWaveIn, UINT uMsg,
            DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 );

    WAVEHDR* allocateBlocks(int size, int count);
    void freeBlocks(WAVEHDR* blockArray);
    bool open();
    void close();

    void initMixer();
    void closeMixer();
    HMIXEROBJ mixerID;
    MIXERLINECONTROLS mixerLineControls;
    qreal cachedVolume;

private slots:
    void feedback();
    bool deviceReady();

signals:
    void processMore();
};

class InputPrivate : public QIODevice
{
    Q_OBJECT
public:
    InputPrivate(QWindowsAudioSource* audio);
    ~InputPrivate();

    qint64 readData( char* data, qint64 len) override;
    qint64 writeData(const char* data, qint64 len) override;

    void trigger();
private:
    QWindowsAudioSource *audioDevice;
};

QT_END_NAMESPACE

#endif
