/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
    int m_orientation;
    int m_frameOrientation;
    bool m_frameMirrored;

    QVideoFrameFormat m_format;
    QSGVideoMaterial *m_material;

    QVideoTextureHelper::SubtitleLayout m_subtitleLayout;
    QQuickTextNode *m_subtitleTextNode = nullptr;
};

QT_END_NAMESPACE

#endif // QSGVIDEONODE_H
