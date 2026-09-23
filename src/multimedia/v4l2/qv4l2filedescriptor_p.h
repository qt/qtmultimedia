// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4L2FILEDESCRIPTOR_P_H
#define QV4L2FILEDESCRIPTOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/private/quniquehandle_types_p.h>
#include <QtCore/qtclasshelpermacros.h>

#include <linux/videodev2.h>

QT_BEGIN_NAMESPACE

namespace QtV4L2 {

class QV4L2FileDescriptor : public QUniqueFileDescriptorHandle
{
public:
    explicit QV4L2FileDescriptor(QUniqueFileDescriptorHandle descriptor);

    Q_DISABLE_COPY_MOVE(QV4L2FileDescriptor)

    bool call(int request, void *arg) const;

    bool requestBuffers(v4l2_memory memoryType, quint32 &buffersCount) const;

    bool startStream();

    bool stopStream();

    bool streamStarted() const { return m_streamStarted; }

private:
    bool m_streamStarted = false;
};

} // namespace QtV4L2

QT_END_NAMESPACE

#endif // QV4L2FILEDESCRIPTOR_P_H
