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

#ifndef QMEMORYVIDEOBUFFER_P_H
#define QMEMORYVIDEOBUFFER_P_H

#include <qabstractvideobuffer.h>

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


class QMemoryVideoBufferPrivate;

class Q_MULTIMEDIA_EXPORT QMemoryVideoBuffer : public QAbstractVideoBuffer
{
    Q_DECLARE_PRIVATE(QMemoryVideoBuffer)
public:
    QMemoryVideoBuffer(const QByteArray &data, int bytesPerLine);
    ~QMemoryVideoBuffer();

    MapMode mapMode() const override;

    uchar *map(MapMode mode, int *numBytes, int *bytesPerLine) override;
    void unmap() override;
};

QT_END_NAMESPACE


#endif
