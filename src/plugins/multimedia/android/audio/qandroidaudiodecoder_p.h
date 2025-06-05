// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDAUDIODECODER_P_H
#define QANDROIDAUDIODECODER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
#include "private/qplatformaudiodecoder_p.h"

#include <QtCore/qurl.h>
#include <QThread>

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"
#include "media/NdkMediaFormat.h"
#include "media/NdkMediaError.h"


QT_USE_NAMESPACE

class Decoder : public QObject
{
    Q_OBJECT
public:
    Decoder();
    ~Decoder();

public slots:
    void setSource(const QUrl &source);
    void doDecode();
    void stop();

signals:
    void positionChanged(const QAudioBuffer &buffer, qint64 position);
    void durationChanged(const qint64 duration);
    void error(const QAudioDecoder::Error error, const QString &errorString);
    void finished();
    void decodingChanged(bool decoding);

private:
    void createDecoder();

    AMediaCodec *m_codec = nullptr;
    AMediaExtractor *m_extractor = nullptr;
    AMediaFormat *m_format = nullptr;

    QAudioFormat m_outputFormat;
    QString m_formatError;
    bool m_inputEOS;
};


class QAndroidAudioDecoder : public QPlatformAudioDecoder
{
    Q_OBJECT
public:
    QAndroidAudioDecoder(QAudioDecoder *parent);
        virtual ~QAndroidAudioDecoder();

    QUrl source() const override { return m_source; }
    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override { return m_device; }
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override { return {}; }
    void setAudioFormat(const QAudioFormat &/*format*/) override {}

    QAudioBuffer read() override;
    bool bufferAvailable() const override;

    qint64 position() const override;
    qint64 duration() const override;

signals:
    void setSourceUrl(const QUrl &source);

private slots:
    void positionChanged(QAudioBuffer audioBuffer, qint64 position);
    void durationChanged(qint64 duration);
    void error(const QAudioDecoder::Error error, const QString &errorString);
    void readDevice();
    void finished();

private:
    bool requestPermissions();
    bool createTempFile();
    void decode();

    QIODevice *m_device = nullptr;
    Decoder *m_decoder = nullptr;

    QList<std::pair<QAudioBuffer, int>> m_audioBuffer;
    QUrl m_source;

    qint64 m_position = -1;
    qint64 m_duration = -1;

    QByteArray m_deviceBuffer;

    QThread *m_threadDecoder = nullptr;
};

QT_END_NAMESPACE

#endif // QANDROIDAUDIODECODER_P_H
