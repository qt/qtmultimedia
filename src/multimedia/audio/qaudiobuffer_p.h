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

#ifndef QAUDIOBUFFER_P_H
#define QAUDIOBUFFER_P_H

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

#include <qtmultimediaglobal.h>
#include <qmultimedia.h>

#include "qaudioformat.h"

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QAbstractAudioBuffer {
public:
    virtual ~QAbstractAudioBuffer() {}

    // Lifetime management
    virtual void release() = 0;

    // Format related
    virtual QAudioFormat format() const = 0;
    virtual qint64 startTime() const = 0;
    virtual int frameCount() const = 0;

    // R/O Data
    virtual void *constData() const = 0;

    // For writable data we do this:
    // If we only have one reference to the provider,
    // call writableData().  If that does not return 0,
    // then we're finished.  If it does return 0, then we call
    // writableClone() to get a new buffer and then release
    // the old clone if that succeeds.  If it fails, we create
    // a memory clone from the constData and release the old buffer.
    // If writableClone() succeeds, we then call writableData() on it
    // and that should be good.

    virtual void *writableData() = 0;
    virtual QAbstractAudioBuffer *clone() const = 0;
};


QT_END_NAMESPACE

#endif // QAUDIOBUFFER_P_H
