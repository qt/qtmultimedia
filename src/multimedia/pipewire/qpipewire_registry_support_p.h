// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_REGISTRY_SUPPORT_P_H
#define QPIPEWIRE_REGISTRY_SUPPORT_P_H

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

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qglobal.h>

#include <pipewire/pipewire.h>

#include <optional>
#include <string_view>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

enum class PipewireRegistryType : uint8_t
{
    Client,
    Core,
    Device,
    Factory,
    Link,
    Metadata,
    Module,
    Node,
    Port,
    Profiler,
    Registry,
    SecurityContext,
};

Q_MULTIMEDIA_EXPORT std::optional<PipewireRegistryType> parsePipewireRegistryType(std::string_view sv);

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_REGISTRY_SUPPORT_P_H
