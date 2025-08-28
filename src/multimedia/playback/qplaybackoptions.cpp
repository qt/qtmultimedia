// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplaybackoptions.h"
#include <chrono>

QT_BEGIN_NAMESPACE

class QPlaybackOptionsPrivate : public QSharedData
{
public:
    QPlaybackOptionsPrivate() = default;

    friend bool comparesEqual(const QPlaybackOptionsPrivate &lhs,
                              const QPlaybackOptionsPrivate &rhs)
    {
        return lhs.m_networkTimeout == rhs.m_networkTimeout
                && lhs.m_playbackIntent == rhs.m_playbackIntent
                && lhs.m_probeSizeBytes == rhs.m_probeSizeBytes;
    }

    friend Qt::strong_ordering compareThreeWay(const QPlaybackOptionsPrivate &lhs,
                                               const QPlaybackOptionsPrivate &rhs)
    {
        if (lhs.m_networkTimeout != rhs.m_networkTimeout)
            return qCompareThreeWay(lhs.m_networkTimeout.count(), rhs.m_networkTimeout.count());
        if (lhs.m_playbackIntent != rhs.m_playbackIntent)
            return qCompareThreeWay(lhs.m_playbackIntent, rhs.m_playbackIntent);
        return qCompareThreeWay(lhs.m_probeSizeBytes, rhs.m_probeSizeBytes);
    }

    std::chrono::milliseconds m_networkTimeout{ 5'000 };
    QPlaybackOptions::PlaybackIntent m_playbackIntent = QPlaybackOptions::PlaybackIntent::Playback;
    int m_probeSizeBytes = -1;
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QPlaybackOptionsPrivate)

/*!
    \class QPlaybackOptions
    \brief The QPlaybackOptions class enables low-level control of media playback options.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_playback
    \ingroup multimedia_video
    \since 6.10

    QPlaybackOptions gives low-level control of media playback options. Although we strongly
    recommend to rely on the default settings of \l QMediaPlayer, QPlaybackOptions can be used to
    optimize media playback to specific use cases where the default options are not ideal.

    Note that options are hints to the media backend, and may be ignored if they are not supported
    by the current media format or codec.

    Playback options rely on support in the media backend. Availability is documented per option.

    \sa QMediaPlayer
*/

/*!
    \qmltype playbackOptions
    \nativetype QPlaybackOptions
    \brief Low level media playback options.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \ingroup multimedia_video_qml
    \since 6.10

    Playback options gives low-level control of media playback options. Although we strongly
    recommend to rely on the default settings of \l MediaPlayer, playbackOptions can be used to
    optimize media playback to specific use cases where the default options are not ideal.

    Note that options are hints to the media backend, and may be ignored if they are not supported
    by the current media format or codec.

    Playback options rely on support in the media backend. Availability is documented per option.

    \sa MediaPlayer
*/

QPlaybackOptions::QPlaybackOptions() : d{ new QPlaybackOptionsPrivate } { }
QPlaybackOptions::QPlaybackOptions(const QPlaybackOptions &) = default;
QPlaybackOptions &QPlaybackOptions::operator=(const QPlaybackOptions &) = default;
QPlaybackOptions::~QPlaybackOptions() = default;

bool comparesEqual(const QPlaybackOptions &lhs, const QPlaybackOptions &rhs) noexcept
{
    if (lhs.d == rhs.d)
        return true;

    return comparesEqual(*lhs.d, *rhs.d);
}

Qt::strong_ordering compareThreeWay(const QPlaybackOptions &lhs, const QPlaybackOptions &rhs) noexcept
{
    return compareThreeWay(*lhs.d, *rhs.d);
}

/*!
    \property QPlaybackOptions::networkTimeout
    \since 6.10

    Determines the network timeout used for socket I/O operations with some
    network formats.

    This option is only supported with the FFmpeg media backend.
*/

/*!
    \qmlproperty qint64 playbackOptions::networkTimeoutMs
    \since 6.10

    Determines the network timeout (in milliseconds) used for socket I/O operations with some
    network formats.

    This option is only supported with the FFmpeg media backend.
*/

std::chrono::milliseconds QPlaybackOptions::networkTimeout() const
{
    return d->m_networkTimeout;
}

void QPlaybackOptions::setNetworkTimeout(std::chrono::milliseconds timeout)
{
    d.detach();
    d->m_networkTimeout = timeout;
}

void QPlaybackOptions::resetNetworkTimeout()
{
    d.detach();
    d->m_networkTimeout = QPlaybackOptionsPrivate{}.m_networkTimeout;
}

/*!
    \enum QPlaybackOptions::PlaybackIntent
    \since 6.10

    Configures the intent of media playback, to focus on either high quality playback or
    low latency media streaming.

    \value Playback The intent is robust and high quality media playback, enabling sufficient
        buffering to prevent glitches during playback.
    \value LowLatencyStreaming Buffering is reduced to optimize for low latency streaming, but
        with a higher likelihood of lost frames or other glitches during playback.
*/

/*!
    \property QPlaybackOptions::playbackIntent
    \since 6.10

    Determines if \l QMediaPlayer should optimize for robust high quality video playback (default),
    or low latency streaming.

    This option is only supported with the FFmpeg media backend.
*/

/*!
    \qmlproperty PlaybackOptions::PlaybackIntent PlaybackOptions::playbackIntent
    \since 6.10

    Determines if \l MediaPlayer should optimize for robust high quality video playback (default),
    or low latency streaming.

    This option is only supported with the FFmpeg media backend.

    \qmlenumeratorsfrom QPlaybackOptions::PlaybackIntent
*/

QPlaybackOptions::PlaybackIntent QPlaybackOptions::playbackIntent() const
{
    return d->m_playbackIntent;
}

void QPlaybackOptions::setPlaybackIntent(PlaybackIntent intent)
{
    d.detach();
    d->m_playbackIntent = intent;
}

void QPlaybackOptions::resetPlaybackIntent()
{
    d.detach();
    d->m_playbackIntent = QPlaybackOptionsPrivate{}.m_playbackIntent;
}

/*!
    \property QPlaybackOptions::probeSize
    \since 6.10

    Probesize defines the amount of data (in bytes) to analyze in order to gather stream
    information before media playback starts.

    A larger probesize value can give more robust playback but may increase latency. Conversely,
    a smaller probesize can reduce latency but might miss some stream details. The default
    probesize is -1, and the actual probesize is determined by the media backend.

    This option is only supported with the FFmpeg media backend.
*/

/*!
    \qmlproperty qsizetype PlaybackOptions::probeSize
    \since 6.10

    Probesize defines the amount of data (in bytes) to analyze in order to gather stream
    information before media playback starts.

    A larger probesize value can give more robust playback but may increase latency. Conversely,
    a smaller probesize can reduce latency but might miss some stream details. The default
    probesize is -1, and the actual probesize is then determined by the media backend.

    Note that a too small probeSize can result in failure to play the media, while a too high
    probeSize can increase latency.

    This option is only supported with the FFmpeg media backend.
*/

qsizetype QPlaybackOptions::probeSize() const
{
    return d->m_probeSizeBytes;
}

void QPlaybackOptions::setProbeSize(qsizetype probeSizeBytes)
{
    d.detach();
    d->m_probeSizeBytes = static_cast<int>(probeSizeBytes);
}

void QPlaybackOptions::resetProbeSize()
{
    d.detach();
    d->m_probeSizeBytes = QPlaybackOptionsPrivate{}.m_probeSizeBytes;
}

QT_END_NAMESPACE

#include "moc_qplaybackoptions.cpp"
