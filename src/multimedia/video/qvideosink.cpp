// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideosink.h"

#include "qvideoframeformat.h"
#include "qvideoframe.h"
#include "qmediaplayer.h"
#include "qmediacapturesession.h"

#include <qvariant.h>
#include <qpainter.h>
#include <qmatrix4x4.h>
#include <QDebug>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformvideosink_p.h>

QT_BEGIN_NAMESPACE

class QVideoSinkPrivate {
public:
    QVideoSinkPrivate(QVideoSink *q)
        : q_ptr(q)
    {
        videoSink = QPlatformMediaIntegration::instance()->createVideoSink(q);
    }
    ~QVideoSinkPrivate()
    {
        delete videoSink;
    }
    void unregisterSource()
    {
        if (!source)
            return;
        auto *old = source;
        source = nullptr;
        if (auto *player = qobject_cast<QMediaPlayer *>(old))
            player->setVideoSink(nullptr);
        else if (auto *capture = qobject_cast<QMediaCaptureSession *>(old))
            capture->setVideoSink(nullptr);
    }

    QVideoSink *q_ptr = nullptr;
    QPlatformVideoSink *videoSink = nullptr;
    QObject *source = nullptr;
    QRhi *rhi = nullptr;
};

/*!
    \class QVideoSink

    \brief The QVideoSink class represents a generic sink for video data.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video

    The QVideoSink class can be used to retrieve video data on a frame by frame
    basis from Qt Multimedia.

    QVideoSink will provide individual video frames to the application developer
    through the videoFrameChanged() signal.

    The video frame can then be used to read out the data of those frames and handle them
    further. When using QPainter, the QVideoFrame can be drawing using the paint() method
    in QVideoSink.

    QVideoFrame objects can consume a significant amount of memory or system resources and
    should thus not be held for longer than required by the application.

    \sa QMediaPlayer, QMediaCaptureSession

*/

/*!
    Constructs a new QVideoSink object with \a parent.
 */
QVideoSink::QVideoSink(QObject *parent)
    : QObject(parent),
    d(new QVideoSinkPrivate(this))
{
    qRegisterMetaType<QVideoFrame>();
}

/*!
    Destroys the object.
 */
QVideoSink::~QVideoSink()
{
    d->unregisterSource();
    delete d;
}

/*!
    \internal
    Returns the QRhi instance being used to create texture data in the video frames.
 */
QRhi *QVideoSink::rhi() const
{
    return d->rhi;
}

/*!
    \internal
    Sets the QRhi instance being used to create texture data in the video frames
    to \a rhi.
 */
void QVideoSink::setRhi(QRhi *rhi)
{
    if (d->rhi == rhi)
        return;
    d->rhi = rhi;
    d->videoSink->setRhi(rhi);
}

/*!
    \internal
*/
QPlatformVideoSink *QVideoSink::platformVideoSink() const
{
    return d->videoSink;
}

/*!
    Returns the current video frame.
 */
QVideoFrame QVideoSink::videoFrame() const
{
    return d->videoSink->currentVideoFrame();
}

/*!
    Sets the current video \a frame.
*/
void QVideoSink::setVideoFrame(const QVideoFrame &frame)
{
    d->videoSink->setVideoFrame(frame);
}

/*!
    Returns the current subtitle text.
*/
QString QVideoSink::subtitleText() const
{
    return d->videoSink->subtitleText();
}

/*!
    Sets the current \a subtitle text.
*/
void QVideoSink::setSubtitleText(const QString &subtitle)
{
    d->videoSink->setSubtitleText(subtitle);
}

/*!
    Returns the size of the video currently being played back. If no video is
    being played, this method returns an invalid size.
 */
QSize QVideoSink::videoSize() const
{
    return d->videoSink ? d->videoSink->nativeSize() : QSize{};
}

void QVideoSink::setSource(QObject *source)
{
    if (d->source == source)
        return;
    if (source)
        d->unregisterSource();
    d->source = source;
}

QT_END_NAMESPACE

#include "moc_qvideosink.cpp"


