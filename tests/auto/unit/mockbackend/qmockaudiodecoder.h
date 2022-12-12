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
    QMockAudioDecoder(QAudioDecoder *parent = nullptr);

    QUrl source() const override;

    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override;

    void setSourceDevice(QIODevice *device) override;

    QAudioFormat audioFormat() const override;

    void setAudioFormat(const QAudioFormat &format) override;

    // When decoding we decode to first buffer, then second buffer
    // we then stop until the first is read again and so on, for
    // 5 buffers
    void start() override;

    void stop() override;

    QAudioBuffer read() override;

    bool bufferAvailable() const override;

    qint64 position() const override;

    qint64 duration() const override;

private slots:
    void pretendDecode();

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
