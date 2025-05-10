// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <QWaitCondition>
#include <QMutex>
#include <QQueue>

#include "private/qplatformaudiodecoder_p.h"
#include "qaudiodecoder.h"

#include <dispatch/dispatch.h>

Q_FORWARD_DECLARE_OBJC_CLASS(AVURLAsset);
Q_FORWARD_DECLARE_OBJC_CLASS(AVAssetReader);
Q_FORWARD_DECLARE_OBJC_CLASS(AVAssetReaderTrackOutput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVFResourceReaderDelegate);
Q_FORWARD_DECLARE_OBJC_CLASS(AVAssetTrack);
Q_FORWARD_DECLARE_OBJC_CLASS(NSError);

QT_BEGIN_NAMESPACE

class AVFAudioDecoder final : public QPlatformAudioDecoder
{
    Q_OBJECT

    struct DecodingContext;

public:
    AVFAudioDecoder(QAudioDecoder *parent);
    ~AVFAudioDecoder() override;

    QUrl source() const override;
    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override;
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override;
    void setAudioFormat(const QAudioFormat &format) override;

    QAudioBuffer read() override;

private:
    void handleNewAudioBuffer(QAudioBuffer);
    void startReading();

    void processInvalidMedia(QAudioDecoder::Error errorCode, const QString& errorString);
    void initAssetReader();
    void onFinished();

    void waitUntilBuffersCounterLessMax();

    void decBuffersCounter(uint val);

    template<typename F>
    void invokeWithDecodingContext(std::weak_ptr<DecodingContext> weakContext, F &&f);
    Q_INVOKABLE void initAssetReaderImpl(AVAssetTrack *track, NSError *error);

private:
    QUrl m_source;
    QIODevice *m_device = nullptr;
    QAudioFormat m_format;

    // Use a separate counter instead of buffers queue size in order to
    // ensure atomic access and also make mutex locking shorter
    std::atomic<int> m_buffersCounter = 0;
    QQueue<QAudioBuffer> m_cachedBuffers;

    AVURLAsset *m_asset = nullptr;

    AVFResourceReaderDelegate *m_readerDelegate = nullptr;
    dispatch_queue_t m_readingQueue;
    dispatch_queue_t m_decodingQueue;

    std::shared_ptr<DecodingContext> m_decodingContext;
    QMutex m_buffersCounterMutex;
    QWaitCondition m_buffersCounterCondition;
};

QT_END_NAMESPACE

#endif // AVFAUDIODECODER_H
