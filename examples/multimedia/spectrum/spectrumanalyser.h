// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPECTRUMANALYSER_H
#define SPECTRUMANALYSER_H

#include "frequencyspectrum.h"
#include "spectrum.h"

#include <QByteArray>
#include <QList>
#include <QObject>

#ifdef DUMP_SPECTRUMANALYSER
#    include <QDir>
#    include <QFile>
#    include <QTextStream>
#endif

#ifndef DISABLE_FFT
#    include "FFTRealFixLenParam.h"
#endif

QT_FORWARD_DECLARE_CLASS(QAudioFormat)
QT_FORWARD_DECLARE_CLASS(QThread)

class FFTRealWrapper;

class SpectrumAnalyserThreadPrivate;

/**
 * Implementation of the spectrum analysis which can be run in a
 * separate thread.
 */
class SpectrumAnalyserThread : public QObject
{
    Q_OBJECT

public:
    SpectrumAnalyserThread(QObject *parent);
    ~SpectrumAnalyserThread();

public slots:
    void setWindowFunction(WindowFunction type);
    void calculateSpectrum(const QByteArray &buffer, int inputFrequency, int bytesPerSample);

signals:
    void calculationComplete(const FrequencySpectrum &spectrum);

private:
    void calculateWindow();

private:
#ifndef DISABLE_FFT
    FFTRealWrapper *m_fft;
#endif

    const int m_numSamples;

    WindowFunction m_windowFunction;

#ifdef DISABLE_FFT
    typedef qreal DataType;
#else
    typedef FFTRealFixLenParam::DataType DataType;
#endif
    QList<DataType> m_window;

    QList<DataType> m_input;
    QList<DataType> m_output;

    FrequencySpectrum m_spectrum;

#ifdef SPECTRUM_ANALYSER_SEPARATE_THREAD
    QThread *m_thread;
#endif
};

/**
 * Class which performs frequency spectrum analysis on a window of
 * audio samples, provided to it by the Engine.
 */
class SpectrumAnalyser : public QObject
{
    Q_OBJECT

public:
    SpectrumAnalyser(QObject *parent = nullptr);
    ~SpectrumAnalyser();

#ifdef DUMP_SPECTRUMANALYSER
    void setOutputPath(const QString &outputPath);
#endif

public:
    /*
     * Set the windowing function which is applied before calculating the FFT
     */
    void setWindowFunction(WindowFunction type);

    /*
     * Calculate a frequency spectrum
     *
     * \param buffer       Audio data
     * \param format       Format of audio data
     *
     * Frequency spectrum is calculated asynchronously.  The result is returned
     * via the spectrumChanged signal.
     *
     * An ongoing calculation can be cancelled by calling cancelCalculation().
     *
     */
    void calculate(const QByteArray &buffer, const QAudioFormat &format);

    /*
     * Check whether the object is ready to perform another calculation
     */
    bool isReady() const;

    /*
     * Cancel an ongoing calculation
     *
     * Note that cancelling is asynchronous.
     */
    void cancelCalculation();

signals:
    void spectrumChanged(const FrequencySpectrum &spectrum);

private slots:
    void calculationComplete(const FrequencySpectrum &spectrum);

private:
    void calculateWindow();

private:
    SpectrumAnalyserThread *m_thread;

    enum State { Idle, Busy, Cancelled };

    State m_state;

#ifdef DUMP_SPECTRUMANALYSER
    QDir m_outputDir;
    int m_count;
    QFile m_textFile;
    QTextStream m_textStream;
#endif
};

#endif // SPECTRUMANALYSER_H
