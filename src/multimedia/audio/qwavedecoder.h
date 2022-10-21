/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WAVEDECODER_H
#define WAVEDECODER_H

#include <QtCore/qiodevice.h>
#include <QtMultimedia/qaudioformat.h>


QT_BEGIN_NAMESPACE



class Q_MULTIMEDIA_EXPORT QWaveDecoder : public QIODevice
{
    Q_OBJECT

public:
    explicit QWaveDecoder(QIODevice *device, QObject *parent = nullptr);
    explicit QWaveDecoder(QIODevice *device, const QAudioFormat &format,
                        QObject *parent = nullptr);
    ~QWaveDecoder();

    QAudioFormat audioFormat() const;
    QIODevice* getDevice();
    int duration() const;
    static qint64 headerLength();

    bool open(QIODevice::OpenMode mode) override;
    void close() override;
    bool seek(qint64 pos) override;
    qint64 pos() const override;
    void setIODevice(QIODevice *device);
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

    bool writeHeader();
    bool writeDataLength();
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

    struct DATAHeader
    {
        chunk       descriptor;
    };

    struct CombinedHeader
    {
        RIFFHeader  riff;
        WAVEHeader  wave;
        DATAHeader  data;
    };
    static const int HeaderLength = sizeof(CombinedHeader);

    bool haveFormat = false;
    bool haveHeader = false;
    qint64 dataSize = 0;
    QIODevice *device = nullptr;
    QAudioFormat format;
    State state = InitialState;
    quint32 junkToSkip = 0;
    bool bigEndian = false;
    bool byteSwap = false;
    int bps = 0;
};

QT_END_NAMESPACE

#endif // WAVEDECODER_H
