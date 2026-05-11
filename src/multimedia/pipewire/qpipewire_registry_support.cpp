// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_registry_support_p.h"

#include <QtCore/private/qflatmap_p.h>
#include <QtCore/qdebug.h>

#if __has_include(<pipewire/extensions/security-context.h>)
#  include <pipewire/extensions/security-context.h>
#else
#  define PW_TYPE_INTERFACE_SecurityContext PW_TYPE_INFO_INTERFACE_BASE "SecurityContext"
#endif
#include <pipewire/extensions/metadata.h>
#include <pipewire/extensions/profiler.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

std::optional<PipewireRegistryType> parsePipewireRegistryType(std::string_view sv)
{
    static QFlatMap<std::string_view, PipewireRegistryType> lookupTable{
        Qt::OrderedUniqueRange,
        {
                { PW_TYPE_INTERFACE_Client, PipewireRegistryType::Client },
                { PW_TYPE_INTERFACE_Core, PipewireRegistryType::Core },
                { PW_TYPE_INTERFACE_Device, PipewireRegistryType::Device },
                { PW_TYPE_INTERFACE_Factory, PipewireRegistryType::Factory },
                { PW_TYPE_INTERFACE_Link, PipewireRegistryType::Link },
                { PW_TYPE_INTERFACE_Metadata, PipewireRegistryType::Metadata },
                { PW_TYPE_INTERFACE_Module, PipewireRegistryType::Module },
                { PW_TYPE_INTERFACE_Node, PipewireRegistryType::Node },
                { PW_TYPE_INTERFACE_Port, PipewireRegistryType::Port },
                { PW_TYPE_INTERFACE_Profiler, PipewireRegistryType::Profiler },
                { PW_TYPE_INTERFACE_Registry, PipewireRegistryType::Registry },
                { PW_TYPE_INTERFACE_SecurityContext, PipewireRegistryType::SecurityContext },
        }
    };

    auto it = lookupTable.find(sv);
    if (it != lookupTable.end())
        return it.value();

    qWarning() << "unknown type" << sv;

    return std::nullopt;
}

} // namespace QtPipeWire

QT_END_NAMESPACE
