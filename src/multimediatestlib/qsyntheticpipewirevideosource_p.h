// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSYNTHETICPIPEWIREVIDEOSOURCE_P_H
#define QSYNTHETICPIPEWIREVIDEOSOURCE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qpipewire_support_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qexpected_p.h>

#include <private/qsyntheticvideoscene_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

/*!
    \internal
    Identity of a synthetic source: what it advertises to the PipeWire graph.

    media.role is load-bearing, not cosmetic. It is how the source decides which
    consumers are meant for it -- see QSyntheticPipeWireVideoSource.
*/
struct QSyntheticPipeWireVideoSourceConfig
{
    QByteArray loopName = QByteArrayLiteral("qt-fake-sc");
    QByteArray nodeName = QByteArrayLiteral("qt-fake-screencast");
    QByteArray nodeDescription = QByteArrayLiteral("Qt Multimedia synthetic screen cast source");
    QByteArray mediaClass = QByteArrayLiteral("Stream/Output/Video");
    QByteArray mediaRole = QByteArrayLiteral("Screen");
    QByteArray mediaCategory = QByteArrayLiteral("Capture");

    //! A screen-cast source, as xdg-desktop-portal's ScreenCast interface hands out.
    [[nodiscard]] static QSyntheticPipeWireVideoSourceConfig screenCast();

    /*!
        A camera source, as the Camera interface hands out: media.class Video/Source
        plus media.role Camera. device.api is deliberately left unset, so that a
        client identifying cameras exercises the media.role branch -- the one a real
        camera portal hits -- rather than the v4l2/libcamera hardware fallback.
    */
    [[nodiscard]] static QSyntheticPipeWireVideoSourceConfig camera(int index = 0);
};

/*!
    \internal

    A synthetic PipeWire video source: an output pw_stream advertising
    \c {media.class = Stream/Output/Video} that publishes frames rendered by
    paintSyntheticVideoScene(). It stands in for a compositor's screen-cast stream,
    so QPipeWireCaptureHelper can be exercised without a real desktop portal.

    The pixel format is fixed to SPA_VIDEO_FORMAT_BGRx, which maps to
    QVideoFrameFormat::Format_BGRX8888 on the consuming side and is byte-identical
    to QImage::Format_RGB32 on little-endian, avoiding a pixel format conversion.

    The source also acts as its own session manager: when it is used against a
    private pipewire daemon there is no WirePlumber, so nothing honours
    PW_STREAM_FLAG_AUTOCONNECT and the consumer would sit in "paused" forever.
    The source therefore links itself, via the link-factory, to consumers meant
    for it.

    Which consumers are "meant for it" is determined by matching media.role, not
    by the target passed to pw_stream_connect(): PipeWire consumes that target
    internally, and the resulting consumer node advertises neither target.object
    nor node.target. media.role is the one property a consumer does advertise --
    QPipeWireCaptureHelper sets Screen, a camera client sets Camera -- so the
    source links only consumer nodes whose media.role matches its own. This
    matters as soon as one graph holds both a screen-cast and a camera source:
    matching by media.role is what keeps camera frames from being linked to a
    screen-capture client.
*/
class QSyntheticPipeWireVideoSource
{
public:
    QSyntheticPipeWireVideoSource();
    ~QSyntheticPipeWireVideoSource();

    Q_DISABLE_COPY_MOVE(QSyntheticPipeWireVideoSource)

    /*!
        Connects to the PipeWire daemon and publishes the source. Blocks until the
        node id is known, so that nodeId() is usable as soon as this returns.
    */
    q23::expected<void, QString> start(const SyntheticVideoScene &scene,
                                       const QSyntheticPipeWireVideoSourceConfig &config =
                                               QSyntheticPipeWireVideoSourceConfig::screenCast());
    void stop();

    //! The PipeWire node id to hand to a consumer. Valid only after a successful start().
    [[nodiscard]] QtPipeWire::ObjectId nodeId() const;

    //! Replaces the published content. Safe to call from any thread.
    void setScene(const SyntheticVideoScene &scene);

    //! Renders and publishes the next frame. Safe to call from any thread.
    void renderNextFrame();

private:
    class Private;
    std::unique_ptr<Private> d;
};

QT_END_NAMESPACE

#endif // QSYNTHETICPIPEWIREVIDEOSOURCE_P_H
