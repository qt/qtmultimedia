// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4L2MEMORYTRANSFER_P_H
#define QV4L2MEMORYTRANSFER_P_H

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <qbytearray.h>
#include <linux/videodev2.h>

#include <memory>

QT_BEGIN_NAMESPACE

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

class QV4L2FileDescriptor;
using QV4L2FileDescriptorPtr = std::shared_ptr<QV4L2FileDescriptor>;

class QV4L2MemoryTransfer
{
public:
    struct Buffer
    {
        v4l2_buffer v4l2Buffer = {};
        QByteArray data;
    };

    QV4L2MemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor);

    virtual ~QV4L2MemoryTransfer();

    virtual std::optional<Buffer> dequeueBuffer() = 0;

    virtual bool enqueueBuffer(quint32 index) = 0;

    virtual quint32 buffersCount() const = 0;

protected:
    bool enqueueBuffers();

    const QV4L2FileDescriptor &fileDescriptor() const { return *m_fileDescriptor; }

private:
    QV4L2FileDescriptorPtr m_fileDescriptor;
};

using QV4L2MemoryTransferUPtr = std::unique_ptr<QV4L2MemoryTransfer>;

QV4L2MemoryTransferUPtr makeUserPtrMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor,
                                                  quint32 imageSize);

QV4L2MemoryTransferUPtr makeMMapMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor);

QT_END_NAMESPACE

#endif // QV4L2MEMORYTRANSFER_P_H
