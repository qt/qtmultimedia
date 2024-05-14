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

#include <common/qgstpipeline_p.h>
#include <common/qgst_p.h>

#if QT_CONFIG(gstreamer_app)
#  include <common/qgstappsource_p.h>
#endif

#include <gst/app/gstappsink.h>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;

class QGstreamerAudioDecoder final : public QPlatformAudioDecoder, public QGstreamerBusMessageFilter
{
    Q_OBJECT

public:
    static QMaybe<QPlatformAudioDecoder *> create(QAudioDecoder *parent);
    virtual ~QGstreamerAudioDecoder();

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

private slots:
    void updateDuration();

private:
    QGstreamerAudioDecoder(QGstPipeline playbin, QGstElement audioconvert, QAudioDecoder *parent);

#if QT_CONFIG(gstreamer_app)
    static GstFlowReturn new_sample(GstAppSink *sink, gpointer user_data);
    GstFlowReturn newSample(GstAppSink *sink);

    static void configureAppSrcElement(GObject *, GObject *, GParamSpec *,
                                       QGstreamerAudioDecoder *_this);
#endif

    void setAudioFlags(bool wantNativeAudio);
    void addAppSink();
    void removeAppSink();

    bool handlePlaybinMessage(const QGstreamerMessage &);

    void processInvalidMedia(QAudioDecoder::Error errorCode, const QString &errorString);
    static std::chrono::nanoseconds getPositionFromBuffer(GstBuffer *buffer);

    QGstPipeline m_playbin;
    QGstBin m_outputBin;
    QGstElement m_audioConvert;
    QGstAppSink m_appSink;
    QGstAppSource *m_appSrc = nullptr;

    QUrl mSource;
    QIODevice *mDevice = nullptr;
    QAudioFormat mFormat;

    int m_buffersAvailable = 0;

    static constexpr auto invalidDuration = std::chrono::milliseconds::max();
    static constexpr auto invalidPosition = std::chrono::milliseconds::max();
    std::chrono::milliseconds m_position{ invalidPosition };
    std::chrono::milliseconds m_duration{ invalidDuration };

    int m_durationQueries = 0;

    qint32 m_currentSessionId{};

    QGObjectHandlerScopedConnection m_deepNotifySourceConnection;
};

QT_END_NAMESPACE

#endif // QGSTREAMERPLAYERSESSION_H
