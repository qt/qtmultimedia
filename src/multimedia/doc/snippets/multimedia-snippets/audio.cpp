// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    void handleStateChanged(QtAudio::State newState);

private:
    //! [Audio input class members]
    QFile destinationFile; // Class member
    QAudioSource* audio;   // Class member
    //! [Audio input class members]
};


void AudioInputExample::setup()
//! [Audio input setup]
{
    destinationFile.setFileName("/tmp/test.raw");
    destinationFile.open( QIODevice::WriteOnly | QIODevice::Truncate );

    QAudioFormat format;
    // Set up the desired format, for example:
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

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
void AudioInputExample::handleStateChanged(QtAudio::State newState)
{
    switch (newState) {
        case QtAudio::StoppedState:
            if (audio->error() != QtAudio::NoError) {
                // Error handling
            } else {
                // Finished recording
            }
            break;

        case QtAudio::ActiveState:
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
    void handleStateChanged(QtAudio::State newState);
    void stopAudioOutput();

private:
    //! [Audio output class members]
    QFile sourceFile;  // class member.
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
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

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
void AudioOutputExample::handleStateChanged(QtAudio::State newState)
{
    switch (newState) {
        case QtAudio::IdleState:
            // Finished playing (no more data)
            AudioOutputExample::stopAudioOutput();
            break;

        case QtAudio::StoppedState:
            // Stopped for other reasons
            if (audio->error() != QtAudio::NoError) {
                // Error handling
            }
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}
//! [Audio output state changed]

class AudioOutputWithCallbackExample : public QObject
{
    Q_OBJECT
public:
    void setupPlaySine();

private:
    //! [Audio callback output class members]
    QAudioSink* audio; // class member.
    float phase;       // class member.
    //! [Audio callback output class members]
};


void AudioOutputWithCallbackExample::setupPlaySine()
//! [Audio callback output setup sine]
{
    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice info(QMediaDevices::defaultAudioOutput());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot play audio.";
        return;
    }

    audio = new QAudioSink(format, this);
    float phaseIncrement = 2 * M_PI * 220.0 / format.sampleRate(); // 220 Hz sine wave
    audio->start([&phase, phaseIncrement] (QSpan<float> interleavedAudioBuffer) {
        // The audio callback should not call any functions that may potentially be blocking

        // Fill the audio buffer with a sine wave
        const int sampleCount = interleavedAudioBuffer.size() / 2; // Stereo, so divide by 2
        for (int i = 0; i < sampleCount; ++i) {
            float sample = std::sin(phase);
            interleavedAudioBuffer[i * 2] = sample;     // Left channel
            interleavedAudioBuffer[i * 2 + 1] = sample; // Right channel
            phase += phaseIncrement;                    // Increment phase for next sample
        }
    });

    if (!audio->error() == QtAudio::Error::NoError) {
        // in addition to the other start() signatures, starting the audio callback will fail if
        // * the backend does not implement callback-based IO (the API is available on all major
        //   platforms)
        // * the signature of the audio callback does not match format.sampleFormat()

        qWarning() << "Error starting audio output:" << audio->errorString();
    }
}
//! [Audio callback output setup sine]

class AudioInputWithCallbackExample : public QObject
{
    Q_OBJECT
public:
    void setupPeakMeter();

private:
    //! [Audio callback capture class members]
    QAudioSource* audio;          // class member.
    std::atomic<float> peakLevel; // class member.
    //! [Audio callback capture class members]
};


void AudioInputWithCallbackExample::setupPeakMeter()
//! [Audio callback capture setup peak meter]
{
    QAudioFormat format;
    // Set up the format, eg.
    format.setSampleRate(44100);
    format.setChannelCount(2);
    format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice info(QMediaDevices::defaultAudioOutput());
    if (!info.isFormatSupported(format)) {
        qWarning() << "Raw audio format not supported by backend, cannot capture audio.";
        return;
    }

    audio = new QAudioSource(format, this);
    audio->start([&peakLevel] (QSpan<float> interleavedAudioBuffer) {
        float level = peakLevel.load();

        for (float sample : interleavedAudioBuffer) {
            // Calculate the peak level from the audio samples
            level = std::max(level, std::abs(sample));
        }

        peakLevel.store(level);
        // Note: care needs to be taken if the application thread needs to be notified, as the
        // audio callback should not use any potentially blocking system calls.
        // Good options are autoreset events (windows), eventfd (linux) or kqueue/EVFILT_USER on macos.
    });

    if (!audio->error() == QtAudio::Error::NoError) {
        // in addition to the other start() signatures, starting the audio callback will fail if
        // * the backend does not implement callback-based IO (the API is available on all major
        //   platforms)
        // * the signature of the audio callback does not match format.sampleFormat()

        qWarning() << "Error starting audio output:" << audio->errorString();
    }
}
//! [Audio callback capture setup peak meter]

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
    void handleStateChanged(QtAudio::State newState);
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

    qreal linearVolume = QtAudio::convertVolume(volumeSliderValue / qreal(100.0),
                                                QtAudio::LogarithmicVolumeScale,
                                                QtAudio::LinearVolumeScale);

    player.setVolume(qRound(linearVolume * 100));
}
//! [Volume conversion]
