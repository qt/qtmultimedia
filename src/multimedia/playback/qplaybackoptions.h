// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPLAYBACKOPTIONS_H
#define QPLAYBACKOPTIONS_H

#include <QtMultimedia/qtmultimediaexports.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qcompare.h>
#include <QtCore/qobject.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

class QPlaybackOptionsPrivate;
class Q_MULTIMEDIA_EXPORT QPlaybackOptions
{
    Q_GADGET
    Q_PROPERTY(int networkTimeoutMs READ networkTimeoutMs WRITE setNetworkTimeoutMs RESET
                       resetNetworkTimeoutMs FINAL)
    Q_PROPERTY(PlaybackIntent playbackIntent READ playbackIntent WRITE setPlaybackIntent RESET
                       resetPlaybackIntent)
    Q_PROPERTY(int probeSize READ probeSize WRITE setProbeSize RESET resetProbeSize)

public:
    enum PlaybackIntent {
        Playback,
        LowLatencyStreaming
    };
    Q_ENUM(PlaybackIntent)

    QPlaybackOptions();
    QPlaybackOptions(const QPlaybackOptions &);
    QPlaybackOptions &operator=(const QPlaybackOptions &);
    QPlaybackOptions(QPlaybackOptions &&) noexcept;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QPlaybackOptions)
    ~QPlaybackOptions();

    void swap(QPlaybackOptions &other) noexcept;

    int networkTimeoutMs() const;
    void setNetworkTimeoutMs(int timeout);
    void resetNetworkTimeoutMs();

    PlaybackIntent playbackIntent() const;
    void setPlaybackIntent(PlaybackIntent intent);
    void resetPlaybackIntent();

    int probeSize() const;
    void setProbeSize(int probeSizeBytes);
    void resetProbeSize();

private:
    friend Q_MULTIMEDIA_EXPORT bool comparesEqual(const QPlaybackOptions &lhs,
                                                  const QPlaybackOptions &rhs);
    friend Q_MULTIMEDIA_EXPORT Qt::strong_ordering compareThreeWay(const QPlaybackOptions &lhs,
                                                                   const QPlaybackOptions &rhs);
    Q_DECLARE_STRONGLY_ORDERED_NON_NOEXCEPT(QPlaybackOptions)

    friend class QPlaybackOptionsPrivate;
    QExplicitlySharedDataPointer<QPlaybackOptionsPrivate> d;
};

Q_DECLARE_SHARED(QPlaybackOptions)

QT_END_NAMESPACE

#endif
