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

#ifndef QIMAGEVIDEOBUFFER_P_H
#define QIMAGEVIDEOBUFFER_P_H

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

#include <qabstractvideobuffer.h>

QT_BEGIN_NAMESPACE


class QImage;

class QImageVideoBufferPrivate;

class Q_MULTIMEDIA_EXPORT QImageVideoBuffer : public QAbstractVideoBuffer
{
    Q_DECLARE_PRIVATE(QImageVideoBuffer)
public:
    QImageVideoBuffer(const QImage &image);
    ~QImageVideoBuffer();

    MapMode mapMode() const override;

    uchar *map(MapMode mode, int *numBytes, int *bytesPerLine) override;
    void unmap() override;
};

QT_END_NAMESPACE


#endif
