// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGVIDEONODE_P_H
#define QSGVIDEONODE_P_H

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

#include <QtQuick/qsgnode.h>
#include <private/qtmultimediaquickglobal_p.h>
#include "private/qvideotexturehelper_p.h"

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtGui/qopenglfunctions.h>

QT_BEGIN_NAMESPACE

class QSGVideoMaterial;
class QQuickVideoOutput;
class QQuickTextNode;

class QSGVideoNode : public QSGGeometryNode
{
public:
    QSGVideoNode(QQuickVideoOutput *parent, const QVideoFrameFormat &format);
    ~QSGVideoNode();

    QVideoFrameFormat::PixelFormat pixelFormat() const {
        return m_format.pixelFormat();
    }
    void setCurrentFrame(const QVideoFrame &frame);

    void setTexturedRectGeometry(const QRectF &boundingRect, const QRectF &textureRect, int orientation);

private:
    void updateSubtitle(const QVideoFrame &frame);
    void setSubtitleGeometry();

    QQuickVideoOutput *m_parent = nullptr;
    QRectF m_rect;
    QRectF m_textureRect;
    int m_orientation = -1;
    int m_frameOrientation = -1;
    bool m_frameMirrored = false;

    QVideoFrameFormat m_format;
    QSGVideoMaterial *m_material = nullptr;

    QVideoTextureHelper::SubtitleLayout m_subtitleLayout;
    QQuickTextNode *m_subtitleTextNode = nullptr;
};

QT_END_NAMESPACE

#endif // QSGVIDEONODE_H
