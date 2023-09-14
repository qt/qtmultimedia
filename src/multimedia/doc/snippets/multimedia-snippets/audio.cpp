// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/* Audio related snippets */
#include <QFile>
#include <QTimer>
#include <QDebug>
#include <qobject.h>
#include <qfile.h>

#include "qaudiodevice.h"
#include "qaudiosource.h"
#include "qaudiooutput.h"
#include "qaudiodecoder.h"
#include "qmediaplayer.h"
#include "qmediadevices.h"

class AudioInputExample : public QObject {
    Q_OBJECT
public:
    void setup();


public Q_SLOTS:
    void stopRecording();
    void handleStateChanged(QAudio::State newState);

private:
    //! [Audio input class members]
    QFile destinationFile;   // Class member
    QAudioSource* audio; // Class member
    //! [Audio input class members]
};


void AudioInputExample::setup()
//! [Audio input setup]
{
    destinationFile.setFileName("/tmp/test.raw");
    destinationFile.open( QIODevice::WriteOnly | QIODevice::Truncate );

    QAudioFormat format;
    // Set up the desired format, for example:
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::UInt8);

    QAudioDevice info = QMediaDevices::defaultAudioInput();
    if (!info.isFormatSupported(format)) {
        qWarning() << "Default format not supported, trying to use the nearest.";
    }

    audio = new QAudioSource(format, this);
    connect(audio, &QAudioSource::stateChanged, this, &AudioInputExample::handleStateChanged);

    QTimer::singleShot(3000, this, &AudioInputExample::stopRecording);
    audio->start(&destinationFile);
    // Records audio for 3000ms
}
//! [Audio input setup]

//! [Audio input stop recording]
void AudioInputExample::stopRecording()
{
    audio->stop();
    destinationFile.close();
    delete audio;
}
//! [Audio input stop recording]

//! [Audio input state changed]
void AudioInputExample::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
        case QAudio::StoppedState:
            if (audio->error() != QAudio::NoError) {
                // Error handling
            } else {
                // Finished recording
            }
            break;

        case QAudio::ActiveState:
            // Started recording - read from IO device
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}
//! [Audio input state changed]


class AudioOutputExample : public QObject {
    Q_OBJECT
public:
    void setup();

public Q_SLOTS:
    void handleStateChanged(QAudio::State newState);
    void stopAudioOutput();

private:
    //! [Audio output class members]
    QFile sourceFile;   // class member.
    QAudioSink* audio; // class member.
    //! [Audio output class members]
};


void AudioOutputExample::setup()
//! [Audio output setup]
{
    sourceFile.setFileName("/tmp/test.raw");
    sourceFile.open(QIODevice::ReadOnly);

    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(8000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::UInt8);

    QAudioDevice info(QMediaDevices::defaultAudioOutput());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return;
    }

    audio = new QAudioSink(format, this);
    connect(audio, QAudioSink::stateChanged, this, &AudioInputExample::handleStateChanged);
    audio->start(&sourceFile);
}
//! [Audio output setup]

//! [Audio output stop]
void AudioOutputExample::stopAudioOutput()
{
    audio->stop();
    sourceFile.close();
    delete audio;
}
//! [Audio output stop]

//! [Audio output state changed]
void AudioOutputExample::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
        case QAudio::IdleState:
            // Finished playing (no more data)
            AudioOutputExample::stopAudioOutput();
            break;

        case QAudio::StoppedState:
            // Stopped for other reasons
            if (audio->error() != QAudio::NoError) {
                // Error handling
            }
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}
//! [Audio output state changed]

void AudioDeviceInfo()
{
    //! [Setting audio format]
    QAudioFormat format;
    format.setSampleRate(44100);
    // ... other format parameters
    format.setSampleFormat(QAudioFormat::Int16);
    //! [Setting audio format]

    //! [Dumping audio formats]
    const auto devices = QMediaDevices::audioOutputs();
    for (const QAudioDevice &device : devices)
        qDebug() << "Device: " << device.description();
    //! [Dumping audio formats]
}

class AudioDecodingExample : public QObject {
    Q_OBJECT
public:
    void decode();

public Q_SLOTS:
    void handleStateChanged(QAudio::State newState);
    void readBuffer();
};

void AudioDecodingExample::decode()
{
    //! [Local audio decoding]
    QAudioFormat desiredFormat;
    desiredFormat.setChannelCount(2);
    desiredFormat.setSampleFormat(QAudioFormat::Int16);
    desiredFormat.setSampleRate(48000);

    QAudioDecoder *decoder = new QAudioDecoder(this);
    decoder->setAudioFormat(desiredFormat);
    decoder->setSource("level1.mp3");

    connect(decoder, &QAudioDecoder::bufferReady, this, &AudioDecodingExample::readBuffer);
    decoder->start();

    // Now wait for bufferReady() signal and call decoder->read()
    //! [Local audio decoding]
}

QMediaPlayer player;

//! [Volume conversion]
void applyVolume(int volumeSliderValue)
{
    // volumeSliderValue is in the range [0..100]

    qreal linearVolume = QAudio::convertVolume(volumeSliderValue / qreal(100.0),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    player.setVolume(qRound(linearVolume * 100));
}
//! [Volume conversion]
