// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIMAGEVIDEOBUFFER_P_H
#define QIMAGEVIDEOBUFFER_P_H

#include <private/qabstractvideobuffer_p.h>
#include <qimage.h>

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

QT_BEGIN_NAMESPACE

class Q_MULTIMEDIA_EXPORT QImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    QImageVideoBuffer(QImage image);

    QVideoFrame::MapMode mapMode() const override;

    MapData map(QVideoFrame::MapMode mode) override;

    void unmap() override;

private:
    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QImage m_image;
};

QT_END_NAMESPACE

#endif // QIMAGEVIDEOBUFFER_P_H
