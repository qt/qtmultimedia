// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QSYNTHETICVIDEOSCENE_P_H
#define QSYNTHETICVIDEOSCENE_P_H

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

#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>
#include <QtCore/qrect.h>
#include <QtCore/qsize.h>
#include <QtCore/qvariantmap.h>

QT_BEGIN_NAMESPACE

class QPainter;

/*!
    \internal

    Describes the content of a synthetic video frame -- a virtual "screen" for
    screen-capture tests, or a virtual camera image.

    The same description drives both the real widget shown on screen and the
    synthetic PipeWire source used by QFakeXdgPortalFixture, so that a test can
    make identical pixel assertions whichever of the two produced the frame.
    Keeping one paint routine (paintSyntheticVideoScene()) is the point: it makes
    "the fake source shows the same thing as the widget" structural rather than
    a duplicated implementation that can drift.
*/
struct SyntheticVideoScene
{
    // Whole virtual screen, in device pixels.
    QSize frameSize{ 1920, 1080 };
    // Where the pattern sits on that screen, in device pixels.
    QRect patternRect{ 200, 100, 430, 351 };
    QColor backgroundColor{ Qt::black };
    QColor firstColor{ 0xFF, 0, 0 };
    QColor secondColor{ 0, 0, 0xFF };
    // When set, firstColor/secondColor swap on odd ticks, so consecutive frames
    // differ. Some capture backends stop emitting frames on static content.
    bool animated = false;
    /*
        Scales the inset's margins. The margins below are expressed in
        device-independent pixels, because that is the space a widget paints in
        (QPainter applies the device pixel ratio for it). When the scene is
        rendered straight into a device-pixel framebuffer instead, as the
        synthetic PipeWire source does, they have to be scaled explicitly.
    */
    qreal scale = 1.0;

    // The inset filled with secondColor. Matches QTestWidget::paintEvent().
    [[nodiscard]] QRect insetRect() const
    {
        return patternRect.adjusted(qRound(40 * scale), qRound(50 * scale), qRound(-60 * scale),
                                    qRound(-70 * scale));
    }

    [[nodiscard]] QVariantMap toVariantMap() const;
    [[nodiscard]] static SyntheticVideoScene fromVariantMap(const QVariantMap &map);
};

/*!
    \internal
    Paints \a scene with \a painter. \a tick only matters when scene.animated is set.
*/
void paintSyntheticVideoScene(QPainter &painter, const SyntheticVideoScene &scene,
                              unsigned tick = 0);

/*!
    \internal
    Renders \a scene into a QImage::Format_RGB32 image of scene.frameSize.

    Format_RGB32 is deliberate: its in-memory byte order is B,G,R,x on
    little-endian, identical to SPA_VIDEO_FORMAT_BGRx, so the PipeWire producer
    can memcpy rows without any conversion.
*/
[[nodiscard]] QImage renderSyntheticVideoScene(const SyntheticVideoScene &scene, unsigned tick = 0);

QT_END_NAMESPACE

#endif // QSYNTHETICVIDEOSCENE_P_H
