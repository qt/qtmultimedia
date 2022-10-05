// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MOCKAUDIODECODERCONTROL_H
#define MOCKAUDIODECODERCONTROL_H

#include "private/qplatformaudiodecoder_p.h"

#include <QtCore/qpair.h>
#include <QtCore/qurl.h>

#include "qaudiobuffer.h"
#include <QTimer>
#include <QIODevice>

#define MOCK_DECODER_MAX_BUFFERS 10

QT_BEGIN_NAMESPACE

class QMockAudioDecoder : public QPlatformAudioDecoder
{
    Q_OBJECT

public:
    QMockAudioDecoder(QAudioDecoder *parent = 0)
        : QPlatformAudioDecoder(parent)
        , mDevice(0)
        , mPosition(-1)
        , mSerial(0)
    {
        mFormat.setChannelCount(1);
        mFormat.setSampleFormat(QAudioFormat::UInt8);
        mFormat.setSampleRate(1000);
    }

    QUrl source() const override
    {
        return mSource;
    }

    void setSource(const QUrl &fileName) override
    {
        mSource = fileName;
        mDevice = 0;
        stop();
    }

    QIODevice* sourceDevice() const override
    {
        return mDevice;
    }

    void setSourceDevice(QIODevice *device) override
    {
        mDevice = device;
        mSource.clear();
        stop();
    }

    QAudioFormat audioFormat() const override
    {
        return mFormat;
    }

    void setAudioFormat(const QAudioFormat &format) override
    {
        if (mFormat != format) {
            mFormat = format;
            emit formatChanged(mFormat);
        }
    }

    // When decoding we decode to first buffer, then second buffer
    // we then stop until the first is read again and so on, for
    // 5 buffers
    void start() override
    {
        if (!isDecoding()) {
            if (!mSource.isEmpty()) {
                setIsDecoding(true);
                emit durationChanged(duration());

                QTimer::singleShot(50, this, SLOT(pretendDecode()));
            } else {
                emit error(QAudioDecoder::ResourceError, "No source set");
            }
        }
    }

    void stop() override
    {
        if (isDecoding()) {
            mSerial = 0;
            mPosition = 0;
            mBuffers.clear();
            setIsDecoding(false);
            emit bufferAvailableChanged(false);
        }
    }

    QAudioBuffer read() override
    {
        QAudioBuffer a;
        if (mBuffers.size() > 0) {
            a = mBuffers.takeFirst();
            mPosition = a.startTime() / 1000;
            positionChanged(mPosition);

            if (mBuffers.isEmpty())
                emit bufferAvailableChanged(false);

            if (mBuffers.isEmpty() && mSerial >= MOCK_DECODER_MAX_BUFFERS) {
                emit finished();
            } else
                QTimer::singleShot(50, this, SLOT(pretendDecode()));
        }

        return a;
    }

    bool bufferAvailable() const override
    {
        return mBuffers.size() > 0;
    }

    qint64 position() const override
    {
        return mPosition;
    }

    qint64 duration() const override
    {
        return (sizeof(mSerial) * MOCK_DECODER_MAX_BUFFERS * qint64(1000)) / (mFormat.sampleRate() * mFormat.channelCount());
    }

private slots:
    void pretendDecode()
    {
        // Check if we've reached end of stream
        if (mSerial >= MOCK_DECODER_MAX_BUFFERS)
            return;

        // We just keep the length of mBuffers to 3 or less.
        if (mBuffers.size() < 3) {
            QByteArray b(sizeof(mSerial), 0);
            memcpy(b.data(), &mSerial, sizeof(mSerial));
            qint64 position = (sizeof(mSerial) * mSerial * qint64(1000000)) / (mFormat.sampleRate() * mFormat.channelCount());
            mSerial++;
            mBuffers.push_back(QAudioBuffer(b, mFormat, position));
            emit bufferReady();
            if (mBuffers.size() == 1)
                emit bufferAvailableChanged(true);
        }
    }

public:
    QUrl mSource;
    QIODevice *mDevice;
    QAudioFormat mFormat;
    qint64 mPosition;

    int mSerial;
    QList<QAudioBuffer> mBuffers;
};

QT_END_NAMESPACE

#endif  // QAUDIODECODERCONTROL_H
