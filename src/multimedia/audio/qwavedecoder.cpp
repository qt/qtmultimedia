// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qwavedecoder.h"

#include <QtCore/qtimer.h>
#include <QtCore/qendian.h>
#include <limits.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

QWaveDecoder::QWaveDecoder(QIODevice *device, QObject *parent)
    : QIODevice(parent),
      device(device)
{
}

QWaveDecoder::QWaveDecoder(QIODevice *device, const QAudioFormat &format, QObject *parent)
    : QIODevice(parent),
        device(device),
        format(format)
{
}

QWaveDecoder::~QWaveDecoder() = default;

bool QWaveDecoder::open(QIODevice::OpenMode mode)
{
    bool canOpen = false;
    if (mode & QIODevice::ReadOnly && mode & ~QIODevice::WriteOnly) {
        canOpen = QIODevice::open(mode | QIODevice::Unbuffered);
        if (canOpen && enoughDataAvailable())
            handleData();
        else
            connect(device, &QIODevice::readyRead, this, &QWaveDecoder::handleData);
        return canOpen;
    }

    if (mode & QIODevice::WriteOnly) {
        if (format.sampleFormat() != QAudioFormat::Int16)
            return false; // data format is not supported
        canOpen = QIODevice::open(mode);
        if (canOpen && writeHeader())
            haveHeader = true;
        return canOpen;
    }
    return QIODevice::open(mode);
}

void QWaveDecoder::close()
{
    if (isOpen() && (openMode() & QIODevice::WriteOnly)) {
        Q_ASSERT(dataSize < INT_MAX);
        if (!device->isOpen() || !writeDataLength())
            qWarning() << "Failed to finalize wav file";
    }
    QIODevice::close();
}

bool QWaveDecoder::seek(qint64 pos)
{
    return device->seek(pos);
}

qint64 QWaveDecoder::pos() const
{
    return device->pos();
}

void QWaveDecoder::setIODevice(QIODevice * /* device */)
{
}

QAudioFormat QWaveDecoder::audioFormat() const
{
    return format;
}

QIODevice* QWaveDecoder::getDevice()
{
    return device;
}

int QWaveDecoder::duration() const
{
    if (openMode() & QIODevice::WriteOnly)
        return 0;
    int bytesPerSec = format.bytesPerFrame() * format.sampleRate();
    return bytesPerSec ? size() * 1000 / bytesPerSec : 0;
}

qint64 QWaveDecoder::size() const
{
    if (openMode() & QIODevice::ReadOnly) {
        if (!haveFormat)
            return 0;
        if (bps == 24)
            return dataSize*2/3;
        return dataSize;
    } else {
        return device->size();
    }
}

bool QWaveDecoder::isSequential() const
{
    return device->isSequential();
}

qint64 QWaveDecoder::bytesAvailable() const
{
    return haveFormat ? device->bytesAvailable() : 0;
}

qint64 QWaveDecoder::headerLength()
{
    return HeaderLength;
}

qint64 QWaveDecoder::readData(char *data, qint64 maxlen)
{
    const int bytesPerSample = format.bytesPerSample();
    if (!haveFormat || bytesPerSample == 0)
        return 0;

    if (bps == 24) {
        // 24 bit WAV, read in as 16 bit
        qint64 l = 0;
        while (l < maxlen - 1) {
            char tmp[3];
            device->read(tmp, 3);
            if (byteSwap)
                qSwap(tmp[0], tmp[2]);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            data[0] = tmp[0];
            data[1] = tmp[1];
#else
            data[0] = tmp[1];
            data[1] = tmp[2];
#endif
            data += 2;
            l += 2;
        }
        return l;
    }

    qint64 nSamples = maxlen / bytesPerSample;
    maxlen = nSamples * bytesPerSample;
    int read = device->read(data, maxlen);

    if (!byteSwap || format.bytesPerFrame() == 1)
        return read;

    nSamples = read / bytesPerSample;
    switch (bytesPerSample) {
    case 2:
        qbswap<2>(data, nSamples, data);
        break;
    case 4:
        qbswap<4>(data, nSamples, data);
        break;
    default:
        Q_UNREACHABLE();
    }
    return read;

}

qint64 QWaveDecoder::writeData(const char *data, qint64 len)
{
    if (!haveHeader)
        return 0;
    qint64 written = device->write(data, len);
    dataSize += written;
    return written;
}

bool QWaveDecoder::writeHeader()
{
    if (device->size() != 0)
        return false;

#ifndef Q_LITTLE_ENDIAN
    // only implemented for LITTLE ENDIAN
    return false;
#endif

    CombinedHeader header;

    memset(&header, 0, HeaderLength);

    // RIFF header
    memcpy(header.riff.descriptor.id,"RIFF",4);
    qToLittleEndian<quint32>(quint32(dataSize + HeaderLength - 8),
                             reinterpret_cast<unsigned char*>(&header.riff.descriptor.size));
    memcpy(header.riff.type, "WAVE",4);

    // WAVE header
    memcpy(header.wave.descriptor.id,"fmt ",4);
    qToLittleEndian<quint32>(quint32(16),
                             reinterpret_cast<unsigned char*>(&header.wave.descriptor.size));
    qToLittleEndian<quint16>(quint16(1),
                             reinterpret_cast<unsigned char*>(&header.wave.audioFormat));
    qToLittleEndian<quint16>(quint16(format.channelCount()),
                             reinterpret_cast<unsigned char*>(&header.wave.numChannels));
    qToLittleEndian<quint32>(quint32(format.sampleRate()),
                             reinterpret_cast<unsigned char*>(&header.wave.sampleRate));
    qToLittleEndian<quint32>(quint32(format.sampleRate() * format.bytesPerFrame()),
                             reinterpret_cast<unsigned char*>(&header.wave.byteRate));
    qToLittleEndian<quint16>(quint16(format.channelCount() * format.bytesPerSample()),
                             reinterpret_cast<unsigned char*>(&header.wave.blockAlign));
    qToLittleEndian<quint16>(quint16(format.bytesPerSample() * 8),
                             reinterpret_cast<unsigned char*>(&header.wave.bitsPerSample));

    // DATA header
    memcpy(header.data.descriptor.id,"data",4);
    qToLittleEndian<quint32>(quint32(dataSize),
                             reinterpret_cast<unsigned char*>(&header.data.descriptor.size));

    return device->write(reinterpret_cast<const char *>(&header), HeaderLength);
}

bool QWaveDecoder::writeDataLength()
{
#ifndef Q_LITTLE_ENDIAN
    // only implemented for LITTLE ENDIAN
    return false;
#endif

    if (isSequential())
        return false;

    // seek to RIFF header size, see header.riff.descriptor.size above
    if (!device->seek(4)) {
        qDebug() << "can't seek";
        return false;
    }

    quint32 length = dataSize + HeaderLength - 8;
    if (device->write(reinterpret_cast<const char *>(&length), 4) != 4)
        return false;

    // seek to DATA header size, see header.data.descriptor.size above
    if (!device->seek(40))
        return false;

    return device->write(reinterpret_cast<const char *>(&dataSize), 4);
}

void QWaveDecoder::parsingFailed()
{
    Q_ASSERT(device);
    disconnect(device, &QIODevice::readyRead, this, &QWaveDecoder::handleData);
    emit parsingError();
}

void QWaveDecoder::handleData()
{
    if (openMode() == QIODevice::WriteOnly)
        return;

    // As a special "state", if we have junk to skip, we do
    if (junkToSkip > 0) {
        discardBytes(junkToSkip); // this also updates junkToSkip

        // If we couldn't skip all the junk, return
        if (junkToSkip > 0) {
            // We might have run out
            if (device->atEnd())
                parsingFailed();
            return;
        }
    }

    if (state == QWaveDecoder::InitialState) {
        if (device->bytesAvailable() < qint64(sizeof(RIFFHeader)))
            return;

        RIFFHeader riff;
        device->read(reinterpret_cast<char *>(&riff), sizeof(RIFFHeader));

        // RIFF = little endian RIFF, RIFX = big endian RIFF
        if (((qstrncmp(riff.descriptor.id, "RIFF", 4) != 0) && (qstrncmp(riff.descriptor.id, "RIFX", 4) != 0))
                || qstrncmp(riff.type, "WAVE", 4) != 0) {
            parsingFailed();
            return;
        }

        state = QWaveDecoder::WaitingForFormatState;
        bigEndian = (qstrncmp(riff.descriptor.id, "RIFX", 4) == 0);
        byteSwap = (bigEndian != (QSysInfo::ByteOrder == QSysInfo::BigEndian));
    }

    if (state == QWaveDecoder::WaitingForFormatState) {
        if (findChunk("fmt ")) {
            chunk descriptor;
            const bool peekSuccess = peekChunk(&descriptor);
            Q_ASSERT(peekSuccess);

            quint32 rawChunkSize = descriptor.size + sizeof(chunk);
            if (device->bytesAvailable() < qint64(rawChunkSize))
                return;

            WAVEHeader wave;
            device->read(reinterpret_cast<char *>(&wave), sizeof(WAVEHeader));

            if (rawChunkSize > sizeof(WAVEHeader))
                discardBytes(rawChunkSize - sizeof(WAVEHeader));

            // Swizzle this
            if (bigEndian) {
                wave.audioFormat = qFromBigEndian<quint16>(wave.audioFormat);
            } else {
                wave.audioFormat = qFromLittleEndian<quint16>(wave.audioFormat);
            }

            if (wave.audioFormat != 0 && wave.audioFormat != 1) {
                // 32bit wave files have format == 0xFFFE (WAVE_FORMAT_EXTENSIBLE).
                // but don't support them at the moment.
                parsingFailed();
                return;
            }

            int rate;
            int channels;
            if (bigEndian) {
                 bps = qFromBigEndian<quint16>(wave.bitsPerSample);
                 rate = qFromBigEndian<quint32>(wave.sampleRate);
                 channels = qFromBigEndian<quint16>(wave.numChannels);
            } else {
                bps = qFromLittleEndian<quint16>(wave.bitsPerSample);
                rate = qFromLittleEndian<quint32>(wave.sampleRate);
                channels = qFromLittleEndian<quint16>(wave.numChannels);
            }

            QAudioFormat::SampleFormat fmt = QAudioFormat::Unknown;
            switch(bps) {
            case 8:
                fmt = QAudioFormat::UInt8;
                break;
            case 16:
                fmt = QAudioFormat::Int16;
                break;
            case 24:
                fmt = QAudioFormat::Int16;
                break;
            case 32:
                fmt = QAudioFormat::Int32;
                break;
            }
            if (fmt == QAudioFormat::Unknown || rate == 0 || channels == 0) {
                parsingFailed();
                return;
            }

            format.setSampleFormat(fmt);
            format.setSampleRate(rate);
            format.setChannelCount(channels);

            state = QWaveDecoder::WaitingForDataState;
        }
    }

    if (state == QWaveDecoder::WaitingForDataState) {
        if (findChunk("data")) {
            disconnect(device, &QIODevice::readyRead, this, &QWaveDecoder::handleData);

            chunk descriptor;
            device->read(reinterpret_cast<char *>(&descriptor), sizeof(chunk));
            if (bigEndian)
                descriptor.size = qFromBigEndian<quint32>(descriptor.size);
            else
                descriptor.size = qFromLittleEndian<quint32>(descriptor.size);

            dataSize = descriptor.size; //means the data size from the data header, not the actual file size
            if (!dataSize)
                dataSize = device->size() - headerLength();

            haveFormat = true;
            connect(device, &QIODevice::readyRead, this, &QIODevice::readyRead);
            emit formatKnown();

            return;
        }
    }

    // If we hit the end without finding data, it's a parsing error
    if (device->atEnd()) {
        parsingFailed();
    }
}

bool QWaveDecoder::enoughDataAvailable()
{
    chunk descriptor;
    if (!peekChunk(&descriptor, false))
        return false;

    // This is only called for the RIFF/RIFX header, before bigEndian is set,
    // so we have to manually swizzle
    if (qstrncmp(descriptor.id, "RIFX", 4) == 0)
        descriptor.size = qFromBigEndian<quint32>(descriptor.size);
    if (qstrncmp(descriptor.id, "RIFF", 4) == 0)
        descriptor.size = qFromLittleEndian<quint32>(descriptor.size);

    if (device->bytesAvailable() < qint64(sizeof(chunk)) + descriptor.size)
        return false;

    return true;
}

bool QWaveDecoder::findChunk(const char *chunkId)
{
    chunk descriptor;

    do {
        if (!peekChunk(&descriptor))
            return false;

        if (qstrncmp(descriptor.id, chunkId, 4) == 0)
            return true;

        // A program reading a RIFF file can skip over any chunk whose chunk
        // ID it doesn't recognize; it simply skips the number of bytes specified
        // by ckSize plus the pad byte, if present. See Multimedia Programming
        // Interface and Data Specifications 1.0. IBM / Microsoft. August 1991. pp. 10-11.
        const quint32 sizeWithPad = descriptor.size + (descriptor.size & 1);

        // It's possible that bytes->available() is less than the chunk size
        // if it's corrupt.
        junkToSkip = qint64(sizeof(chunk) + sizeWithPad);

        // Skip the current amount
        if (junkToSkip > 0)
            discardBytes(junkToSkip);

        // If we still have stuff left, just exit and try again later
        // since we can't call peekChunk
        if (junkToSkip > 0)
            return false;

    } while (device->bytesAvailable() > 0);

    return false;
}

bool QWaveDecoder::peekChunk(chunk *pChunk, bool handleEndianness)
{
    if (device->bytesAvailable() < qint64(sizeof(chunk)))
        return false;

    if (!device->peek(reinterpret_cast<char *>(pChunk), sizeof(chunk)))
        return false;

    if (handleEndianness) {
        if (bigEndian)
            pChunk->size = qFromBigEndian<quint32>(pChunk->size);
        else
            pChunk->size = qFromLittleEndian<quint32>(pChunk->size);
    }
    return true;
}

void QWaveDecoder::discardBytes(qint64 numBytes)
{
    // Discards a number of bytes
    // If the iodevice doesn't have this many bytes in it,
    // remember how much more junk we have to skip.
    if (device->isSequential()) {
        QByteArray r = device->read(qMin(numBytes, qint64(16384))); // uggh, wasted memory, limit to a max of 16k
        if (r.size() < numBytes)
            junkToSkip = numBytes - r.size();
        else
            junkToSkip = 0;
    } else {
        quint64 origPos = device->pos();
        device->seek(device->pos() + numBytes);
        junkToSkip = origPos + numBytes - device->pos();
    }
}

QT_END_NAMESPACE

#include "moc_qwavedecoder.cpp"
