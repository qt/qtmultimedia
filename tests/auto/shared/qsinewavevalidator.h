// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSINEWAVEVALIDATOR_H
#define QSINEWAVEVALIDATOR_H

#include <QtCore/qmath.h>

#include <array>

#include <QtCore/QDebug>

// sine wave validator: accumulating peak value and result of passing the signal through a notch
// filter. A sine wave of that frequency should not pass the notch filter, so if the peak exceeds a
// small threshold, it we can detect discontinuities for example.
struct QSineWaveValidator
{
    QSineWaveValidator(const QSineWaveValidator &) = delete;
    QSineWaveValidator(QSineWaveValidator &&) = delete;
    QSineWaveValidator &operator=(const QSineWaveValidator &) = delete;
    QSineWaveValidator &operator=(QSineWaveValidator &&) = delete;

    QSineWaveValidator(float notchFrequency, float sampleRate)
    {
        using namespace std;
        float pi = M_PI;
        float f0 = notchFrequency;
        float Fs = sampleRate;
        float Q = 1 / sqrt(2.f); // higher Q gives narrow bandwidth, but requires a larger number of
                                 // frames during the transient

        // compare https://webaudio.github.io/Audio-EQ-Cookbook/Audio-EQ-Cookbook.txt
        float w0 = 2 * pi * f0 / Fs;
        float alpha = sin(w0) / (2 * Q);

        b0 = 1;
        b1 = -2 * cos(w0);
        b2 = 1;
        a0 = 1 + alpha;
        a1 = -2 * cos(w0);
        a2 = 1 - alpha;
    }

    void feedSample(float sample)
    {
        if (pendingFramesBeforeAnalysis == framesBeforeAnalysis) {
            x.fill(sample);
            y.fill(b0 / a0 * sample);
        }

        x[2] = x[1];
        x[1] = x[0];
        x[0] = sample;

        y[2] = y[1];
        y[1] = y[0];
        y[0] = (b0 / a0) * x[0] + (b1 / a0) * x[1] + (b2 / a0) * x[2] - (a1 / a0) * y[1]
                - (a2 / a0) * y[2];

        accumPeak = std::max(std::abs(sample), accumPeak);

        if (pendingFramesBeforeAnalysis) {
            pendingFramesBeforeAnalysis -= 1;
            return;
        }

        accumNotchPeak = std::max(std::abs(y[0]), accumNotchPeak);
    }

    float peak() const { return accumPeak; }
    float notchPeak() const
    {
        if (pendingFramesBeforeAnalysis)
            qWarning() << "notchPeak during initial frames. Result will not be accurate";
        return accumNotchPeak;
    }

private:
    float a0, a1, a2;
    float b0, b1, b2;

    std::array<float, 3> x{}, y{};
    int framesBeforeAnalysis = 128; // unscientific estimate, larger than the transient response
                                    // time for the IIR filter
    int pendingFramesBeforeAnalysis = framesBeforeAnalysis;

    float accumPeak{};
    float accumNotchPeak{};
};

#endif // QSINEWAVEVALIDATOR_H
