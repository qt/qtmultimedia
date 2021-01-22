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

#ifndef QABSTRACTVIDEOBUFFER_P_H
#define QABSTRACTVIDEOBUFFER_P_H

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

#include <QtCore/qshareddata.h>
#include "qabstractvideobuffer.h"

#include <qtmultimediaglobal.h>
#include <qmultimedia.h>


QT_BEGIN_NAMESPACE

class QAbstractVideoBufferPrivate
{
public:
    QAbstractVideoBufferPrivate()
        : q_ptr(nullptr)
    {}

    virtual ~QAbstractVideoBufferPrivate()
    {}

    virtual int map(
            QAbstractVideoBuffer::MapMode mode,
            int *numBytes,
            int bytesPerLine[4],
            uchar *data[4]);

    QAbstractVideoBuffer *q_ptr;
};

class QAbstractPlanarVideoBufferPrivate : QAbstractVideoBufferPrivate
{
public:
    QAbstractPlanarVideoBufferPrivate()
    {}

    int map(QAbstractVideoBuffer::MapMode mode, int *numBytes, int bytesPerLine[4], uchar *data[4]) override;

private:
    Q_DECLARE_PUBLIC(QAbstractPlanarVideoBuffer)
};

QT_END_NAMESPACE


#endif
