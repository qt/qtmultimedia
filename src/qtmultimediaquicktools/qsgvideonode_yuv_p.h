/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QSGVIDEONODE_YUV_H
#define QSGVIDEONODE_YUV_H

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

#include <private/qsgvideonode_p.h>
#include <QtMultimedia/qvideosurfaceformat.h>

QT_BEGIN_NAMESPACE

class QSGVideoMaterial_YUV;
class QSGVideoNode_YUV : public QSGVideoNode
{
public:
    QSGVideoNode_YUV(const QVideoSurfaceFormat &format);
    ~QSGVideoNode_YUV();

    QVideoFrame::PixelFormat pixelFormat() const override {
        return m_format.pixelFormat();
    }
    QAbstractVideoBuffer::HandleType handleType() const override {
        return QAbstractVideoBuffer::NoHandle;
    }
    void setCurrentFrame(const QVideoFrame &frame, FrameFlags flags) override;

private:
    void bindTexture(int id, int unit, int w, int h, const uchar *bits);

    QVideoSurfaceFormat m_format;
    QSGVideoMaterial_YUV *m_material;
};

class QSGVideoNodeFactory_YUV : public QSGVideoNodeFactoryInterface {
public:
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const override;
    QSGVideoNode *createNode(const QVideoSurfaceFormat &format) override;
};

QT_END_NAMESPACE

#endif // QSGVIDEONODE_YUV_H
