// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "audiogenerationutils_p.h"

#include <QtCore/qscopeguard.h>

#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#define DR_WAV_NO_CONVERSION_API
#define DR_WAV_NO_WCHAR
#define DRWAV_API static
#define DRWAV_PRIVATE static
#define QtPrivate QtPrivateDrwavMultimediaTest
#include <dr_wav.h>
#undef QtPrivate

using namespace QtPrivateDrwavMultimediaTest;

QT_BEGIN_NAMESPACE

unsigned char *writeSineSample(unsigned char *ptr, QAudioFormat::SampleFormat sampleFormat, qreal x)
{
    auto writeNextFrame = [&](auto value) {
        *reinterpret_cast<decltype(value) *>(ptr) = value;
        return ptr + sizeof(value);
    };

    switch (sampleFormat) {
    case QAudioFormat::UInt8:
        return writeNextFrame(quint8(std::round((1.0 + x) / 2 * 255)));
    case QAudioFormat::Int16:
        return writeNextFrame(qint16(std::round(x * std::numeric_limits<qint16>::max())));
    case QAudioFormat::Int32:
        return writeNextFrame(qint32(std::round(x * std::numeric_limits<qint32>::max())));
    case QAudioFormat::Float:
        return writeNextFrame(float(x));
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        break;
    }
    return ptr;
}

QByteArray createSineWaveData(const QAudioFormat &format, std::chrono::microseconds duration,
                              qint32 sampleIndex, qreal frequency, qreal volume)
{
    if (!format.isValid())
        return {};

    const qint32 length = format.bytesForDuration(duration.count());

    QByteArray data(format.bytesForDuration(duration.count()), Qt::Uninitialized);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(data.data());
    const auto end = ptr + length;

    const double initialPhase = 2.0 * M_PI * frequency * sampleIndex / format.sampleRate();
    SineWaveSignal generator(frequency, format.sampleRate(), initialPhase);
    for (double rawSample : generator) {
        if (ptr >= end)
            break;
        const qreal x = rawSample * volume;
        for (int channel = 0; channel < format.channelCount(); ++channel)
            ptr = writeSineSample(ptr, format.sampleFormat(), x);
    }

    Q_ASSERT(ptr == end);

    return data;
}

std::unique_ptr<QTemporaryFile> makeMonoPcm16WavFile(int sampleRate,
                                                     std::chrono::microseconds duration,
                                                     qreal frequency, qreal volume)
{
    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Int16);
    format.setChannelCount(1);
    format.setSampleRate(sampleRate);

    const QByteArray pcmData =
            createSineWaveData(format, duration, /*sampleIndex=*/0, frequency, volume);

    drwav_data_format wavFormat{};
    wavFormat.container = drwav_container_riff;
    wavFormat.format = DR_WAVE_FORMAT_PCM;
    wavFormat.channels = 1;
    wavFormat.sampleRate = drwav_uint32(sampleRate);
    wavFormat.bitsPerSample = 16;

    void *wavData = nullptr;
    size_t wavDataSize = 0;
    drwav wav;
    if (!drwav_init_memory_write(&wav, &wavData, &wavDataSize, &wavFormat, nullptr))
        return nullptr;

    auto dataGuard = qScopeGuard([&] {
        drwav_uninit(&wav);
        drwav_free(wavData, nullptr);
    });

    const auto framesToWrite = drwav_uint64(pcmData.size() / sizeof(qint16));
    if (drwav_write_pcm_frames(&wav, framesToWrite, pcmData.constData()) != framesToWrite)
        return nullptr;

    auto file = std::make_unique<QTemporaryFile>();
    if (!file->open())
        return nullptr;
    file->write(static_cast<const char *>(wavData), qint64(wavDataSize));
    file->close();

    return file;
}

QT_END_NAMESPACE
