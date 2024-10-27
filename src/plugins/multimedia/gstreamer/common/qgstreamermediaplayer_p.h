// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
#include <common/qgst_p.h>
#include <common/qgstpipeline_p.h>

#include <array>

QT_BEGIN_NAMESPACE

class QGstreamerMessage;
class QGstreamerAudioOutput;
class QGstreamerVideoOutput;

class QGstreamerMediaPlayer : public QObject,
                              public QPlatformMediaPlayer,
                              public QGstreamerBusMessageFilter,
                              public QGstreamerSyncMessageFilter
{
public:
    static QMaybe<QPlatformMediaPlayer *> create(QMediaPlayer *parent = nullptr);
    ~QGstreamerMediaPlayer();

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

private:
    QGstreamerMediaPlayer(QGstreamerVideoOutput *videoOutput, QMediaPlayer *parent);

    // track selection
    struct TrackSelector
    {
        TrackSelector(TrackType, QGstElement selector);

        QByteArrayView streamIdAtIndex(int index);

        void removeAllInputPads();
        QGstPad inputPad(int index);
        void setActiveInputPad(const QGstPad &input);
        QGstPad getSinkPadForDecoderPad(const QGstPad &decoderPad);

        void addAndConnectInputSelector(QGstPipeline &pipeline, QGstElement sink);
        void removeInputSelector(QGstPipeline &pipeline);

        void connectInputSelector(QGstPipeline &pipeline, QGstElement, bool inHandler);

        int selectedInputIndex = -1;

        QGstElement inputSelector;
        QGstElement connectedSink;
        QGstElement dummySink;

        TrackType type;
        QList<QByteArray> streams;
        std::map<QByteArray, QMediaMetaData, std::less<>> metaData;
        std::map<QByteArray, QGstPad, std::less<>> pads;
        std::map<QByteArray, QSize, std::less<>> nativeSize;
        std::map<QGstPad, QGstPad, std::less<>> connectionMap; // decoderPad->inputSelectorPad

        bool inputSelectorInPipeline = false;
    };
    using LockType = std::unique_lock<QMutex>;

    void connectTrackSelectorToOutput(TrackSelector &, bool inHandler = false);
    void disconnectTrackSelectorFromOutput(TrackSelector &, bool inHandler = false);
    void disconnectAllTrackSelectors();
    void setActivePad(TrackSelector &, const QGstPad &pad, bool flush = true);

    TrackSelector &trackSelector(TrackType type);
    void updateTrackMetadata(const QGstStreamCollectionHandle &);
    void prepareTrackMetadata(const QGstStreamCollectionHandle &, const LockType &);

    mutable QMutex trackSelectorsMutex;
    std::array<TrackSelector, NTrackTypes> trackSelectors;

    void stopOrEOS(bool eos);
    void detectPipelineIsSeekable(std::optional<std::chrono::nanoseconds> timeout);
    bool hasMedia() const;

    std::chrono::nanoseconds pipelinePosition() const;
    void updatePositionFromPipeline();
    std::optional<std::chrono::milliseconds> updateDurationFromPipeline();
    void updateBufferProgress(float);

    QGstElement getSinkElementForTrackType(TrackType);

    QMediaMetaData m_metaData;

    QUrl m_url;
    QIODevice *m_stream = nullptr;

    enum class ResourceErrorState : uint8_t {
        NoError,
        ErrorOccurred,
        ErrorReported,
    };

    bool m_prerolling = false; // pipeline did not enter PAUSED state or higher
    bool m_waitingForStreams = false; // GST_MESSAGE_STREAM_START not yet received
    bool m_playerReady = false; // player is ready when it's done prerolling & streams are selected

    bool m_initialBufferProgressSent = false;
    ResourceErrorState m_resourceErrorState = ResourceErrorState::NoError;
    float m_rate = 1.f;
    float m_bufferProgress = 0.f;
    std::chrono::milliseconds m_duration{};
    QTimer positionUpdateTimer;

    // Gst elements
    QGstPipeline playerPipeline;
    QGstElement decoder;
    QGstElement fakeAudioSink = QGstElement::createFromFactory("fakesink", "fakeAudioSink");

    QGstreamerAudioOutput *gstAudioOutput = nullptr;
    QGstreamerVideoOutput *gstVideoOutput = nullptr;

    // Message handler
    bool processBusMessage(const QGstreamerMessage &message) override;
    bool processBusMessageTags(const QGstreamerMessage &);
    bool processBusMessageDurationChanged(const QGstreamerMessage &);
    bool processBusMessageEOS(const QGstreamerMessage &);
    bool processBusMessageBuffering(const QGstreamerMessage &);
    bool processBusMessageStateChanged(const QGstreamerMessage &);
    bool processBusMessageError(const QGstreamerMessage &);
    bool processBusMessageWarning(const QGstreamerMessage &);
    bool processBusMessageInfo(const QGstreamerMessage &);
    bool processBusMessageSegmentStart(const QGstreamerMessage &);
    bool processBusMessageSegmentDone(const QGstreamerMessage &);
    bool processBusMessageElement(const QGstreamerMessage &);
    bool processBusMessageAsyncDone(const QGstreamerMessage &);
    bool processBusMessageLatency(const QGstreamerMessage &);
    bool processBusMessageStreamStart(const QGstreamerMessage &);
    bool processBusMessageStreamCollection(const QGstreamerMessage &);
    bool processBusMessageStreamsSelected(const QGstreamerMessage &);

    bool processSyncMessage(const QGstreamerMessage &) override;
    bool processSyncMessageStreamCollection(const QGstreamerMessage &);

    void finalizePreroll();

    // gstreamer signals
    void decoderPadAdded(const QGstElement &src, const QGstPad &pad);
    void decoderPadRemoved(const QGstElement &src, const QGstPad &pad);
    static void sourceSetupCallback(GstElement *uridecodebin, GstElement *source,
                                    QGstreamerMediaPlayer *);
    static gint s_decodebin3SelectStream(GstElement *decodebin, GstStreamCollection *collection,
                                         GstStream *stream, QGstreamerMediaPlayer *self);

    void disconnectDecoderHandlers();

    QGObjectHandlerScopedConnection padAdded;
    QGObjectHandlerScopedConnection padRemoved;
    QGObjectHandlerScopedConnection sourceSetup;
    QGObjectHandlerScopedConnection selectStream;

    // media state handling, stalled media detection
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    static constexpr auto stalledMediaDebouncePeriod = std::chrono::milliseconds{ 500 };
    QTimer m_stalledMediaNotifier;

    // pending state changes
    void applyPendingOperations(bool inTimer = false);
    using PlaybackState = QMediaPlayer::PlaybackState;

    std::optional<std::chrono::nanoseconds> m_pendingSeekPosition;
    std::optional<float> m_pendingRate;
    std::optional<PlaybackState> m_pendingState;

    QElapsedTimer m_seekTimer;
    QTimer m_seekRateLimiter;
};

QT_END_NAMESPACE

#endif
