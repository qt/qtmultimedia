/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGVIDEONODE_P_H
#define QSGVIDEONODE_P_H

#include <qsgnode.h>

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideosurfaceformat.h>
#include <QtOpenGL/qglfunctions.h>

class QSGVideoNode : public QSGGeometryNode
{
public:
    QSGVideoNode();

    virtual void setCurrentFrame(const QVideoFrame &frame) = 0;
    virtual QVideoFrame::PixelFormat pixelFormat() const = 0;

    void setTexturedRectGeometry(const QRectF &boundingRect, const QRectF &textureRect, int orientation);

private:
    QRectF m_rect;
    QRectF m_textureRect;
    int m_orientation;
};

class QSGVideoNodeFactory {
public:
    virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const = 0;
    virtual QSGVideoNode *createNode(const QVideoSurfaceFormat &format) = 0;
};

#endif // QSGVIDEONODE_H
