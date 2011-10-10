/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwavedecoder_p.h"

#include <QtCore/qtimer.h>
#include <QtCore/qendian.h>

QT_BEGIN_NAMESPACE

QWaveDecoder::QWaveDecoder(QIODevice *s, QObject *parent):
    QIODevice(parent),
    haveFormat(false),
    dataSize(0),
    remaining(0),
    source(s),
    state(QWaveDecoder::InitialState)
{
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);

    if (enoughDataAvailable())
        QTimer::singleShot(0, this, SLOT(handleData()));
    else
        connect(source, SIGNAL(readyRead()), SLOT(handleData()));
}

QWaveDecoder::~QWaveDecoder()
{
}

QAudioFormat QWaveDecoder::audioFormat() const
{
    return format;
}

int QWaveDecoder::duration() const
{
    return size() * 1000 / (format.sampleSize() / 8) / format.channels() / format.frequency();
}

qint64 QWaveDecoder::size() const
{
    return haveFormat ? dataSize : 0;
}

bool QWaveDecoder::isSequential() const
{
    return source->isSequential();
}

qint64 QWaveDecoder::bytesAvailable() const
{
    return haveFormat ? source->bytesAvailable() : 0;
}

qint64 QWaveDecoder::readData(char *data, qint64 maxlen)
{
    return haveFormat ? source->read(data, maxlen) : 0;
}

qint64 QWaveDecoder::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return -1;
}

void QWaveDecoder::handleData()
{
    bool valid = true;
    if (state == QWaveDecoder::InitialState) {
        if (source->bytesAvailable() < qint64(sizeof(RIFFHeader)))
            return;

        RIFFHeader riff;
        source->read(reinterpret_cast<char *>(&riff), sizeof(RIFFHeader));

        if (qstrncmp(riff.descriptor.id, "RIFF", 4) != 0 ||
            qstrncmp(riff.type, "WAVE", 4) != 0) {
            source->disconnect(SIGNAL(readyRead()), this, SLOT(handleData()));
            emit invalidFormat();

            return;
        } else {
            state = QWaveDecoder::WaitingForFormatState;
        }
    }

    if (state == QWaveDecoder::WaitingForFormatState) {
        if (valid = findChunk("fmt ")) {
            chunk descriptor;
            source->peek(reinterpret_cast<char *>(&descriptor), sizeof(chunk));

            if (source->bytesAvailable() < qint64(descriptor.size + sizeof(chunk)))
                return;

            WAVEHeader wave;
            source->read(reinterpret_cast<char *>(&wave), sizeof(WAVEHeader));
            if (descriptor.size > sizeof(WAVEHeader))
                discardBytes(descriptor.size - sizeof(WAVEHeader));

            if (wave.audioFormat != 0 && wave.audioFormat != 1) {
                source->disconnect(SIGNAL(readyRead()), this, SLOT(handleData()));
                emit invalidFormat();

                return;
            } else {
                int bps = qFromLittleEndian<quint16>(wave.bitsPerSample);

                format.setCodec(QLatin1String("audio/pcm"));
                format.setSampleType(bps == 8 ? QAudioFormat::UnSignedInt : QAudioFormat::SignedInt);
                format.setByteOrder(QAudioFormat::LittleEndian);
                format.setFrequency(qFromLittleEndian<quint32>(wave.sampleRate));
                format.setSampleSize(bps);
                format.setChannels(qFromLittleEndian<quint16>(wave.numChannels));

                state = QWaveDecoder::WaitingForDataState;
            }
        }
    }

    if (state == QWaveDecoder::WaitingForDataState) {
        if (valid = findChunk("data")) {
            source->disconnect(SIGNAL(readyRead()), this, SLOT(handleData()));

            chunk descriptor;
            source->read(reinterpret_cast<char *>(&descriptor), sizeof(chunk));
            dataSize = descriptor.size;

            haveFormat = true;
            connect(source, SIGNAL(readyRead()), SIGNAL(readyRead()));
            emit formatKnown();

            return;
        }
    }

    if (source->atEnd() || !valid) {
        source->disconnect(SIGNAL(readyRead()), this, SLOT(handleData()));
        emit invalidFormat();

        return;
    }

}

bool QWaveDecoder::enoughDataAvailable()
{
    if (source->bytesAvailable() < qint64(sizeof(chunk)))
        return false;

    chunk descriptor;
    source->peek(reinterpret_cast<char *>(&descriptor), sizeof(chunk));

    if (source->bytesAvailable() < qint64(sizeof(chunk) + descriptor.size))
        return false;

    return true;
}

bool QWaveDecoder::findChunk(const char *chunkId)
{
    if (source->bytesAvailable() < qint64(sizeof(chunk)))
        return false;

    chunk descriptor;
    source->peek(reinterpret_cast<char *>(&descriptor), sizeof(chunk));

    if (qstrncmp(descriptor.id, chunkId, 4) == 0)
        return true;

    while (source->bytesAvailable() >= qint64(sizeof(chunk) + descriptor.size)) {
        discardBytes(sizeof(chunk) + descriptor.size);

        source->peek(reinterpret_cast<char *>(&descriptor), sizeof(chunk));

        if (qstrncmp(descriptor.id, chunkId, 4) == 0)
            return true;
    }

    return false;
}

void QWaveDecoder::discardBytes(qint64 numBytes)
{
    if (source->isSequential())
        source->read(numBytes);
    else
        source->seek(source->pos() + numBytes);
}

QT_END_NAMESPACE

#include "moc_qwavedecoder_p.cpp"
