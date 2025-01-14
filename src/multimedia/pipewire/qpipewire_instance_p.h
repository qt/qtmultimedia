// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_INSTANCE_P_H
#define QPIPEWIRE_INSTANCE_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qloggingcategory.h>

#include <pipewire/pipewire.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_DECLARE_LOGGING_CATEGORY(lcPipewire); // "qt.multimedia.pipewire"

class QPipeWireInstance
{
public:
    [[nodiscard]] static std::shared_ptr<QPipeWireInstance> instance();

    QPipeWireInstance();
    ~QPipeWireInstance();
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_INSTANCE_P_H
