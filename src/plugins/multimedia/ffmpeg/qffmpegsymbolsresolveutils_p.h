// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGSYMBOLSRESOLVEUTILS_P_H
#define QFFMPEGSYMBOLSRESOLVEUTILS_P_H

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

#include <QtCore/qlibrary.h>

#include <qatomic.h>

#include <vector>
#include <memory>
#include <tuple>

QT_BEGIN_NAMESPACE

using Libs = std::vector<std::unique_ptr<QLibrary>>;

class LibSymbolsResolver
{
public:
    using FunctionSetter = void (*)(QFunctionPointer);
    using LibsLoader = Libs (*)();

    LibSymbolsResolver(const char *libName, size_t symbolsCount, LibsLoader libsLoader);

    bool resolve();

    void registerSymbol(const char *name, FunctionSetter setter);

    static void unload(const Libs &libs);

    static bool tryLoad(const Libs &libs);

private:
    static QFunctionPointer resolve(const Libs &libs, const char *symbolName);

private:
    const char *const m_libName;
    LibsLoader m_libsLoader;

    struct SymbolElement
    {
        const char *name;
        FunctionSetter setter;
    };

    std::vector<SymbolElement> m_symbols;

    enum State { Initial, Requested, Ready, Finished };

    QAtomicInteger<int> m_state = Initial;
};

QT_END_NAMESPACE

template <typename T>
struct DefaultReturn
{
    template <typename... Arg>
    T operator()(Arg &&...) { return val; }
    T val;
};

template <>
struct DefaultReturn<void>
{
    DefaultReturn(int = 0){};
    template <typename... Arg>
    void operator()(Arg &&...) { }
};

template <typename...>
struct FuncInfo;

template <typename R, typename... A>
struct FuncInfo<R(A...)>
{
    using Return = R;
    using Args = std::tuple<A...>;
};

// clang-format off

#define DEFINE_FUNC_IMPL(F, Vars, TypesWithVars, ReturnFunc) \
    using F##_ReturnType = FuncInfo<decltype(F)>::Return; \
    using q_##F##_Type = F##_ReturnType (*)(TypesWithVars(F)); \
    static q_##F##_Type q_##F = []() { \
        auto setter = [](QFunctionPointer ptr) { q_##F = (q_##F##_Type)ptr; }; \
        resolver()->registerSymbol(#F, setter); \
        return [](TypesWithVars(F)) { return ReturnFunc(Vars()); }; \
    }(); \
    extern "C" [[maybe_unused]] F##_ReturnType F(TypesWithVars(F)) { return q_##F(Vars()); }

#define VAR(I) a##I
#define VARS0()
#define VARS1() VAR(0)
#define VARS2() VARS1(), VAR(1)
#define VARS3() VARS2(), VAR(2)
#define VARS4() VARS3(), VAR(3)
#define VARS5() VARS4(), VAR(4)
#define VARS6() VARS5(), VAR(5)
#define VARS7() VARS6(), VAR(6)
#define VARS8() VARS7(), VAR(7)
#define VARS9() VARS8(), VAR(8)
#define VARS10() VARS9(), VAR(9)
#define VARS11() VARS10(), VAR(10)

#define TYPE_WITH_VAR(F, I) std::tuple_element_t<I, FuncInfo<decltype(F)>::Args> VAR(I)
#define TYPES_WITH_VARS0(F)
#define TYPES_WITH_VARS1(F) TYPE_WITH_VAR(F, 0)
#define TYPES_WITH_VARS2(F) TYPES_WITH_VARS1(F), TYPE_WITH_VAR(F, 1)
#define TYPES_WITH_VARS3(F) TYPES_WITH_VARS2(F), TYPE_WITH_VAR(F, 2)
#define TYPES_WITH_VARS4(F) TYPES_WITH_VARS3(F), TYPE_WITH_VAR(F, 3)
#define TYPES_WITH_VARS5(F) TYPES_WITH_VARS4(F), TYPE_WITH_VAR(F, 4)
#define TYPES_WITH_VARS6(F) TYPES_WITH_VARS5(F), TYPE_WITH_VAR(F, 5)
#define TYPES_WITH_VARS7(F) TYPES_WITH_VARS6(F), TYPE_WITH_VAR(F, 6)
#define TYPES_WITH_VARS8(F) TYPES_WITH_VARS7(F), TYPE_WITH_VAR(F, 7)
#define TYPES_WITH_VARS9(F) TYPES_WITH_VARS8(F), TYPE_WITH_VAR(F, 8)
#define TYPES_WITH_VARS10(F) TYPES_WITH_VARS9(F), TYPE_WITH_VAR(F, 9)
#define TYPES_WITH_VARS11(F) TYPES_WITH_VARS10(F), TYPE_WITH_VAR(F, 10)


#define RET(F, ...) DefaultReturn<FuncInfo<decltype(F)>::Return>{__VA_ARGS__}

#define DEFINE_FUNC(F, ArgsCount, /*Return value*/...) \
    DEFINE_FUNC_IMPL(F, VARS##ArgsCount, TYPES_WITH_VARS##ArgsCount, RET(F, __VA_ARGS__));

// clang-format on

#endif // QFFMPEGSYMBOLSRESOLVEUTILS_P_H
