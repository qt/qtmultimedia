// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegsymbolsresolveutils_p.h"

#include <qdebug.h>
#include <algorithm>
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcLibSymbolsRelolver, "qt.multimedia.ffmpeg.libsymbolsresolver");

LibSymbolsResolver::LibSymbolsResolver(const char *libName, size_t symbolsCount,
                                       LibsLoader libsLoader)
    : m_libName(libName), m_libsLoader(libsLoader)
{
    Q_ASSERT(m_libName);
    Q_ASSERT(m_libsLoader);
    m_symbols.reserve(symbolsCount);
}

bool LibSymbolsResolver::resolve()
{
    if (m_state.testAndSetRelaxed(Initial, Requested)
        || !m_state.testAndSetAcquire(Ready, Finished))
        return false;

    qCDebug(qLcLibSymbolsRelolver)
            << "Start" << m_libName << "symbols resolving:" << m_symbols.size() << "symbols";

    Q_ASSERT(m_symbols.size() == m_symbols.capacity());

    auto cleanup = qScopeGuard([this]() { m_symbols = {}; });

    auto libs = m_libsLoader();
    if (libs.empty()) {
        qCWarning(qLcLibSymbolsRelolver) << "Couldn't load" << m_libName << "library";
        return false;
    }

    std::vector<QFunctionPointer> functions(m_symbols.size());

    auto resolveElement = [&libs](const SymbolElement &element) {
        return resolve(libs, element.name);
    };

    std::transform(m_symbols.begin(), m_symbols.end(), functions.begin(), resolveElement);

    if (std::find(functions.begin(), functions.end(), nullptr) != functions.end()) {
        unload(libs);
        qCWarning(qLcLibSymbolsRelolver) << "Couldn't resolve" << m_libName << "symbols";
        return false;
    }

    for (size_t i = 0; i < functions.size(); ++i)
        m_symbols[i].setter(functions[i]);

    qCDebug(qLcLibSymbolsRelolver) << m_libName << "symbols resolved";
    return true;
}

void LibSymbolsResolver::registerSymbol(const char *name, FunctionSetter setter)
{
    Q_ASSERT(setter);
    Q_ASSERT(m_symbols.size() < m_symbols.capacity());

    m_symbols.push_back({ name, setter });

    // handle the corner case: a user has initialized QtMM with global vars construction
    // and it happened before the symbols initializing
    if (m_symbols.size() == m_symbols.capacity() && !m_state.testAndSetRelease(Initial, Ready)
        && m_state.testAndSetRelease(Requested, Ready))
        resolve();
}

void LibSymbolsResolver::unload(const Libs &libs)
{
    for (auto &lib : libs)
        lib->unload();
}

bool LibSymbolsResolver::tryLoad(const Libs &libs)
{
    auto load = [](auto &lib) { return lib->load(); };
    if (std::all_of(libs.begin(), libs.end(), load))
        return true;

    unload(libs);
    return false;
}

QFunctionPointer LibSymbolsResolver::resolve(const Libs &libs, const char *symbolName)
{
    for (auto &lib : libs)
        if (auto pointer = lib->resolve(symbolName))
            return pointer;

    qWarning() << "Cannot resolve symbol" << symbolName;
    return nullptr;
}

QT_END_NAMESPACE
