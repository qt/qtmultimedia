// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmockaudiodecoder.h"

QT_BEGIN_NAMESPACE

QMockAudioDecoder::QMockAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent), mDevice(0), mPosition(-1), mSerial(0)
{
    mFormat.setChannelCount(1);
    mFormat.setSampleFormat(QAudioFormat::UInt8);
    mFormat.setSampleRate(1000);
}

QUrl QMockAudioDecoder::source() const
{
    return mSource;
}

void QMockAudioDecoder::setSource(const QUrl &fileName)
{
    mSource = fileName;
    mDevice = 0;
    stop();
}

QIODevice *QMockAudioDecoder::sourceDevice() const
{
    return mDevice;
}

void QMockAudioDecoder::setSourceDevice(QIODevice *device)
{
    mDevice = device;
    mSource.clear();
    stop();
}

QAudioFormat QMockAudioDecoder::audioFormat() const
{
    return mFormat;
}

void QMockAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (mFormat != format) {
        mFormat = format;
        formatChanged(mFormat);
    }
}

// When decoding we decode to first buffer, then second buffer
// we then stop until the first is read again and so on, for
// 5 buffers
void QMockAudioDecoder::start()
{
    if (!isDecoding()) {
        if (!mSource.isEmpty()) {
            setIsDecoding(true);
            durationChanged(duration());

            QTimer::singleShot(50, this, &QMockAudioDecoder::pretendDecode);
        } else {
            error(QAudioDecoder::ResourceError, "No source set");
        }
    }
}

void QMockAudioDecoder::stop()
{
    if (isDecoding()) {
        mSerial = 0;
        mPosition = 0;
        mBuffers.clear();
        setIsDecoding(false);
        bufferAvailableChanged(false);
    }
}

QAudioBuffer QMockAudioDecoder::read()
{
    QAudioBuffer a;
    if (mBuffers.size() > 0) {
        a = mBuffers.takeFirst();
        mPosition = a.startTime() / 1000;
        positionChanged(mPosition);

        if (mBuffers.isEmpty())
            bufferAvailableChanged(false);

        if (mBuffers.isEmpty() && mSerial >= MOCK_DECODER_MAX_BUFFERS) {
            finished();
        } else
            QTimer::singleShot(50, this, &QMockAudioDecoder::pretendDecode);
    }

    return a;
}

bool QMockAudioDecoder::bufferAvailable() const
{
    return mBuffers.size() > 0;
}

qint64 QMockAudioDecoder::position() const
{
    return mPosition;
}

qint64 QMockAudioDecoder::duration() const
{
    return (sizeof(mSerial) * MOCK_DECODER_MAX_BUFFERS * qint64(1000))
            / (mFormat.sampleRate() * mFormat.channelCount());
}

void QMockAudioDecoder::pretendDecode()
{
    // Check if we've reached end of stream
    if (mSerial >= MOCK_DECODER_MAX_BUFFERS)
        return;

    // We just keep the length of mBuffers to 3 or less.
    if (mBuffers.size() < 3) {
        QByteArray b(sizeof(mSerial), 0);
        memcpy(b.data(), &mSerial, sizeof(mSerial));
        qint64 position = (sizeof(mSerial) * mSerial * qint64(1000000))
                / (mFormat.sampleRate() * mFormat.channelCount());
        mSerial++;
        mBuffers.push_back(QAudioBuffer(b, mFormat, position));
        bufferReady();
        if (mBuffers.size() == 1)
            bufferAvailableChanged(true);
    }
}

QT_END_NAMESPACE

#include "moc_qmockaudiodecoder.cpp"
