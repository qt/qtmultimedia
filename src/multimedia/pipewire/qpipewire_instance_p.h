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

#include <QtMultimedia/private/qpipewire_support_p.h>

#include <QtCore/private/qexpected_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qloggingcategory.h>

#include <pipewire/pipewire.h>

#include <system_error>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_DECLARE_LOGGING_CATEGORY(lcPipewire); // "qt.multimedia.pipewire"

struct QPipewireInitializer
{
    QPipewireInitializer();
    ~QPipewireInitializer();
};

class QPipeWireInstance : private QPipewireInitializer
{
public:
    [[nodiscard]] static std::shared_ptr<QPipeWireInstance> instance();
    [[nodiscard]] static bool isLoaded();
    [[nodiscard]] static q23::expected<void, std::error_code> hasSPAFactory(const char *factoryName);

    // event loop
    template <typename Closure>
    auto runWithEventLoopLock(Closure &&c)
    {
        return m_eventLoop.runWithEventLoopLock(std::forward<Closure>(c));
    }

    auto eventLoopLock() { return std::unique_lock{ m_eventLoop }; }
    bool isInPwThreadLoop() const;
    pw_loop *eventLoop() const;
    PWThreadedEventLoop &pwEventLoop() { return m_eventLoop; };

    // context
    pw_context *context() const;

    ~QPipeWireInstance();

private:
    static std::unique_ptr<QPipeWireInstance> create();
    QPipeWireInstance();

    PWThreadedEventLoop m_eventLoop{ "PwEventLoop" };
    PwContextHandle m_context;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_INSTANCE_P_H
