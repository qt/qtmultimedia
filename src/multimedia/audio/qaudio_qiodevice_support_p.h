// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIO_QIODEVICE_SUPPORT_P_H
#define QAUDIO_QIODEVICE_SUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qdebug.h>
#include <QtCore/qglobal.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qmutex.h>
#include <QtCore/qspan.h>

#include <QtMultimedia/private/qaudio_alignment_support_p.h>
#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qaudioringbuffer_p.h>

#include <deque>
#include <mutex>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

// QIODevice writing to a QAudioRingBuffer
template <typename SampleType>
class QIODeviceRingBufferWriter : public QIODevice
{
public:
    using Ringbuffer = QtPrivate::QAudioRingBuffer<SampleType>;

    explicit QIODeviceRingBufferWriter(Ringbuffer *rb, QObject *parent = nullptr)
        : QIODevice(parent), m_ringbuffer(rb)
    {
        Q_ASSERT(rb);
    }

private:
    qint64 readData(char * /*data*/, qint64 /*maxlen*/) override { return -1; }

    qint64 writeData(const char *data, qint64 len) override
    {
        using namespace QtMultimediaPrivate; // take/drop

        // we don't write fractional samples
        int64_t usableLength = alignDown(len, sizeof(SampleType));

        QSpan<const std::byte> dataRegion = as_bytes(QSpan{ data, usableLength });

        qint64 totalBytesWritten = 0;

        do {
            int64_t remainingSamples = dataRegion.size() / sizeof(SampleType);
            auto writeRegion = m_ringbuffer->acquireWriteRegion(remainingSamples);
            if (writeRegion.isEmpty())
                break; // no space in buffer

            QSpan<std::byte> writeByteRegion = as_writable_bytes(writeRegion);
            int64_t bytesToWrite = std::min(dataRegion.size(), writeByteRegion.size());

            QSpan<const std::byte> dataForChunk = take(dataRegion, bytesToWrite);
            std::copy(dataForChunk.begin(), dataForChunk.end(), writeByteRegion.begin());

            totalBytesWritten += bytesToWrite;
            dataRegion = drop(dataRegion, bytesToWrite);

            m_ringbuffer->releaseWriteRegion(bytesToWrite / sizeof(SampleType));
        } while (!dataRegion.isEmpty());

        if (totalBytesWritten)
            emit readyRead();

        return totalBytesWritten;
    }

    qint64 bytesToWrite() const override { return m_ringbuffer->free() * sizeof(SampleType); }

    Ringbuffer *m_ringbuffer;
};

// QIODevice reading from a QAudioRingBuffer
template <typename SampleType>
class QIODeviceRingBufferReader : public QIODevice
{
public:
    using Ringbuffer = QtPrivate::QAudioRingBuffer<SampleType>;

    explicit QIODeviceRingBufferReader(Ringbuffer *rb, QObject *parent = nullptr)
        : QIODevice(parent), m_ringbuffer(rb)
    {
        Q_ASSERT(rb);
    }

private:
    qint64 readData(char *data, qint64 maxlen) override
    {
        using namespace QtMultimediaPrivate; // drop

        QSpan<std::byte> outputRegion = as_writable_bytes(QSpan{ data, maxlen });

        qint64 totalBytesRead = 0;

        while (!outputRegion.isEmpty()) {
            int maxSizeToRead = outputRegion.size_bytes() / sizeof(SampleType);
            QSpan readRegion = m_ringbuffer->acquireReadRegion(maxSizeToRead);
            if (readRegion.isEmpty())
                return totalBytesRead;

            QSpan readByteRegion = as_bytes(readRegion);
            std::copy(readByteRegion.begin(), readByteRegion.end(), outputRegion.begin());

            outputRegion = drop(outputRegion, readByteRegion.size());
            totalBytesRead += readByteRegion.size();

            m_ringbuffer->releaseReadRegion(readRegion.size());
        }

        return totalBytesRead;
    }

    qint64 writeData(const char * /*data*/, qint64 /*len*/) override { return -1; }
    qint64 bytesAvailable() const override { return m_ringbuffer->used() * sizeof(SampleType); }

    Ringbuffer *m_ringbuffer;
};

// QIODevice backed by a std::deque
class QDequeIODevice : public QIODevice
{
public:
    using Deque = std::deque<char>;

    explicit QDequeIODevice(QObject *parent = nullptr) : QIODevice(parent) { }

    qsizetype bytesAvailable() const override { return m_deque.size(); }

private:
    qint64 readData(char *data, qint64 maxlen) override
    {
        std::lock_guard guard{ m_mutex };

        size_t bytesToRead = std::min<size_t>(m_deque.size(), maxlen);
        std::copy_n(m_deque.begin(), bytesToRead, data);

        m_deque.erase(m_deque.begin(), m_deque.begin() + bytesToRead);
        return qint64(bytesToRead);
    }

    qint64 writeData(const char *data, qint64 len) override
    {
        std::lock_guard guard{ m_mutex };
        m_deque.insert(m_deque.end(), data, data + len);
        return len;
    }

    QMutex m_mutex;
    Deque m_deque;
};

inline qint64 writeToDevice(QIODevice &device, QSpan<const std::byte> data)
{
    return device.write(reinterpret_cast<const char *>(data.data()), data.size());
}

inline qint64 readFromDevice(QIODevice &device, QSpan<std::byte> outputBuffer)
{
    return device.read(reinterpret_cast<char *>(outputBuffer.data()), outputBuffer.size());
}

template <typename SampleType>
qsizetype pullFromQIODeviceToRingbuffer(QIODevice &device, QAudioRingBuffer<SampleType> &ringbuffer)
{
    using namespace QtMultimediaPrivate;

    qsizetype totalBytesWritten = 0;

    for (;;) {
        qint64 bytesAvailableInDevice = alignDown(device.bytesAvailable(), sizeof(SampleType));
        if (!bytesAvailableInDevice)
            return totalBytesWritten; // no data in iodevice

        qint64 samplesAvailableInDevice = bytesAvailableInDevice / sizeof(SampleType);

        auto writeRegion = ringbuffer.acquireWriteRegion(samplesAvailableInDevice);
        if (writeRegion.empty())
            return totalBytesWritten;

        qint64 bytesRead = readFromDevice(device, as_writable_bytes(writeRegion));
        if (bytesRead < 0) {
            qWarning() << "pullFromQIODeviceToRingbuffer cannot read from QIODevice:"
                       << device.errorString();
            return totalBytesWritten;
        }

        Q_ASSERT(bytesRead == writeRegion.size_bytes());
        ringbuffer.releaseWriteRegion(writeRegion.size());

        totalBytesWritten += writeRegion.size_bytes();
    }
}

template <typename SampleType>
qsizetype pushToQIODeviceFromRingbuffer(QIODevice &device, QAudioRingBuffer<SampleType> &ringbuffer)
{
    using namespace QtMultimediaPrivate;
    qsizetype totalBytesWritten = 0;

    for (;;) {
        auto ringbufferRegion = ringbuffer.acquireReadRegion(ringbuffer.size());
        if (ringbufferRegion.empty())
            return totalBytesWritten;
        QSpan bufferByteRegion = as_bytes(ringbufferRegion);

        int deviceBytesToWrite = device.bytesToWrite();
        if (deviceBytesToWrite > 0) {
            // we do our best effort and only push full samples to the device
            int bytesToWrite = alignDown(deviceBytesToWrite, sizeof(SampleType));
            bufferByteRegion = take(bufferByteRegion, bytesToWrite);

            int bytesWritten = writeToDevice(device, bufferByteRegion);

            if (bytesWritten < 0) {
                qWarning() << "pushToQIODeviceFromRingbuffer cannot push data to QIODevice:"
                           << device.errorString();
                return totalBytesWritten;
            }
            if (bytesWritten == 0)
                return totalBytesWritten;
            totalBytesWritten += bytesWritten;
            Q_ASSERT(isAligned(bytesWritten, sizeof(SampleType)));
            int samplesWritten = bytesWritten / sizeof(SampleType);
            ringbuffer.releaseReadRegion(samplesWritten);
        } else {
            // we don't know how many bytes to write, so we end up filling as much as possible
            int bytesWritten = writeToDevice(device, bufferByteRegion);
            int samplesWritten = bytesWritten / sizeof(SampleType);
            ringbuffer.releaseReadRegion(samplesWritten);
        }
    }
}

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QAUDIO_QIODEVICE_SUPPORT_P_H
