// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsymbolsresolveutils_p.h"

#include <qdebug.h>
#include <algorithm>
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcSymbolsResolver, "qt.multimedia.symbolsresolver");

bool SymbolsResolver::isLazyLoadEnabled()
{
    static const bool lazyLoad =
            !static_cast<bool>(qEnvironmentVariableIntValue("QT_INSTANT_LOAD_FFMPEG_STUBS"));
    return lazyLoad;
}

SymbolsResolver::SymbolsResolver(const char *libLoggingName, LibraryLoader loader)
    : m_libLoggingName(libLoggingName)
{
    Q_ASSERT(libLoggingName);
    Q_ASSERT(loader);

    auto library = loader();
    if (library && library->isLoaded())
        m_library = std::move(library);
    else
        qCWarning(qLcSymbolsResolver) << "Couldn't load" << m_libLoggingName << "library";
}

SymbolsResolver::SymbolsResolver(const char *libName, const char *version,
                                 const char *libLoggingName)
    : m_libLoggingName(libLoggingName ? libLoggingName : libName)
{
    Q_ASSERT(libName);
    Q_ASSERT(version);

    auto library = std::make_unique<QLibrary>(QString::fromLocal8Bit(libName),
                                              QString::fromLocal8Bit(version));
    if (library->load())
        m_library = std::move(library);
    else
        qCWarning(qLcSymbolsResolver) << "Couldn't load" << m_libLoggingName << "library";
}

SymbolsResolver::~SymbolsResolver()
{
    if (m_library)
        m_library->unload();
}

QFunctionPointer SymbolsResolver::initOptionalFunction(const char *funcName)
{
    return m_library ? m_library->resolve(funcName) : nullptr;
}

QFunctionPointer SymbolsResolver::initFunction(const char *funcName)
{
    QFunctionPointer func = initOptionalFunction(funcName);

    if (!func && m_library)
    {
        qCWarning(qLcSymbolsResolver) << "Couldn't resolve" << m_libLoggingName << "symbol" << funcName;
        m_library->unload();
        m_library.reset();
    }

    return func;
}

void SymbolsResolver::checkLibrariesLoaded(SymbolsMarker *begin, SymbolsMarker *end)
{
    if (m_library) {
        qCDebug(qLcSymbolsResolver) << m_libLoggingName << "symbols resolved";
    } else {
        const auto size = reinterpret_cast<char *>(end) - reinterpret_cast<char *>(begin);
        memset(begin, 0, size);
        qCWarning(qLcSymbolsResolver) << "Couldn't resolve" << m_libLoggingName << "symbols";
    }
}

QT_END_NAMESPACE
