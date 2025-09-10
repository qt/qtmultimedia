// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4L2FILEDESCRIPTOR_P_H
#define QV4L2FILEDESCRIPTOR_P_H

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

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

int xioctl(int fd, int request, void *arg);

class QV4L2FileDescriptor
{
public:
    QV4L2FileDescriptor(int descriptor);

    ~QV4L2FileDescriptor();

    bool call(int request, void *arg) const;

    int get() const { return m_descriptor; }

    bool requestBuffers(quint32 memoryType, quint32 &buffersCount) const;

    bool startStream();

    bool stopStream();

    bool streamStarted() const { return m_streamStarted; }

private:
    int m_descriptor;
    bool m_streamStarted = false;
};

QT_END_NAMESPACE

#endif // QV4L2FILEDESCRIPTOR_P_H
