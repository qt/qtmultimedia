// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef UDMABUFTESTUTILS_P_H
#define UDMABUFTESTUTILS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qbytearray.h>
#include <QtCore/private/quniquehandle_types_p.h>

QT_BEGIN_NAMESPACE

quint64 udmabufPageAlignedSize(quint64 size);

QUniqueFileDescriptorHandle createUdmabufTestMemfd(const QByteArray &data, quint64 bufferSize,
                                                   const char *debugName);

QUniqueFileDescriptorHandle createUdmabufFd(const QUniqueFileDescriptorHandle &memFd, quint64 size);

bool canUseUdmabuf();

QT_END_NAMESPACE

#endif // UDMABUFTESTUTILS_P_H
