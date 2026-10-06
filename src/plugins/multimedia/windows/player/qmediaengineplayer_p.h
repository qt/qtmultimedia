// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAENGINEPLAYER_P_H
#define QMEDIAENGINEPLAYER_P_H

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

#include <qwindowsmfdefs_p.h>

#include <QtMultimedia/qmediametadata.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qplatformmediaplayer_p.h>
#include <QtCore/qchronotimer.h>
#include <QtCore/qfuture.h>
#include <QtCore/qurl.h>
#include <QtCore/private/qcomptr_p.h>

#include <d3d11.h>

#include <array>
#include <chrono>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE

class QAudioOutput;
class QMediaEngineNotify;
class QMediaEngineTimedTextNotify;
class QPlatformVideoSink;

namespace QtMultimediaPrivate {
class QWindowsD3D11RhiTextureImporter;
}

class QMediaEnginePlayer : public QPlatformMediaPlayer
{
public:
    QMediaEnginePlayer(QMediaPlayer *player);
    ~QMediaEnginePlayer() override;

    // QPlatformMediaPlayer interface
    qint64 duration() const override;
    qint64 position() const override;
    void setPosition(qint64 position) override;
    float bufferProgress() const override;
    QMediaTimeRange availablePlaybackRanges() const override;
    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;
    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &media, QIODevice *stream) override;
    void play() override;
    void pause() override;
    void stop() override;
    bool streamPlaybackSupported() const override { return true; }
    PitchCompensationAvailability pitchCompensationAvailability() const override;
    QMediaMetaData metaData() const override;
    void setAudioOutput(QPlatformAudioOutput *output) override;
    void setVideoSink(QVideoSink *sink) override;
    void setActiveTrack(TrackType type, int index) override;
    int activeTrack(TrackType type) override;
    int trackCount(TrackType type) override;
    QMediaMetaData trackMetaData(TrackType type, int trackNumber) override;

private:
    // Initialization and lifecycle
    bool initD3D11();
    bool initMediaEngine();
    void releaseMediaEngine();

    // Event handling
    void handleMediaEngineEvent(DWORD event, DWORD_PTR param1, DWORD param2);

    // Video rendering
    void presentVideoFrame();
    void updateVideoFrameFormat();
    void updateNativeSize();

    // Audio
    void applyAudioOutput();

    // Metadata and tracks
    void updateMetaData();
    void updateTracks();
    void updateSubtitleTracks();
    void updateDuration();

    // Timer helpers
    void startPlaybackTimers();
    void stopPlaybackTimers();

    // Utilities
    qint64 currentPositionMs() const;
    QSize nativeVideoSize() const;

    // Context object for QObject::connect / callOnTimeout (the player
    // itself is not a QObject, so we use the position timer instead).
    QObject *context() { return &m_positionUpdateTimer; }

private:
    // Media Foundation
    ComPtr<IMFMediaEngine> m_mediaEngine;
    ComPtr<IMFMediaEngineEx> m_mediaEngineEx;
    ComPtr<IMFTimedText> m_timedText;
    ComPtr<QMediaEngineTimedTextNotify> m_timedTextNotify;
    ComPtr<QMediaEngineNotify> m_notify;

    // D3D11
    ComPtr<ID3D11Device> m_d3dDevice;
    ComPtr<IMFDXGIDeviceManager> m_dxgiManager;
    UINT m_resetToken = 0;

    // Video rendering
    ComPtr<ID3D11Texture2D> m_transferTexture;
    QSize m_videoSize;
    QVideoFrameFormat m_videoFormat;
    std::chrono::microseconds m_frameDuration{};
    std::shared_ptr<QtMultimediaPrivate::QWindowsD3D11RhiTextureImporter> m_textureImporter;
    int m_deviceLostRetries = 0;
    static constexpr int kMaxDeviceLostRetries = 3;

    // Outputs
    QPlatformVideoSink *m_videoSink = nullptr;
    QPlatformAudioOutput *m_audioOutput = nullptr;

    // Source
    QUrl m_url;
    QIODevice *m_stream = nullptr;

    // Metadata and tracks
    QMediaMetaData m_metaData;
    QFuture<void> m_metadataFuture;
    struct TrackInfo
    {
        struct Entry
        {
            QMediaMetaData metaData;
            DWORD streamIndex = 0;
        };
        std::vector<Entry> entries;
        int currentIndex = -1;
    };
    TrackTypeMap<TrackInfo> m_tracks;

    // Playback state
    std::chrono::milliseconds m_duration{};
    std::optional<std::chrono::milliseconds> m_seekTarget;
    qreal m_playbackRate = 1.0;
    bool m_pendingPlay = false;
    QChronoTimer m_positionUpdateTimer{ std::chrono::milliseconds{ 100 } };
    QChronoTimer m_videoRenderTimer{ std::chrono::milliseconds{ 16 } };
    QChronoTimer m_seekTimeoutTimer{ std::chrono::milliseconds{ 5000 } };
};

QT_END_NAMESPACE

#endif // QMEDIAENGINEPLAYER_P_H
