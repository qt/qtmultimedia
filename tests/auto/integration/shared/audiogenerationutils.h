// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef AUDIOGENERATIONUTILS_H
#define AUDIOGENERATIONUTILS_H

#include <QAudioFormat>
#include <chrono>
#include <limits>

QT_BEGIN_NAMESPACE

inline QByteArray createSineWaveData(const QAudioFormat &format, std::chrono::microseconds duration,
                                     qint32 sampleIndex = 0, qreal frequency = 500,
                                     qreal volume = 0.8)
{
    if (!format.isValid())
        return {};

    const qint32 length = format.bytesForDuration(duration.count());

    QByteArray data(format.bytesForDuration(duration.count()), Qt::Uninitialized);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(data.data());
    const auto end = ptr + length;

    auto writeNextFrame = [&](auto value) {
        Q_ASSERT(sizeof(value) == format.bytesPerSample());
        *reinterpret_cast<decltype(value) *>(ptr) = value;
        ptr += sizeof(value);
    };

    for (; ptr < end; ++sampleIndex) {
        const qreal x = sin(2 * M_PI * frequency * sampleIndex / format.sampleRate()) * volume;
        for (int ch = 0; ch < format.channelCount(); ++ch) {
            switch (format.sampleFormat()) {
            case QAudioFormat::UInt8:
                writeNextFrame(static_cast<quint8>(std::round((1.0 + x) / 2 * 255)));
                break;
            case QAudioFormat::Int16:
                writeNextFrame(
                        static_cast<qint16>(std::round(x * std::numeric_limits<qint16>::max())));
                break;
            case QAudioFormat::Int32:
                writeNextFrame(
                        static_cast<qint32>(std::round(x * std::numeric_limits<qint32>::max())));
                break;
            case QAudioFormat::Float:
                writeNextFrame(static_cast<float>(x));
                break;
            case QAudioFormat::Unknown:
            case QAudioFormat::NSampleFormats:
                break;
            }
        }
    }

    Q_ASSERT(ptr == end);

    return data;
}

QT_END_NAMESPACE

#endif // AUDIOGENERATIONUTILS_H
