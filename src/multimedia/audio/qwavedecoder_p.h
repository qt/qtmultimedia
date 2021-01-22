/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef WAVEDECODER_H
#define WAVEDECODER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qiodevice.h>
#include <qaudioformat.h>


QT_BEGIN_NAMESPACE



class QWaveDecoder : public QIODevice
{
    Q_OBJECT

public:
    explicit QWaveDecoder(QIODevice *source, QObject *parent = nullptr);
    ~QWaveDecoder();

    QAudioFormat audioFormat() const;
    int duration() const;

    qint64 size() const override;
    bool isSequential() const override;
    qint64 bytesAvailable() const override;

Q_SIGNALS:
    void formatKnown();
    void parsingError();

private Q_SLOTS:
    void handleData();

private:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

    bool enoughDataAvailable();
    bool findChunk(const char *chunkId);
    void discardBytes(qint64 numBytes);
    void parsingFailed();

    enum State {
        InitialState,
        WaitingForFormatState,
        WaitingForDataState
    };

    struct chunk
    {
        char        id[4];
        quint32     size;
    };
    bool peekChunk(chunk* pChunk, bool handleEndianness = true);

    struct RIFFHeader
    {
        chunk       descriptor;
        char        type[4];
    };
    struct WAVEHeader
    {
        chunk       descriptor;
        quint16     audioFormat;
        quint16     numChannels;
        quint32     sampleRate;
        quint32     byteRate;
        quint16     blockAlign;
        quint16     bitsPerSample;
    };

    bool haveFormat;
    qint64 dataSize;
    QAudioFormat format;
    QIODevice *source;
    State state;
    quint32 junkToSkip;
    bool bigEndian;
};

QT_END_NAMESPACE

#endif // WAVEDECODER_H
