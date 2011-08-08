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

#ifndef QSGVIDEONODE_I420_H
#define QSGVIDEONODE_I420_H

#include "qsgvideonode_p.h"
#include <QtMultimediaKit/qvideosurfaceformat.h>

class QSGVideoMaterial_YUV420;
class QSGVideoNode_I420 : public QSGVideoNode
{
public:
    QSGVideoNode_I420(const QVideoSurfaceFormat &format);
    ~QSGVideoNode_I420();

    virtual QVideoFrame::PixelFormat pixelFormat() const {
        return m_format.pixelFormat();
    }
    void setCurrentFrame(const QVideoFrame &frame);

private:
    void bindTexture(int id, int unit, int w, int h, const uchar *bits);

    int m_width;
    int m_height;
    GLuint m_id[3];

    QVideoSurfaceFormat m_format;
    QSGVideoMaterial_YUV420 *m_material;
    QVideoFrame m_frame;
};

class QSGVideoNodeFactory_I420 : public QSGVideoNodeFactory {
public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const;
    QSGVideoNode *createNode(const QVideoSurfaceFormat &format);
};


#endif // QSGVIDEONODE_I420_H
