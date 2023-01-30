// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ENGINE_H
#define ENGINE_H

#include "spectrum.h"
#include "spectrumanalyser.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QBuffer>
#include <QByteArray>
#include <QDir>
#include <QList>
#include <QMediaDevices>
#include <QObject>
#include <QTimer>
#include <QWaveDecoder>

#ifdef DUMP_CAPTURED_AUDIO
#    define DUMP_DATA
#endif

#ifdef DUMP_SPECTRUM
#    define DUMP_DATA
#endif

class FrequencySpectrum;
QT_BEGIN_NAMESPACE
class QAudioSource;
class QAudioSink;
QT_END_NAMESPACE

/**
 * This class interfaces with the Qt Multimedia audio classes, and also with
 * the SpectrumAnalyser class.  Its role is to manage the capture and playback
 * of audio data, meanwhile performing real-time analysis of the audio level
 * and frequency spectrum.
 */
class Engine : public QObject
{
    Q_OBJECT

public:
    explicit Engine(QObject *parent = nullptr);
    ~Engine();

    const QList<QAudioDevice> &availableAudioInputDevices() const
    {
        return m_availableAudioInputDevices;
    }

    const QList<QAudioDevice> &availableAudioOutputDevices() const
    {
        return m_availableAudioOutputDevices;
    }

    QAudioDevice::Mode mode() const { return m_mode; }
    QAudio::State state() const { return m_state; }

    /**
     * \return Current audio format
     * \note May be QAudioFormat() if engine is not initialized
     */
    const QAudioFormat &format() const { return m_format; }

    /**
     * Stop any ongoing recording or playback, and reset to ground state.
     */
    void reset();
    /**
     * Load data from WAV file
     */
    bool loadFile(const QString &fileName);

    /**
     * Generate tone
     */
    bool generateTone(const Tone &tone);

    /**
     * Generate tone
     */
    bool generateSweptTone(qreal amplitude);

    /**
     * Initialize for recording
     */
    bool initializeRecord();

    /**
     * Position of the audio input device.
     * \return Position in bytes.
     */
    qint64 recordPosition() const { return m_recordPosition; }

    /**
     * RMS level of the most recently processed set of audio samples.
     * \return Level in range (0.0, 1.0)
     */
    qreal rmsLevel() const { return m_rmsLevel; }

    /**
     * Peak level of the most recently processed set of audio samples.
     * \return Level in range (0.0, 1.0)
     */
    qreal peakLevel() const { return m_peakLevel; }

    /**
     * Position of the audio output device.
     * \return Position in bytes.
     */
    qint64 playPosition() const { return m_playPosition; }

    /**
     * Length of the internal engine buffer.
     * \return Buffer length in bytes.
     */
    qint64 bufferLength() const;

    /**
     * Amount of data held in the buffer.
     * \return Data length in bytes.
     */
    qint64 dataLength() const { return m_dataLength; }

    /**
     * Set window function applied to audio data before spectral analysis.
     */
    void setWindowFunction(WindowFunction type);

public slots:
    void startRecording();
    void startPlayback();
    void suspend();
    void setAudioInputDevice(const QAudioDevice &device);
    void setAudioOutputDevice(const QAudioDevice &device);

signals:
    void stateChanged(QAudioDevice::Mode mode, QAudio::State state);

    /**
     * Informational message for non-modal display
     */
    void infoMessage(const QString &message, int durationMs);

    /**
     * Error message for modal display
     */
    void errorMessage(const QString &heading, const QString &detail);

    /**
     * Format of audio data has changed
     */
    void formatChanged(const QAudioFormat &format);

    /**
     * Length of buffer has changed.
     * \param duration Duration in microseconds
     */
    void bufferLengthChanged(qint64 duration);

    /**
     * Amount of data in buffer has changed.
     * \param Length of data in bytes
     */
    void dataLengthChanged(qint64 duration);

    /**
     * Position of the audio input device has changed.
     * \param position Position in bytes
     */
    void recordPositionChanged(qint64 position);

    /**
     * Position of the audio output device has changed.
     * \param position Position in bytes
     */
    void playPositionChanged(qint64 position);

    /**
     * Level changed
     * \param rmsLevel RMS level in range 0.0 - 1.0
     * \param peakLevel Peak level in range 0.0 - 1.0
     * \param numSamples Number of audio samples analyzed
     */
    void levelChanged(qreal rmsLevel, qreal peakLevel, int numSamples);

    /**
     * Spectrum has changed.
     * \param position Position of start of window in bytes
     * \param length   Length of window in bytes
     * \param spectrum Resulting frequency spectrum
     */
    void spectrumChanged(qint64 position, qint64 length, const FrequencySpectrum &spectrum);

    /**
     * Buffer containing audio data has changed.
     * \param position Position of start of buffer in bytes
     * \param buffer   Buffer
     */
    void bufferChanged(qint64 position, qint64 length, const QByteArray &buffer);

private slots:
    void initAudioDevices();
    void audioNotify();
    void audioStateChanged(QAudio::State state);
    void audioDataReady();
    void spectrumChanged(const FrequencySpectrum &spectrum);

private:
    void resetAudioDevices();
    bool initialize();
    bool selectFormat();
    void stopRecording();
    void stopPlayback();
    void setState(QAudio::State state);
    void setState(QAudioDevice::Mode mode, QAudio::State state);
    void setFormat(const QAudioFormat &format);
    void setRecordPosition(qint64 position, bool forceEmit = false);
    void setPlayPosition(qint64 position, bool forceEmit = false);
    void calculateLevel(qint64 position, qint64 length);
    void calculateSpectrum(qint64 position);
    void setLevel(qreal rmsLevel, qreal peakLevel, int numSamples);
    void emitError(QAudio::Error error);

#ifdef DUMP_DATA
    void createOutputDir();
    QString outputPath() const
    {
        return m_outputDir.path();
    }
#endif

#ifdef DUMP_CAPTURED_AUDIO
    void dumpData();
#endif

private:
    QAudioDevice::Mode m_mode;
    QAudio::State m_state;
    QMediaDevices *m_devices;

    bool m_generateTone;
    SweptTone m_tone;

    QWaveDecoder *m_file;
    // We need a second file handle via which to read data into m_buffer
    // for analysis
    QWaveDecoder *m_analysisFile;

    QAudioFormat m_format;

    QList<QAudioDevice> m_availableAudioInputDevices;
    QAudioDevice m_audioInputDevice;
    QAudioSource *m_audioInput;
    QIODevice *m_audioInputIODevice;
    qint64 m_recordPosition;

    QList<QAudioDevice> m_availableAudioOutputDevices;
    QAudioDevice m_audioOutputDevice;
    QAudioSink *m_audioOutput;
    qint64 m_playPosition;
    QBuffer m_audioOutputIODevice;

    QByteArray m_buffer;
    qint64 m_bufferPosition;
    qint64 m_bufferLength;
    qint64 m_dataLength;

    int m_levelBufferLength;
    qreal m_rmsLevel;
    qreal m_peakLevel;

    int m_spectrumBufferLength;
    QByteArray m_spectrumBuffer;
    SpectrumAnalyser m_spectrumAnalyser;
    qint64 m_spectrumPosition;

    int m_count;
    QTimer *m_notifyTimer = nullptr;

#ifdef DUMP_DATA
    QDir m_outputDir;
#endif
};

#endif // ENGINE_H
