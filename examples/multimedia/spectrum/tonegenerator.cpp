// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "spectrum.h"
#include "utils.h"

#include <QAudioFormat>
#include <QByteArray>
#include <QtEndian>
#include <QtMath>

void generateTone(const SweptTone &tone, const QAudioFormat &format, QByteArray &buffer)
{
    Q_ASSERT(format.sampleFormat() == QAudioFormat::Int16);

    const int channelBytes = format.bytesPerSample();
    const int sampleBytes = format.channelCount() * channelBytes;
    int length = buffer.size();
    const int numSamples = buffer.size() / sampleBytes;

    Q_ASSERT(length % sampleBytes == 0);
    Q_UNUSED(sampleBytes); // suppress warning in release builds

    unsigned char *ptr = reinterpret_cast<unsigned char *>(buffer.data());

    qreal phase = 0.0;

    const qreal d = 2 * M_PI / format.sampleRate();

    // We can't generate a zero-frequency sine wave
    const qreal startFreq = tone.startFreq ? tone.startFreq : 1.0;

    // Amount by which phase increases on each sample
    qreal phaseStep = d * startFreq;

    // Amount by which phaseStep increases on each sample
    // If this is non-zero, the output is a frequency-swept tone
    const qreal phaseStepStep = d * (tone.endFreq - startFreq) / numSamples;

    while (length) {
        const qreal x = tone.amplitude * qSin(phase);
        const qint16 value = realToPcm(x);
        for (int i = 0; i < format.channelCount(); ++i) {
            qToLittleEndian<qint16>(value, ptr);
            ptr += channelBytes;
            length -= channelBytes;
        }

        phase += phaseStep;
        while (phase > 2 * M_PI)
            phase -= 2 * M_PI;
        phaseStep += phaseStepStep;
    }
}
