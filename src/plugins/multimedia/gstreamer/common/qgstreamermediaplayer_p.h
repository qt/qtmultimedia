// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERMEDIAPLAYER_P_H
#define QGSTREAMERMEDIAPLAYER_P_H

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

#include <QtMultimedia/private/qplatformmediaplayer_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qurl.h>

#include <common/qgst_bus_observer_p.h>
#include <common/qgst_discoverer_p.h>
#include <common/qgst_p.h>
#include <common/qgstpipeline_p.h>

#include <gst/play/gstplay.h>

#include <array>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;
class QGstreamerAudioOutput;
class QGstreamerVideoOutput;

class QGstreamerMediaPlayer : public QObject,
                              public QPlatformMediaPlayer,
                              public QGstreamerBusMessageFilter
{
    using QGstPlayHandle = QGstImpl::QGstHandleHelper<GstPlay>::SharedHandle;

public:
    static q23::expected<QPlatformMediaPlayer *, QString> create(QMediaPlayer *parent = nullptr);
    ~QGstreamerMediaPlayer() override;

    qint64 duration() const override;

    float bufferProgress() const override;

    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &, QIODevice *) override;

    bool streamPlaybackSupported() const override { return true; }

    void setAudioOutput(QPlatformAudioOutput *output) override;

    QMediaMetaData metaData() const override;

    void setVideoSink(QVideoSink *sink) override;

    int trackCount(TrackType) override;
    QMediaMetaData trackMetaData(TrackType /*type*/, int /*streamNumber*/) override;
    int activeTrack(TrackType) override;
    void setActiveTrack(TrackType, int /*streamNumber*/) override;

    void setPosition(qint64 pos) override;
    void setPosition(std::chrono::milliseconds pos);

    void play() override;
    void pause() override;
    void stop() override;

    const QGstPipeline &pipeline() const;

    bool canPlayQrc() const override;

    PitchCompensationAvailability pitchCompensationAvailability() const override;
    bool pitchCompensation() const override;

private:
    QGstreamerMediaPlayer(QGstreamerVideoOutput *videoOutput, QMediaPlayer *parent);

    static void sourceSetupCallback(GstElement *uridecodebin, GstElement *source,
                                    QGstreamerMediaPlayer *);
    bool hasMedia() const;
    bool hasValidMedia() const;

    void updatePositionFromPipeline();
    void updateBufferProgress(float);

    QUrl m_url;
    QIODevice *m_stream = nullptr;

    enum class ResourceErrorState : uint8_t {
        NoError,
        ErrorOccurred,
        ErrorReported,
    };

    ResourceErrorState m_resourceErrorState = ResourceErrorState::NoError;
    float m_bufferProgress = 0.f;
    std::chrono::milliseconds m_duration{};

    QGstreamerAudioOutput *gstAudioOutput = nullptr;
    QGstreamerVideoOutput *gstVideoOutput = nullptr;

    // // Message handler
    bool processBusMessage(const QGstreamerMessage &message) override;
    bool processBusMessageApplication(const QGstreamerMessage &message);

    // decoder connections
    void disconnectDecoderHandlers();
    QGObjectHandlerScopedConnection sourceSetup;

    bool discover(const QUrl &);

    // custom sources
    void decoderPadAddedCustomSource(const QGstElement &src, const QGstPad &pad);
    void decoderPadRemovedCustomSource(const QGstElement &src, const QGstPad &pad);
    bool isCustomSource() const { return m_url.scheme() == QStringLiteral(u"gstreamer-pipeline"); }
    void setMediaCustomSource(const QUrl &content);
    void cleanupCustomPipeline();
    QGstElement decoder;
    QGstPipeline customPipeline;
    QGObjectHandlerScopedConnection padAdded;
    QGObjectHandlerScopedConnection padRemoved;
    std::unique_ptr<QTimer> positionUpdateTimer;
    std::array<QGstElement, 3> customPipelineSinks;
    std::array<QGstPad, 3> customPipelinePads;

    // play
    QGstPlayHandle m_gstPlay;
    QGstPipeline m_playbin;
    QGstBusObserver m_gstPlayBus;

    // metadata
    QMediaMetaData m_metaData;
    std::array<std::vector<QMediaMetaData>, 3> m_trackMetaData;
    std::array<std::vector<QByteArray>, 3> m_trackIDs;
    std::array<int, 3> m_activeTrack{};
    QList<QSize> m_nativeSize;

    void resetStateForEmptyOrInvalidMedia();
    void updateNativeSizeOnVideoOutput();

    void seekToCurrentPosition();

    std::optional<std::chrono::nanoseconds> m_pendingSeek;

    int stateChangeToSkip = 0;

    void updateVideoTrackEnabled();
    void updateAudioTrackEnabled();
    void updateSubtitleTrackEnabled();
};

QT_END_NAMESPACE

#endif
