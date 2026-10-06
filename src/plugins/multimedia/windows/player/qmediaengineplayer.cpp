// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaengineplayer_p.h"

#include <common/mfmetadata_p.h>
#include <mfstream_p.h>
#include <player/qmediaenginenotify_p.h>
#include <player/qmediaenginetimedtextnotify_p.h>
#include <player/qmediaenginevideobuffer_p.h>

#include <QtMultimedia/qaudiooutput.h>
#include <QtMultimedia/qvideosink.h>
#include <QtMultimedia/private/qcomtaskresource_p.h>
#include <QtMultimedia/private/qplatformaudiooutput_p.h>
#include <QtMultimedia/private/qplatformvideosink_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtMultimedia/private/qwindows_scopedpropvariant_p.h>
#include <QtMultimedia/private/qwindowsd3d11rhitextureimporter_p.h>
#include <QtMultimedia/private/qwmf_support_p.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QtCore/qfile.h>
#include <QtCore/qlocale.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qtimer.h>
#include <QtCore/private/qbstr_p.h>
#include <QtCore/private/qsystemerror_p.h>

#include <dxgi.h>
#include <mfapi.h>
#include <mfidl.h>

Q_LOGGING_CATEGORY(lcMediaEngine, "qt.multimedia.mediaengine")

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;
using QtMultimediaPrivate::ScopedPropVariant;
using reference_time = QWMF::reference_time;

namespace {

QVariant languageVariant(const QString &langStr)
{
    const QLocale::Language lang = QLocale::codeToLanguage(langStr, QLocale::AnyLanguageCode);
    return lang != QLocale::AnyLanguage ? QVariant::fromValue(lang) : QVariant(langStr);
}

QMediaPlayer::Error mapMediaEngineError(MF_MEDIA_ENGINE_ERR err)
{
    switch (err) {
    case MF_MEDIA_ENGINE_ERR_NOERROR:
        return QMediaPlayer::NoError;
    case MF_MEDIA_ENGINE_ERR_ABORTED:
        return QMediaPlayer::ResourceError;
    case MF_MEDIA_ENGINE_ERR_NETWORK:
        return QMediaPlayer::NetworkError;
    case MF_MEDIA_ENGINE_ERR_DECODE:
        return QMediaPlayer::FormatError;
    case MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED:
        return QMediaPlayer::ResourceError;
    case MF_MEDIA_ENGINE_ERR_ENCRYPTED:
        return QMediaPlayer::AccessDeniedError;
    default:
        return QMediaPlayer::ResourceError;
    }
}

std::chrono::microseconds videoFrameDurationUs(IMFMediaEngineEx *engine)
{
    if (!engine)
        return {};

    DWORD streamCount = 0;
    if (FAILED(engine->GetNumberOfStreams(&streamCount)))
        return {};

    for (DWORD i = 0; i < streamCount; ++i) {
        ScopedPropVariant majorType;
        if (FAILED(engine->GetStreamAttribute(i, MF_MT_MAJOR_TYPE, majorType.get()))
            || majorType->vt != VT_CLSID || *majorType->puuid != MFMediaType_Video) {
            continue;
        }

        ScopedPropVariant rate;
        if (FAILED(engine->GetStreamAttribute(i, MF_MT_FRAME_RATE, rate.get()))
            || rate->vt != VT_UI8) {
            return {};
        }

        UINT64 avgTimePerFrame = 0;
        if (SUCCEEDED(MFFrameRateToAverageTimePerFrame(
                    HI32(rate->uhVal.QuadPart), LO32(rate->uhVal.QuadPart), &avgTimePerFrame)))
            return std::chrono::round<std::chrono::microseconds>(reference_time(avgTimePerFrame));

        return {};
    }
    return {};
}

} // namespace

bool QMediaEnginePlayer::initD3D11()
{
    m_textureImporter = std::make_shared<QtMultimediaPrivate::QWindowsD3D11RhiTextureImporter>();

    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    UINT flags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT | D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevels,
                                   ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &m_d3dDevice,
                                   nullptr, nullptr);

    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "D3D11CreateDevice:" << QSystemError::windowsComString(hr);
        return false;
    }

    ComPtr<ID3D10Multithread> multithread;
    if (SUCCEEDED(m_d3dDevice.As(&multithread)))
        multithread->SetMultithreadProtected(TRUE);

    return true;
}

bool QMediaEnginePlayer::initMediaEngine()
{
    HRESULT hr = MFCreateDXGIDeviceManager(&m_resetToken, &m_dxgiManager);
    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "MFCreateDXGIDeviceManager:"
                                 << QSystemError::windowsComString(hr);
        return false;
    }

    hr = m_dxgiManager->ResetDevice(m_d3dDevice.Get(), m_resetToken);
    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "ResetDevice:" << QSystemError::windowsComString(hr);
        return false;
    }

    m_notify = makeComObject<QMediaEngineNotify>();
    // Queued: some MF events fire synchronously (e.g. PAUSE from Pause()),
    // must defer to avoid re-entering handleMediaEngineEvent mid-operation.
    QObject::connect(m_notify.Get(), &QMediaEngineNotify::eventNotified, context(),
                     [this](DWORD event, DWORD_PTR param1, DWORD param2) {
        handleMediaEngineEvent(event, param1, param2);
    }, Qt::QueuedConnection);

    ComPtr<IMFAttributes> attrs;
    hr = MFCreateAttributes(&attrs, 4);
    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "MFCreateAttributes:" << QSystemError::windowsComString(hr);
        return false;
    }

    attrs->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, m_notify.Get());
    attrs->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, m_dxgiManager.Get());
    attrs->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM);

    ComPtr<IMFMediaEngineClassFactory> factory;
    hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "CoCreateInstance(MFMediaEngineClassFactory):"
                                 << QSystemError::windowsComString(hr);
        return false;
    }

    hr = factory->CreateInstance(0, attrs.Get(), &m_mediaEngine);
    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "IMFMediaEngine::CreateInstance:"
                                 << QSystemError::windowsComString(hr);
        return false;
    }

    hr = m_mediaEngine.As(&m_mediaEngineEx);
    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "IMFMediaEngineEx:" << QSystemError::windowsComString(hr);
        return false;
    }

    // Enable inband timed text (embedded subtitles) discovery.
    // IMFTimedText is obtained via MFGetService with MF_MEDIA_ENGINE_TIMEDTEXT.
    ComPtr<IMFGetService> getService;
    if (SUCCEEDED(m_mediaEngine.As(&getService))) {
        getService->GetService(MF_MEDIA_ENGINE_TIMEDTEXT, IID_PPV_ARGS(&m_timedText));
    }

    if (m_timedText) {
        m_timedText->SetInBandEnabled(TRUE);

        m_timedTextNotify = makeComObject<QMediaEngineTimedTextNotify>();

        QObject::connect(m_timedTextNotify.Get(), &QMediaEngineTimedTextNotify::trackAdded,
                         context(), [this] {
            updateSubtitleTracks();
        });

        QObject::connect(m_timedTextNotify.Get(), &QMediaEngineTimedTextNotify::subtitleTextChanged,
                         context(), [this](const QString &text) {
            if (m_videoSink)
                m_videoSink->setSubtitleText(text);
        });

        m_timedText->RegisterNotifications(m_timedTextNotify.Get());
    }

    // Apply cached playback rate to the newly created engine.
    if (!qFuzzyCompare(m_playbackRate, 1.0)) {
        m_mediaEngine->SetDefaultPlaybackRate(m_playbackRate);
        m_mediaEngine->SetPlaybackRate(m_playbackRate);
    }

    return true;
}

void QMediaEnginePlayer::releaseMediaEngine()
{
    if (m_mediaEngine) {
        m_mediaEngine->Shutdown();
        m_mediaEngine.Reset();
    }
    m_mediaEngineEx.Reset();
    m_timedText.Reset();
    m_timedTextNotify.Reset();
    m_notify.Reset();
    m_dxgiManager.Reset();
    m_resetToken = 0;
}

void QMediaEnginePlayer::startPlaybackTimers()
{
    m_positionUpdateTimer.start();
    m_videoRenderTimer.start();
}

void QMediaEnginePlayer::stopPlaybackTimers()
{
    m_positionUpdateTimer.stop();
    m_videoRenderTimer.stop();
}

QMediaEnginePlayer::QMediaEnginePlayer(QMediaPlayer *player) : QPlatformMediaPlayer(player)
{
    if (!initD3D11())
        error(QMediaPlayer::ResourceError, u"Failed to initialize D3D11"_s);

    m_positionUpdateTimer.callOnTimeout(context(), [this] {
        if (m_mediaEngine && !m_seekTarget)
            positionChanged(currentPositionMs());
    });

    m_videoRenderTimer.callOnTimeout(context(), [this] {
        presentVideoFrame();
    });

    m_seekTimeoutTimer.setSingleShot(true);
    m_seekTimeoutTimer.callOnTimeout(context(), [this] {
        if (m_seekTarget) {
            qCWarning(lcMediaEngine) << "Seek timeout — SEEKED event not received, resetting";
            m_seekTarget.reset();
        }
    });
}

QMediaEnginePlayer::~QMediaEnginePlayer()
{
    stopPlaybackTimers();
    m_seekTimeoutTimer.stop();

    m_metadataFuture.cancel();

    releaseMediaEngine();
}

void QMediaEnginePlayer::handleMediaEngineEvent(DWORD event, DWORD_PTR param1, DWORD param2)
{
    Q_UNUSED(param2);

    if (!m_mediaEngine)
        return;

    switch (event) {
    case MF_MEDIA_ENGINE_EVENT_LOADEDMETADATA:
        qCDebug(lcMediaEngine) << "LOADEDMETADATA";
        updateMetaData();
        updateTracks();
        updateDuration();
        break;

    case MF_MEDIA_ENGINE_EVENT_LOADEDDATA: {
        qCDebug(lcMediaEngine) << "LOADEDDATA";
        audioAvailableChanged(m_mediaEngine->HasAudio());
        videoAvailableChanged(m_mediaEngine->HasVideo());
        updateVideoFrameFormat();
        updateNativeSize();
        seekableChanged(!m_stream || !m_stream->isSequential());
        mediaStatusChanged(QMediaPlayer::LoadedMedia);

        if (const float bp = bufferProgress(); bp > 0)
            bufferProgressChanged(bp);

        if (m_pendingPlay) {
            m_pendingPlay = false;
            play();
        }
        break;
    }

    case MF_MEDIA_ENGINE_EVENT_CANPLAY:
        qCDebug(lcMediaEngine) << "CANPLAY";
        if (state() == QMediaPlayer::PlayingState || mediaStatus() == QMediaPlayer::BufferingMedia
            || m_pendingPlay) {
            mediaStatusChanged(QMediaPlayer::BufferedMedia);
        }
        bufferProgressChanged(1.0f);
        break;

    case MF_MEDIA_ENGINE_EVENT_PLAYING:
        qCDebug(lcMediaEngine) << "PLAYING";
        if (state() != QMediaPlayer::PlayingState)
            stateChanged(QMediaPlayer::PlayingState);
        if (mediaStatus() == QMediaPlayer::BufferingMedia)
            mediaStatusChanged(QMediaPlayer::BufferedMedia);
        startPlaybackTimers();
        break;

    case MF_MEDIA_ENGINE_EVENT_PAUSE:
        qCDebug(lcMediaEngine) << "PAUSE";
        if (state() != QMediaPlayer::StoppedState && state() != QMediaPlayer::PausedState) {
            stateChanged(QMediaPlayer::PausedState);
            stopPlaybackTimers();
        }
        break;

    case MF_MEDIA_ENGINE_EVENT_ENDED: {
        qCDebug(lcMediaEngine) << "ENDED";
        stopPlaybackTimers();

        if (mediaStatus() == QMediaPlayer::BufferingMedia)
            mediaStatusChanged(QMediaPlayer::BufferedMedia);

        if (doLoop()) {
            m_mediaEngine->SetCurrentTime(0);
            m_mediaEngine->Play();
        } else {
            if (m_videoSink)
                m_videoSink->setVideoFrame({});
            positionChanged(m_duration.count());
            stateChanged(QMediaPlayer::StoppedState);
            mediaStatusChanged(QMediaPlayer::EndOfMedia);
            resetCurrentLoop();
        }
        break;
    }

    case MF_MEDIA_ENGINE_EVENT_TIMEUPDATE:
        if (!m_seekTarget)
            positionChanged(currentPositionMs());
        break;

    case MF_MEDIA_ENGINE_EVENT_ERROR: {
        const auto mfErr = static_cast<MF_MEDIA_ENGINE_ERR>(param1);

        QString detail;
        switch (mfErr) {
        case MF_MEDIA_ENGINE_ERR_ABORTED:
            detail = u"Playback aborted"_s;
            break;
        case MF_MEDIA_ENGINE_ERR_NETWORK:
            detail = u"Network error"_s;
            break;
        case MF_MEDIA_ENGINE_ERR_DECODE:
            detail = u"Unsupported or corrupt media"_s;
            break;
        case MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED:
            detail = u"Source not supported"_s;
            break;
        case MF_MEDIA_ENGINE_ERR_ENCRYPTED:
            detail = u"Encrypted content"_s;
            break;
        default:
            detail = u"Unknown error %1"_s.arg(param1);
            break;
        }

        qCWarning(lcMediaEngine) << "Error:" << detail;
        error(mapMediaEngineError(mfErr), detail);
        mediaStatusChanged(QMediaPlayer::InvalidMedia);
        stopPlaybackTimers();
        break;
    }

    case MF_MEDIA_ENGINE_EVENT_BUFFERINGSTARTED:
        qCDebug(lcMediaEngine) << "BUFFERINGSTARTED";
        mediaStatusChanged(QMediaPlayer::BufferingMedia);
        bufferProgressChanged(bufferProgress());
        break;

    case MF_MEDIA_ENGINE_EVENT_BUFFERINGENDED:
        qCDebug(lcMediaEngine) << "BUFFERINGENDED";
        mediaStatusChanged(QMediaPlayer::BufferedMedia);
        bufferProgressChanged(bufferProgress());
        break;

    case MF_MEDIA_ENGINE_EVENT_DURATIONCHANGE:
        qCDebug(lcMediaEngine) << "DURATIONCHANGE";
        updateDuration();
        break;

    case MF_MEDIA_ENGINE_EVENT_FORMATCHANGE:
        qCDebug(lcMediaEngine) << "FORMATCHANGE";
        updateVideoFrameFormat();
        break;

    case MF_MEDIA_ENGINE_EVENT_FIRSTFRAMEREADY:
        qCDebug(lcMediaEngine) << "FIRSTFRAMEREADY";
        if (state() != QMediaPlayer::StoppedState)
            presentVideoFrame();
        break;

    case MF_MEDIA_ENGINE_EVENT_SEEKED:
        qCDebug(lcMediaEngine) << "SEEKED";
        m_seekTimeoutTimer.stop();
        m_seekTarget.reset();
        break;

    default:
        break;
    }
}

void QMediaEnginePlayer::setMedia(const QUrl &media, QIODevice *stream)
{
    qCDebug(lcMediaEngine) << "setMedia:" << media << "stream:" << stream;

    if (state() != QMediaPlayer::StoppedState)
        stop();

    m_url = media;
    m_stream = stream;

    m_duration = {};
    m_pendingPlay = false;
    m_seekTarget.reset();
    m_seekTimeoutTimer.stop();
    seekableChanged(false);
    m_videoSize = {};
    m_transferTexture.Reset();
    m_metaData = {};
    m_metadataFuture.cancel();

    if (m_videoSink)
        m_videoSink->setVideoFrame({});

    m_tracks = {};

    if (media.isEmpty() && !stream) {
        audioAvailableChanged(false);
        videoAvailableChanged(false);
        // IMFMediaEngine has no API to clear the current source without
        // shutting down. Release the engine now; it will be lazily
        // recreated on the next setMedia() call with a real source.
        releaseMediaEngine();
        metaDataChanged();
        mediaStatusChanged(QMediaPlayer::NoMedia);
        return;
    }

    // Lazy init: create the MediaEngine on first use,
    // or recreate it after a previous NoMedia release.
    if (!m_mediaEngine) {
        if (!initMediaEngine()) {
            error(QMediaPlayer::ResourceError, u"MediaEngine not initialized"_s);
            mediaStatusChanged(QMediaPlayer::InvalidMedia);
            return;
        }
    }

    HRESULT hr = E_FAIL;

    if (stream) {
        ComPtr<MFStream> mfStream = makeComObject<MFStream>(stream, false);
        QString urlStr = media.toString();
        // SetSourceFromByteStream requires a non-null URL.
        // Use the real file name when available so that Media Foundation
        // can infer the MIME type from the extension.
        if (urlStr.isEmpty()) {
            if (auto *file = qobject_cast<QFile *>(stream))
                urlStr = u"file:///"_s + file->fileName();
            else
                urlStr = u"stream:///media"_s;
        }
        const QBStr bstrUrl(urlStr);
        hr = m_mediaEngineEx->SetSourceFromByteStream(mfStream.Get(), bstrUrl.bstr());
    } else {
        const QBStr bstrUrl(media.toString());
        hr = m_mediaEngine->SetSource(bstrUrl.bstr());
    }

    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "SetSource:" << QSystemError::windowsComString(hr);
        error(QMediaPlayer::ResourceError, u"Failed to set media source"_s);
        mediaStatusChanged(QMediaPlayer::InvalidMedia);
        return;
    }

    mediaStatusChanged(QMediaPlayer::LoadingMedia);
    applyAudioOutput();
}

void QMediaEnginePlayer::play()
{
    if (!m_mediaEngine)
        return;

    if (mediaStatus() == QMediaPlayer::NoMedia || mediaStatus() == QMediaPlayer::InvalidMedia) {
        return;
    }

    if (mediaStatus() == QMediaPlayer::LoadingMedia) {
        m_pendingPlay = true;
        return;
    }

    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        if (!m_seekTarget) {
            m_mediaEngine->SetCurrentTime(0);
            positionChanged(0);
        }
        resetCurrentLoop();
        mediaStatusChanged(QMediaPlayer::BufferingMedia);
    } else if (mediaStatus() == QMediaPlayer::LoadedMedia
               || mediaStatus() == QMediaPlayer::BufferedMedia) {
        mediaStatusChanged(QMediaPlayer::BufferingMedia);
    }

    HRESULT hr = m_mediaEngine->Play();
    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "Play:" << QSystemError::windowsComString(hr);
        return;
    }

    stateChanged(QMediaPlayer::PlayingState);
    startPlaybackTimers();
}

void QMediaEnginePlayer::pause()
{
    if (!m_mediaEngine)
        return;

    m_pendingPlay = false;
    HRESULT hr = m_mediaEngine->Pause();
    if (FAILED(hr)) {
        qCWarning(lcMediaEngine) << "Pause:" << QSystemError::windowsComString(hr);
        return;
    }

    if (mediaStatus() != QMediaPlayer::NoMedia && mediaStatus() != QMediaPlayer::InvalidMedia) {
        stateChanged(QMediaPlayer::PausedState);
        m_positionUpdateTimer.stop();
    }

    // Briefly start the render timer to deliver the current frame.
    if (isVideoAvailable() && !m_videoRenderTimer.isActive())
        m_videoRenderTimer.start();
}

void QMediaEnginePlayer::stop()
{
    if (!m_mediaEngine)
        return;

    m_pendingPlay = false;
    m_seekTarget.reset();
    m_seekTimeoutTimer.stop();
    stopPlaybackTimers();

    // Don't call Pause() if already at EndOfMedia — the engine is already
    // stopped internally, and Pause() in this state can prevent subsequent
    // Play() from working.
    if (mediaStatus() != QMediaPlayer::EndOfMedia) {
        HRESULT hr = m_mediaEngine->Pause();
        if (FAILED(hr))
            qCWarning(lcMediaEngine) << "Pause (in stop):" << QSystemError::windowsComString(hr);
    }

    // Flush the current position before SetCurrentTime(0) so that
    // QPlatformMediaPlayer's duplicate filter doesn't suppress positionChanged(0).
    positionChanged(currentPositionMs());

    m_mediaEngine->SetCurrentTime(0);

    if (m_videoSink)
        m_videoSink->setVideoFrame({});

    stateChanged(QMediaPlayer::StoppedState);

    positionChanged(0);

    if (mediaStatus() == QMediaPlayer::BufferedMedia
        || mediaStatus() == QMediaPlayer::BufferingMedia
        || mediaStatus() == QMediaPlayer::EndOfMedia) {
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
        bufferProgressChanged(0);
    }

    resetCurrentLoop();
}

qint64 QMediaEnginePlayer::position() const
{
    if (!m_mediaEngine)
        return 0;

    if (m_seekTarget.has_value())
        return m_seekTarget->count();

    return currentPositionMs();
}

qint64 QMediaEnginePlayer::currentPositionMs() const
{
    if (!m_mediaEngine)
        return 0;
    const qint64 pos = qRound64(m_mediaEngine->GetCurrentTime() * 1000.0);
    // GetCurrentTime() can overshoot the reported duration slightly.
    return m_duration.count() > 0 ? qMin(pos, m_duration.count()) : pos;
}

QSize QMediaEnginePlayer::nativeVideoSize() const
{
    if (!m_mediaEngine)
        return {};
    DWORD w = 0, h = 0;
    m_mediaEngine->GetNativeVideoSize(&w, &h);
    return { static_cast<int>(w), static_cast<int>(h) };
}

void QMediaEnginePlayer::setPosition(qint64 position)
{
    if (!m_mediaEngine)
        return;

    position = qMax(qint64(0), position);
    if (const qint64 dur = duration(); dur > 0)
        position = qMin(position, dur);

    // IMFMediaEngine auto-resumes after a seek from the ended state
    // (per HTML5 spec). Remember the ended state before SetCurrentTime
    // resets it, so we can pause immediately after to prevent silent
    // playback when our state is Stopped or Paused.
    const bool wasEnded = m_mediaEngine->IsEnded();

    m_seekTarget = std::chrono::milliseconds(position);
    m_mediaEngine->SetCurrentTime(position / 1000.0);

    if (wasEnded && state() != QMediaPlayer::PlayingState)
        m_mediaEngine->Pause();

    // SEEKED may not arrive after device lost or rapid seeks.
    // Reset m_seekTarget on timeout to unblock position/frame updates.
    m_seekTimeoutTimer.start();

    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);

    positionChanged(position);

    if (isVideoAvailable() && state() == QMediaPlayer::PausedState)
        m_videoRenderTimer.start();
}

qreal QMediaEnginePlayer::playbackRate() const
{
    return m_playbackRate;
}

void QMediaEnginePlayer::setPlaybackRate(qreal rate)
{
    if (qFuzzyCompare(rate, m_playbackRate))
        return;

    m_playbackRate = rate;

    if (m_mediaEngine) {
        m_mediaEngine->SetDefaultPlaybackRate(rate);
        m_mediaEngine->SetPlaybackRate(rate);
    }

    playbackRateChanged(rate);
}

float QMediaEnginePlayer::bufferProgress() const
{
    if (!m_mediaEngine)
        return 0;

    ComPtr<IMFMediaTimeRange> buffered;
    if (FAILED(m_mediaEngine->GetBuffered(&buffered)) || !buffered)
        return 0;

    const DWORD count = buffered->GetLength();
    if (count == 0)
        return 0;

    const double dur = m_mediaEngine->GetDuration();
    if (!std::isfinite(dur) || dur <= 0)
        return 0;

    double end = 0;
    buffered->GetEnd(count - 1, &end);
    return static_cast<float>(qBound(0.0, end / dur, 1.0));
}

QMediaTimeRange QMediaEnginePlayer::availablePlaybackRanges() const
{
    QMediaTimeRange ranges;
    if (!m_mediaEngine)
        return ranges;

    ComPtr<IMFMediaTimeRange> buffered;
    if (FAILED(m_mediaEngine->GetBuffered(&buffered)) || !buffered)
        return ranges;

    const DWORD count = buffered->GetLength();

    for (DWORD i = 0; i < count; ++i) {
        double start = 0, end = 0;
        buffered->GetStart(i, &start);
        buffered->GetEnd(i, &end);
        ranges.addInterval(qRound64(start * 1000), qRound64(end * 1000));
    }

    return ranges;
}

qint64 QMediaEnginePlayer::duration() const
{
    return m_duration.count();
}

QUrl QMediaEnginePlayer::media() const
{
    return m_url;
}

const QIODevice *QMediaEnginePlayer::mediaStream() const
{
    return m_stream;
}

QMediaEnginePlayer::PitchCompensationAvailability
QMediaEnginePlayer::pitchCompensationAvailability() const
{
    return PitchCompensationAvailability::AlwaysOn;
}

QMediaMetaData QMediaEnginePlayer::metaData() const
{
    return m_metaData;
}

void QMediaEnginePlayer::presentVideoFrame()
{
    if (!m_mediaEngine || !isVideoAvailable())
        return;

    // Auto-stop the render timer when not in playing state, regardless of
    // which code path we take (including early returns). Keep it running
    // while a seek is pending so we can deliver the post-seek frame.
    const bool isPlaying = (state() == QMediaPlayer::PlayingState);
    auto stopGuard = qScopeGuard([&] {
        if (!isPlaying && !m_seekTarget)
            m_videoRenderTimer.stop();
    });

    LONGLONG pts = 0;
    if (m_mediaEngine->OnVideoStreamTick(&pts) != S_OK)
        return;

    // While a seek is pending, skip frame delivery — the engine may
    // still return a frame from the pre-seek position.
    if (m_seekTarget)
        return;

    if (!m_videoSink)
        return;

    const QSize videoSize = nativeVideoSize();
    if (videoSize.isEmpty())
        return;

    if (videoSize != m_videoSize) {
        m_videoSize = videoSize;
        m_transferTexture.Reset();
    }

    if (!m_transferTexture) {
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = videoSize.width();
        desc.Height = videoSize.height();
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags = 0;

        HRESULT hr = m_d3dDevice->CreateTexture2D(&desc, nullptr, &m_transferTexture);
        if (FAILED(hr)) {
            qCWarning(lcMediaEngine) << "CreateTexture2D:" << QSystemError::windowsComString(hr);
            return;
        }

        updateVideoFrameFormat();
    }

    MFVideoNormalizedRect srcRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    RECT destRect = { 0, 0, videoSize.width(), videoSize.height() };

    HRESULT hr = m_mediaEngine->TransferVideoFrame(m_transferTexture.Get(), &srcRect, &destRect,
                                                   nullptr);

    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {

            if (m_deviceLostRetries >= kMaxDeviceLostRetries) {
                qCWarning(lcMediaEngine) << "Device lost recovery failed after"
                                         << m_deviceLostRetries << "attempts, giving up";
                error(QMediaPlayer::ResourceError, u"D3D11 device lost recovery failed"_s);
                return;
            }
            ++m_deviceLostRetries;

            qCWarning(lcMediaEngine) << "Device lost during TransferVideoFrame, recovering"
                                     << "(attempt" << m_deviceLostRetries << ")...";

            const qint64 savedPosition = currentPositionMs();
            const auto savedState = state();
            stopPlaybackTimers();

            // Release dead resources immediately — the device is already lost.
            releaseMediaEngine();
            m_transferTexture.Reset();
            m_d3dDevice.Reset();

            // Defer re-creation to avoid re-entering presentVideoFrame().
            QTimer::singleShot(0, context(), [this, savedPosition, savedState] {
                if (!initD3D11()) {
                    error(QMediaPlayer::ResourceError, u"D3D11 device recreation failed"_s);
                    return;
                }

                if (!m_url.isEmpty() || m_stream) {
                    setMedia(m_url, m_stream);

                    if (savedPosition > 0)
                        setPosition(savedPosition);
                    if (savedState == QMediaPlayer::PlayingState)
                        play();
                }
            });
            return;
        }
        qCWarning(lcMediaEngine) << "TransferVideoFrame:" << QSystemError::windowsComString(hr);
        return;
    }

    // Successful frame transfer — reset device lost counter.
    m_deviceLostRetries = 0;

    auto buffer = std::make_unique<QMediaEngineVideoBuffer>(m_transferTexture, m_d3dDevice,
                                                            videoSize, m_textureImporter);

    QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(buffer), m_videoFormat);

    auto startTime = std::chrono::round<std::chrono::microseconds>(reference_time(pts));

    frame.setStartTime(startTime.count());
    if (m_frameDuration > 0us)
        frame.setEndTime((startTime + m_frameDuration).count());

    m_videoSink->setVideoFrame(frame);
    m_videoSink->setNativeSize(videoSize);
}

// IMFMediaEngine already applies rotation to the decoded pixels, so
// the video format does not include rotation — VideoOutput would apply
// it a second time.
void QMediaEnginePlayer::updateVideoFrameFormat()
{
    const QSize size = nativeVideoSize();
    if (size.isEmpty())
        return;

    m_videoFormat = QVideoFrameFormat(size, QVideoFrameFormat::Format_BGRA8888);
    m_frameDuration = videoFrameDurationUs(m_mediaEngineEx.Get());
}

void QMediaEnginePlayer::updateNativeSize()
{
    if (!m_videoSink)
        return;

    const QSize size = nativeVideoSize();
    if (!size.isEmpty())
        m_videoSink->setNativeSize(size);
}

void QMediaEnginePlayer::setVideoSink(QVideoSink *sink)
{
    auto *newSink = sink ? sink->platformVideoSink() : nullptr;
    auto *oldSink = m_videoSink;
    m_videoSink = newSink;
    updateNativeSize();

    // Transfer the current frame to the new sink (e.g. when switching
    // sinks while paused — OnVideoStreamTick won't return the same frame).
    if (newSink && oldSink)
        newSink->setVideoFrame(oldSink->currentVideoFrame());
}

void QMediaEnginePlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;

    if (m_audioOutput && m_audioOutput->q)
        m_audioOutput->q->disconnect(context());

    m_audioOutput = output;

    if (output && output->q) {
        QObject::connect(output->q, &QAudioOutput::volumeChanged, context(), [this] {
            applyAudioOutput();
        });
        QObject::connect(output->q, &QAudioOutput::mutedChanged, context(), [this] {
            applyAudioOutput();
        });
        QObject::connect(output->q, &QAudioOutput::deviceChanged, context(), [this] {
            applyAudioOutput();
        });
    }

    applyAudioOutput();
}

void QMediaEnginePlayer::applyAudioOutput()
{
    if (!m_mediaEngine)
        return;

    if (!m_audioOutput) {
        m_mediaEngine->SetMuted(TRUE);
        return;
    }

    m_mediaEngine->SetVolume(static_cast<double>(m_audioOutput->volume));
    m_mediaEngine->SetMuted(m_audioOutput->muted ? TRUE : FALSE);

    const QByteArray id = m_audioOutput->device.id();
    if (!id.isEmpty()) {
        ComPtr<IMFMediaEngineAudioEndpointId> endpointId;
        if (SUCCEEDED(m_mediaEngine.As(&endpointId))) {
            const QString deviceId = QString::fromUtf8(id);
            endpointId->SetAudioEndpointId(reinterpret_cast<LPCWSTR>(deviceId.utf16()));
        }
    }
}

void QMediaEnginePlayer::updateMetaData()
{
    if (!m_mediaEngineEx)
        return;

    m_metaData = {};

    const QSize size = nativeVideoSize();
    if (!size.isEmpty())
        m_metaData.insert(QMediaMetaData::Resolution, size);

    if (m_duration > 0ms)
        m_metaData.insert(QMediaMetaData::Duration, m_duration.count());

    m_metadataFuture.cancel();

    const QUrl url = m_url;

    // IMFSourceResolver needs a URL; skip for pure QIODevice streams.
    if (url.isEmpty()) {
        metaDataChanged();
        return;
    }

    m_metadataFuture = QtConcurrent::run([url]() -> QMediaMetaData {
        ComPtr<IMFSourceResolver> resolver;
        if (FAILED(MFCreateSourceResolver(&resolver)))
            return {};

        ComPtr<IUnknown> sourceUnk;
        MF_OBJECT_TYPE objectType = MF_OBJECT_INVALID;
        HRESULT hr = E_FAIL;

        if (url.scheme() == u"qrc"_s) {
            auto file = std::make_unique<QFile>(u':' + url.path());
            if (!file->open(QIODevice::ReadOnly))
                return {};

            // MFStream in synchronous mode: BeginRead performs the read
            // immediately and invokes the callback on the calling thread,
            // so no event loop is needed.  ownStream=true so MFStream
            // takes ownership of the QFile.
            ComPtr<MFStream> mfStream =
                    makeComObject<MFStream>(file.release(), true, true /*synchronous*/);

            const auto urlStr = url.toString();
            hr = resolver->CreateObjectFromByteStream(
                    mfStream.Get(), reinterpret_cast<LPCWSTR>(urlStr.utf16()),
                    MF_RESOLUTION_MEDIASOURCE
                            | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                    nullptr, &objectType, &sourceUnk);
        } else {
            const auto urlStr = url.toString();
            hr = resolver->CreateObjectFromURL(
                    reinterpret_cast<LPCWSTR>(urlStr.utf16()),
                    MF_RESOLUTION_MEDIASOURCE
                            | MF_RESOLUTION_CONTENT_DOES_NOT_HAVE_TO_MATCH_EXTENSION_OR_MIME_TYPE,
                    nullptr, &objectType, &sourceUnk);
        }

        if (FAILED(hr) || !sourceUnk)
            return {};

        ComPtr<IMFMediaSource> mediaSource;
        if (FAILED(sourceUnk.As(&mediaSource)))
            return {};

        QMediaMetaData result = MFMetaData::fromNative(mediaSource.Get());
        mediaSource->Shutdown();
        return result;
    }).then(context(), [this](QMediaMetaData rich) {
        if (!rich.isEmpty()) {
            for (const auto &[key, value] : std::as_const(m_metaData).asKeyValueRange())
                rich.insert(key, value);
            m_metaData = rich;
        }
        metaDataChanged();
    });
}

void QMediaEnginePlayer::updateTracks()
{
    if (!m_mediaEngineEx)
        return;

    m_tracks = {};

    DWORD streamCount = 0;
    if (FAILED(m_mediaEngineEx->GetNumberOfStreams(&streamCount)))
        return;

    for (DWORD i = 0; i < streamCount; ++i) {
        std::optional<TrackType> trackType;
        {
            ScopedPropVariant var;
            if (FAILED(m_mediaEngineEx->GetStreamAttribute(i, MF_MT_MAJOR_TYPE, var.get())))
                continue;

            if (var->vt == VT_CLSID) {
                if (*var->puuid == MFMediaType_Audio)
                    trackType = TrackType::AudioStream;
                else if (*var->puuid == MFMediaType_Video)
                    trackType = TrackType::VideoStream;
            }
        }

        if (!trackType)
            continue;

        QMediaMetaData trackMeta;
        {
            ScopedPropVariant var;
            if (SUCCEEDED(m_mediaEngineEx->GetStreamAttribute(i, MF_SD_LANGUAGE, var.get()))) {
                if (var->vt == VT_LPWSTR && var->pwszVal)
                    trackMeta.insert(QMediaMetaData::Language,
                                     languageVariant(QString::fromWCharArray(var->pwszVal)));
            }
        }

        m_tracks[*trackType].entries.push_back({ trackMeta, i });

        BOOL selected = FALSE;
        if (SUCCEEDED(m_mediaEngineEx->GetStreamSelection(i, &selected)) && selected)
            m_tracks[*trackType].currentIndex =
                    static_cast<int>(m_tracks[*trackType].entries.size()) - 1;
    }

    updateSubtitleTracks();

    tracksChanged();
}

void QMediaEnginePlayer::updateSubtitleTracks()
{
    auto &subtitleInfo = m_tracks[TrackType::SubtitleStream];
    subtitleInfo = {};

    if (!m_timedText)
        return;

    ComPtr<IMFTimedTextTrackList> trackList;
    if (FAILED(m_timedText->GetTextTracks(&trackList)) || !trackList)
        return;

    const DWORD count = trackList->GetLength();
    for (DWORD i = 0; i < count; ++i) {
        ComPtr<IMFTimedTextTrack> track;
        if (FAILED(trackList->GetTrack(i, &track)) || !track)
            continue;

        QMediaMetaData trackMeta;

        QComTaskResource<WCHAR> label;
        if (SUCCEEDED(track->GetLabel(label.address())) && label)
            trackMeta.insert(QMediaMetaData::Title, QString::fromWCharArray(label.get()));

        QComTaskResource<WCHAR> lang;
        if (SUCCEEDED(track->GetLanguage(lang.address())) && lang)
            trackMeta.insert(QMediaMetaData::Language,
                             languageVariant(QString::fromWCharArray(lang.get())));

        // Store the timed-text track ID as the streamIndex for SelectTrack.
        const DWORD trackId = track->GetId();

        subtitleInfo.entries.push_back({ trackMeta, trackId });

        if (track->IsActive()) {
            subtitleInfo.currentIndex = static_cast<int>(subtitleInfo.entries.size()) - 1;
        }
    }
}

void QMediaEnginePlayer::updateDuration()
{
    if (!m_mediaEngine)
        return;

    const double dur = m_mediaEngine->GetDuration();
    const auto newDuration = std::chrono::milliseconds(
            (std::isfinite(dur) && dur >= 0) ? qRound64(dur * 1000.0) : 0);
    if (newDuration != m_duration) {
        m_duration = newDuration;
        durationChanged(m_duration.count());
    }
}

int QMediaEnginePlayer::trackCount(TrackType type)
{
    return static_cast<int>(m_tracks[type].entries.size());
}

QMediaMetaData QMediaEnginePlayer::trackMetaData(TrackType type, int trackNumber)
{
    if (trackNumber < 0 || trackNumber >= static_cast<int>(m_tracks[type].entries.size()))
        return {};
    return m_tracks[type].entries.at(trackNumber).metaData;
}

int QMediaEnginePlayer::activeTrack(TrackType type)
{
    return m_tracks[type].currentIndex;
}

void QMediaEnginePlayer::setActiveTrack(TrackType type, int index)
{
    if (index < -1 || index >= static_cast<int>(m_tracks[type].entries.size()))
        return;

    if (type == TrackType::SubtitleStream) {
        // Subtitle tracks are managed via IMFTimedText, not stream selection.
        if (!m_timedText)
            return;

        const int currentIdx = m_tracks[type].currentIndex;
        if (currentIdx >= 0 && currentIdx < static_cast<int>(m_tracks[type].entries.size())) {
            DWORD trackId = m_tracks[type].entries.at(currentIdx).streamIndex;
            m_timedText->SelectTrack(trackId, FALSE);
        }

        if (index >= 0) {
            DWORD trackId = m_tracks[type].entries.at(index).streamIndex;
            m_timedText->SelectTrack(trackId, TRUE);
        }

        if (m_videoSink)
            m_videoSink->setSubtitleText({});

        m_tracks[type].currentIndex = index;
        activeTracksChanged();
        return;
    }

    if (!m_mediaEngineEx)
        return;

    const int currentIdx = m_tracks[type].currentIndex;
    if (currentIdx >= 0 && currentIdx < static_cast<int>(m_tracks[type].entries.size())) {
        DWORD streamIdx = m_tracks[type].entries.at(currentIdx).streamIndex;
        m_mediaEngineEx->SetStreamSelection(streamIdx, FALSE);
    }

    if (index >= 0) {
        DWORD streamIdx = m_tracks[type].entries.at(index).streamIndex;
        m_mediaEngineEx->SetStreamSelection(streamIdx, TRUE);
    }

    m_tracks[type].currentIndex = index;
    activeTracksChanged();
}

QT_END_NAMESPACE
