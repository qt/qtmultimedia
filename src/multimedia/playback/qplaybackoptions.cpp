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
        return lhs.m_networkTimeout == rhs.m_networkTimeout;
    }

    friend Qt::strong_ordering compareThreeWay(const QPlaybackOptionsPrivate &lhs,
                                               const QPlaybackOptionsPrivate &rhs)
    {
        return qCompareThreeWay(lhs.m_networkTimeout.count(), rhs.m_networkTimeout.count());
    }

    std::chrono::milliseconds m_networkTimeout{ 5'000 };
};

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
QPlaybackOptions::QPlaybackOptions(QPlaybackOptions &&) noexcept = default;
QPlaybackOptions::~QPlaybackOptions() = default;

void QPlaybackOptions::swap(QPlaybackOptions &other) noexcept
{
    d.swap(other.d);
}

bool comparesEqual(const QPlaybackOptions &lhs, const QPlaybackOptions &rhs)
{
    if (lhs.d == rhs.d)
        return true;

    return comparesEqual(*lhs.d, *rhs.d);
}

Qt::strong_ordering compareThreeWay(const QPlaybackOptions &lhs, const QPlaybackOptions &rhs)
{
    return compareThreeWay(*lhs.d, *rhs.d);
}

/*!
    \property QPlaybackOptions::networkTimeoutMs
    \since 6.10

    Determines the network timeout (in milliseconds) used for socket I/O operations with some
    network formats.

    This option is only supported with the FFmpeg media backend.
*/

/*!
    \qmlproperty int playbackOptions::networkTimeoutMs
    \since 6.10

    Determines the network timeout (in milliseconds) used for socket I/O operations with some
    network formats.

    This option is only supported with the FFmpeg media backend.
*/

int QPlaybackOptions::networkTimeoutMs() const
{
    return static_cast<int>(d->m_networkTimeout.count());
}

void QPlaybackOptions::setNetworkTimeoutMs(int timeout)
{
    d.detach();
    d->m_networkTimeout = std::chrono::milliseconds{ timeout };
}

void QPlaybackOptions::resetNetworkTimeoutMs()
{
    d.detach();
    d->m_networkTimeout = QPlaybackOptionsPrivate{}.m_networkTimeout;
}

QT_END_NAMESPACE
