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

#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/private/quniquehandle_types_p.h>
#include <QtCore/private/qexpected_p.h>

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <linux/videodev2.h>

#include <system_error>

QT_BEGIN_NAMESPACE

namespace QtV4L2 {

class QV4L2FileDescriptor : public QUniqueFileDescriptorHandle
{
public:
    explicit QV4L2FileDescriptor(QUniqueFileDescriptorHandle descriptor);

    Q_DISABLE_COPY_MOVE(QV4L2FileDescriptor)

    q23::expected<int, std::error_code> call(int request, void *arg) const;

    q23::expected<int, std::error_code> requestBuffers(v4l2_memory memoryType,
                                                       uint32_t bufferCount) const;

    bool startStream();

    bool stopStream();

    bool streamStarted() const { return m_streamStarted; }

private:
    bool m_streamStarted = false;
};

} // namespace QtV4L2

QT_END_NAMESPACE

#endif // QV4L2FILEDESCRIPTOR_P_H
