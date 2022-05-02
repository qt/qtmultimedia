/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef AVFAUDIODECODER_H
#define AVFAUDIODECODER_H

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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QObject>
#include <QtCore/qurl.h>

#include "private/qplatformaudiodecoder_p.h"
#include "qaudiodecoder.h"

#include <dispatch/dispatch.h>

Q_FORWARD_DECLARE_OBJC_CLASS(AVURLAsset);
Q_FORWARD_DECLARE_OBJC_CLASS(AVAssetReader);
Q_FORWARD_DECLARE_OBJC_CLASS(AVAssetReaderTrackOutput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVFResourceReaderDelegate);

QT_BEGIN_NAMESPACE

class AVFAudioDecoder : public QPlatformAudioDecoder
{
    Q_OBJECT

public:
    AVFAudioDecoder(QAudioDecoder *parent);
    virtual ~AVFAudioDecoder();

    QUrl source() const override;
    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override;
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override;
    void setAudioFormat(const QAudioFormat &format) override;

    QAudioBuffer read() override;
    bool bufferAvailable() const override;

    qint64 position() const override;
    qint64 duration() const override;

private slots:
    void handleNewAudioBuffer(QAudioBuffer);
    void startReading();

signals:
    void newAudioBuffer(QAudioBuffer);
    void readyToRead();

private:
    void processInvalidMedia(QAudioDecoder::Error errorCode, const QString& errorString);
    void initAssetReader();

    QUrl m_source;
    QIODevice *m_device = nullptr;
    QAudioFormat m_format;

    int m_buffersAvailable = 0;
    QList<QAudioBuffer> m_cachedBuffers;

    qint64 m_position = -1;
    qint64 m_duration = -1;

    bool m_loadingSource = false;

    AVURLAsset *m_asset = nullptr;
    AVAssetReader *m_reader = nullptr;
    AVAssetReaderTrackOutput *m_readerOutput = nullptr;
    AVFResourceReaderDelegate *m_readerDelegate = nullptr;
    dispatch_queue_t m_readingQueue;
    dispatch_queue_t m_decodingQueue;
};

QT_END_NAMESPACE

#endif // AVFAUDIODECODER_H
