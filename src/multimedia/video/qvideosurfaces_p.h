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

#ifndef QVIDEOSURFACES_P_H
#define QVIDEOSURFACES_P_H

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

#include <QAbstractVideoSurface>
#include <QVector>

QT_BEGIN_NAMESPACE

class QVideoSurfaces : public QAbstractVideoSurface
{
public:
    QVideoSurfaces(const QVector<QAbstractVideoSurface *> &surfaces, QObject *parent = nullptr);
    ~QVideoSurfaces();

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const override;
    bool start(const QVideoSurfaceFormat &format) override;
    void stop() override;
    bool present(const QVideoFrame &frame) override;

private:
    QVector<QAbstractVideoSurface *> m_surfaces;
    Q_DISABLE_COPY(QVideoSurfaces)
};

QT_END_NAMESPACE

#endif // QVIDEOSURFACES_P_H
