// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERAUDIODECODERCONTROL_H
#define QGSTREAMERAUDIODECODERCONTROL_H

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

#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qplatformaudiodecoder_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qaudiodecoder.h>
#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qurl.h>
#include <QtCore/private/qexpected_p.h>

#include <common/qgst_p.h>
#include <common/qgst_bus_observer_p.h>
#include <common/qgstpipeline_p.h>

#include <gst/app/gstappsink.h>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;

class QGstreamerAudioDecoder final : public QPlatformAudioDecoder, public QGstreamerBusMessageFilter
{
    Q_OBJECT

public:
    static q23::expected<QPlatformAudioDecoder *, QString> create(QAudioDecoder *parent);
    ~QGstreamerAudioDecoder() override;

    QUrl source() const override;
    void setSource(const QUrl &fileName) override;

    QIODevice *sourceDevice() const override;
    void setSourceDevice(QIODevice *device) override;

    void start() override;
    void stop() override;

    QAudioFormat audioFormat() const override;
    void setAudioFormat(const QAudioFormat &format) override;

    QAudioBuffer read() override;

    qint64 position() const override;
    qint64 duration() const override;

    // GStreamerBusMessageFilter interface
    bool processBusMessage(const QGstreamerMessage &message) override;

    bool canReadQrc() const override;

private slots:
    void updateDuration();

private:
    explicit QGstreamerAudioDecoder(QAudioDecoder *parent);

    static GstFlowReturn new_sample(GstAppSink *sink, gpointer user_data);
    GstFlowReturn newSample(GstAppSink *sink);

    void setAudioFlags(bool wantNativeAudio);
    void addAppSink();
    void removeAppSink();

    void processInvalidMedia(QAudioDecoder::Error errorCode, const QString &errorString);
    static std::chrono::nanoseconds getPositionFromBuffer(GstBuffer *buffer);

    bool processBusMessageError(const QGstreamerMessage &);
    bool processBusMessageDuration(const QGstreamerMessage &);
    bool processBusMessageWarning(const QGstreamerMessage &);
    bool processBusMessageInfo(const QGstreamerMessage &);
    bool processBusMessageEOS(const QGstreamerMessage &);
    bool processBusMessageStateChanged(const QGstreamerMessage &);
    bool processBusMessageStreamsSelected(const QGstreamerMessage &);

    QGstPipeline m_playbin;
    QGstBin m_outputBin;
    QGstElement m_audioConvert;
    QGstAppSink m_appSink;

    QUrl mSource;
    QIODevice *mDevice = nullptr;
    QAudioFormat mFormat;

    int m_buffersAvailable = 0;

    static constexpr auto invalidDuration = std::chrono::milliseconds{ -1 };
    static constexpr auto invalidPosition = std::chrono::milliseconds{ -1 };
    std::chrono::milliseconds m_position{ invalidPosition };
    std::chrono::milliseconds m_duration{ invalidDuration };

    int m_durationQueries = 0;

    qint32 m_currentSessionId{};

    QGObjectHandlerScopedConnection m_deepNotifySourceConnection;
};

QT_END_NAMESPACE

#endif // QGSTREAMERPLAYERSESSION_H
