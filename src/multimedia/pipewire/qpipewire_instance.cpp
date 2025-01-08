// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_instance_p.h"

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_LOGGING_CATEGORY(lcPipewire, "qt.multimedia.pipewire");

Q_GLOBAL_STATIC(QPipeWireInstance, s_instance);

QPipeWireInstance *QPipeWireInstance::instance()
{
    return s_instance;
}

QPipeWireInstance::QPipeWireInstance()
{
    pw_init(nullptr, nullptr);

    qCDebug(lcPipewire) << "PipeWire initialized: compiled against" << pw_get_headers_version()
                        << " running " << pw_get_library_version();
}

QPipeWireInstance::~QPipeWireInstance()
{
    pw_deinit();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
