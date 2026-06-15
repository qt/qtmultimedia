// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qwavedecoder.h"

#include <QtCore/qdebug.h>
#include <QtCore/qendian.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qtimer.h>
#include <QtCore/qbytearray.h>

#include <limits.h>

#include <dr_wav.h>

QT_BEGIN_NAMESPACE

#if QT_DEPRECATED_SINCE(6, 11)

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

QWaveDecoder::~QWaveDecoder()
{
    m_headerBuf.reset();
}

bool QWaveDecoder::open(QIODevice::OpenMode mode)
{
    bool canOpen = false;
    if (mode & QIODevice::ReadOnly && mode & ~QIODevice::WriteOnly) {
        canOpen = QIODevice::open(mode | QIODevice::Unbuffered);
        if (canOpen) {
            m_headerBuf = std::make_unique<QByteArray>();
            connect(device, &QIODevice::readyRead, this, &QWaveDecoder::handleData);
            // Try immediately if data already available
            if (device->bytesAvailable() > 0)
                handleData();
        }
        return canOpen;
    }

    if (mode & QIODevice::WriteOnly) {
        if (format.sampleFormat() != QAudioFormat::Int16)
            return false;
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

    m_headerBuf.reset();

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

    // Align to sample boundary
    maxlen = (maxlen / bytesPerSample) * bytesPerSample;
    if (maxlen == 0)
        return 0;

    qint64 totalRead = 0;
    char *dst = data;

    // For sequential devices, drain the PCM prefix buffer first.
    // This buffer holds bytes already consumed from the device during header parsing.
    if (m_headerBuf && !m_headerBuf->isEmpty()) {
        qint64 fromBuf = qMin(maxlen, qint64(m_headerBuf->size()));
        // Align to sample boundary
        fromBuf = (fromBuf / bytesPerSample) * bytesPerSample;
        if (fromBuf > 0) {
            memcpy(dst, m_headerBuf->constData(), size_t(fromBuf));
            m_headerBuf->remove(0, static_cast<qsizetype>(fromBuf));
            totalRead += fromBuf;
            dst       += fromBuf;
            maxlen    -= fromBuf;
        }
        if (m_headerBuf->isEmpty())
            m_headerBuf.reset();
    }

    // Read remainder from device
    if (maxlen > 0) {
        qint64 read = device->read(dst, maxlen);
        if (read > 0)
            totalRead += read;
    }

    // Byte-swap the entire output for big-endian (RIFX) WAV on LE host (or vice versa)
    if (m_byteSwap && format.bytesPerFrame() > 1 && totalRead > 0) {
        qint64 nSamples = totalRead / bytesPerSample;
        switch (bytesPerSample) {
        case 2: qbswap<2>(data, qsizetype(nSamples), data); break;
        case 4: qbswap<4>(data, qsizetype(nSamples), data); break;
        default: Q_UNREACHABLE();
        }
    }

    return totalRead;
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
    return false;
#endif

    CombinedHeader header;
    memset(&header, 0, HeaderLength);

    memcpy(header.riff.descriptor.id, "RIFF", 4);
    qToLittleEndian<quint32>(quint32(dataSize + HeaderLength - 8),
                             reinterpret_cast<unsigned char*>(&header.riff.descriptor.size));
    memcpy(header.riff.type, "WAVE", 4);

    memcpy(header.wave.descriptor.id, "fmt ", 4);
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

    memcpy(header.data.descriptor.id, "data", 4);
    qToLittleEndian<quint32>(quint32(dataSize),
                             reinterpret_cast<unsigned char*>(&header.data.descriptor.size));

    return device->write(reinterpret_cast<const char *>(&header), HeaderLength);
}

bool QWaveDecoder::writeDataLength()
{
#ifndef Q_LITTLE_ENDIAN
    return false;
#endif
    if (isSequential())
        return false;

    if (!device->seek(4)) {
        qDebug() << "can't seek";
        return false;
    }

    quint32 length = quint32(dataSize + HeaderLength - 8);
    if (device->write(reinterpret_cast<const char *>(&length), 4) != 4)
        return false;

    if (!device->seek(40))
        return false;

    return device->write(reinterpret_cast<const char *>(&dataSize), 4);
}

void QWaveDecoder::parsingFailed()
{
    m_headerBuf.reset();

    Q_ASSERT(device);
    disconnect(device, &QIODevice::readyRead, this, &QWaveDecoder::handleData);
    emit parsingError();
}

void QWaveDecoder::handleData()
{
    using namespace QtPrivate;

    if (openMode() == QIODevice::WriteOnly)
        return;

    if (haveFormat) {
        // Already parsed — relay readyRead from device to our listeners
        disconnect(device, &QIODevice::readyRead, this, &QWaveDecoder::handleData);
        connect(device, &QIODevice::readyRead, this, &QIODevice::readyRead);
        return;
    }

    if (!m_headerBuf)
        return;

    // Accumulate all available bytes into the header buffer
    QByteArray incoming = device->readAll();
    if (!incoming.isEmpty())
        m_headerBuf->append(incoming);

    // Need at least the RIFF header + fmt chunk to attempt parsing
    if (m_headerBuf->size() < int(sizeof(RIFFHeader)) + int(sizeof(chunk)))
        return;

    // Try to parse the accumulated buffer with dr_wav in-memory mode
    drwav wav;
    if (!drwav_init_memory(&wav, m_headerBuf->constData(), size_t(m_headerBuf->size()), nullptr)) {
        // Not enough data yet — wait for more readyRead signals
        // (but only if the device isn't done)
        if (device->atEnd())
            parsingFailed();
        return;
    }

    // dr_wav parsed the header successfully. Extract what we need.
    drwav_uint16 audioFormat = drwav_fmt_get_format(&wav.fmt);

    if (audioFormat != 0 && audioFormat != 1) {
        // Not PCM (e.g. float, ADPCM, extensible) — reject
        drwav_uninit(&wav);
        parsingFailed();
        return;
    }

    int bitsPerSample = wav.bitsPerSample;
    int sampleRate = int(wav.sampleRate);
    int channels = int(wav.channels);

    // Only 8-bit and 16-bit PCM supported
    QAudioFormat::SampleFormat fmt = QAudioFormat::Unknown;
    switch (bitsPerSample) {
    case 8:  fmt = QAudioFormat::UInt8; break;
    case 16: fmt = QAudioFormat::Int16; break;
    default: break; // 24-bit, 32-bit, float — rejected
    }

    if (fmt == QAudioFormat::Unknown || sampleRate == 0 || channels == 0) {
        drwav_uninit(&wav);
        parsingFailed();
        return;
    }

    // Endianness: RIFX = big-endian container
    bool bigEndian = (wav.container == drwav_container_rifx);
    m_byteSwap = (bigEndian != (QSysInfo::ByteOrder == QSysInfo::BigEndian));

    qint64 dataChunkDataPos = qint64(wav.dataChunkDataPos);
    dataSize = qint64(wav.dataChunkDataSize);

    drwav_uninit(&wav);

    // Seek the device to the PCM data start
    if (!device->isSequential()) {
        if (!device->seek(dataChunkDataPos)) {
            parsingFailed();
            return;
        }
        // No longer need the header buffer for non-sequential
        m_headerBuf.reset();
    } else {
        // Sequential device: m_headerBuf already holds all bytes consumed so far.
        // Bytes [0..dataChunkDataPos-1] = WAV header.
        // Bytes [dataChunkDataPos..m_headerBuf->size()-1] = PCM prefix already read.
        // Trim the buffer to keep only the PCM prefix.
        if (dataChunkDataPos < m_headerBuf->size()) {
            *m_headerBuf = m_headerBuf->mid(qsizetype(dataChunkDataPos));
        } else {
            m_headerBuf->clear();
        }
    }

    format.setSampleFormat(fmt);
    format.setSampleRate(sampleRate);
    format.setChannelCount(channels);

    if (!dataSize)
        dataSize = device->size() - dataChunkDataPos;

    haveFormat = true;
    disconnect(device, &QIODevice::readyRead, this, &QWaveDecoder::handleData);
    connect(device, &QIODevice::readyRead, this, &QIODevice::readyRead);
    emit formatKnown();
}

#endif // QT_DEPRECATED_SINCE(6, 11)

QT_END_NAMESPACE

#include "moc_qwavedecoder.cpp"
