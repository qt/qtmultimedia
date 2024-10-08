// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcominitializer_p.h"

#include <QtCore/private/qfunctions_win_p.h>
#include <memory>

QT_BEGIN_NAMESPACE

namespace {

// Destroys QComHelper on thread exit, thereby calling CoUninitialize
thread_local std::unique_ptr<QComHelper> s_comHelperRegistry;

}

// Initializes COM as a single-threaded apartment on this thread and
// ensures that CoUninitialize will be called on the same thread when
// the thread exits. Note that the last call to CoUninitialize on the
// main thread will always be made during destruction of static
// variables at process exit.
void ensureComInitializedOnThisThread() {
    if (!s_comHelperRegistry)
        s_comHelperRegistry = std::make_unique<QComHelper>();
}

QT_END_NAMESPACE
