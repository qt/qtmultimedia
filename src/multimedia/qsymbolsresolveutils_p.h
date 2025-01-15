// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef Q_SYMBOLSRESOLVEUTILS
#define Q_SYMBOLSRESOLVEUTILS

#include <QtCore/qlibrary.h>
#include <QtMultimedia/qtmultimediaexports.h>
#include <tuple>
#include <memory>

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

QT_BEGIN_NAMESPACE

constexpr bool areVersionsEqual(const char lhs[], const char rhs[])
{
    int i = 0;
    for (; lhs[i] && rhs[i]; ++i)
        if (lhs[i] != rhs[i])
            return false;
    return lhs[i] == rhs[i];
}

constexpr bool areVersionsEqual(const char lhs[], int rhsInt)
{
    int lhsInt = 0;
    for (int i = 0; lhs[i]; ++i) {
        if (lhs[i] < '0' || lhs[i] > '9')
            return false;

        lhsInt *= 10;
        lhsInt += lhs[i] - '0';
    }

    return lhsInt == rhsInt;
}


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

class Q_MULTIMEDIA_EXPORT SymbolsResolver
{
public:
    using LibraryLoader = std::unique_ptr<QLibrary> (*)();
    static bool isLazyLoadEnabled();
    bool isLoaded() const { return m_library != nullptr; }

    ~SymbolsResolver();
protected:
    SymbolsResolver(const char *libLoggingName, LibraryLoader loader);

    SymbolsResolver(const char *libName, const char *version = "",
                    const char *libLoggingName = nullptr);

    QFunctionPointer initOptionalFunction(const char *name);
    QFunctionPointer initFunction(const char *name);

    struct SymbolsMarker {};
    void checkLibrariesLoaded(SymbolsMarker *begin, SymbolsMarker *end);

private:
    const char *m_libLoggingName;
    std::unique_ptr<QLibrary> m_library;
};


QT_END_NAMESPACE

// clang-format off

#define CHECK_VERSIONS(Name, NeededSoversion, DetectedVersion) \
    static_assert(areVersionsEqual(NeededSoversion, DetectedVersion), \
                 "Configuartion error: misleading " Name " versions!")

#define BEGIN_INIT_FUNCS(...) \
    QT_USE_NAMESPACE \
    namespace { \
        class SymbolsResolverImpl : public SymbolsResolver { \
        public: \
            SymbolsResolverImpl() : SymbolsResolver(__VA_ARGS__) \
            { checkLibrariesLoaded(&symbolsBegin, &symbolsEnd); } \
            static const SymbolsResolverImpl& instance() \
            { static const SymbolsResolverImpl instance; return instance; } \
            SymbolsMarker symbolsBegin;

#define INIT_FUNC(F) QFunctionPointer F = initFunction(#F);
#define INIT_OPT_FUNC(F) QFunctionPointer F = initOptionalFunction(#F);

#define END_INIT_FUNCS() \
           SymbolsMarker symbolsEnd; \
        }; \
        [[maybe_unused]] static const auto *instantResolver = \
            SymbolsResolver::isLazyLoadEnabled() ? &SymbolsResolverImpl::instance() : nullptr; \
    }


#ifdef Q_EXPORT_STUB_SYMBOLS
#define EXPORT_FUNC Q_MULTIMEDIA_EXPORT
#else
#define EXPORT_FUNC
#endif

#define DEFINE_FUNC_IMPL(F, Vars, TypesWithVars, ReturnFunc) \
    using F##_ReturnType = FuncInfo<decltype(F)>::Return; \
    extern "C" EXPORT_FUNC [[maybe_unused]] F##_ReturnType F(TypesWithVars(F)) { \
        using F##_Type = F##_ReturnType (*)(TypesWithVars(F)); \
        const auto f = SymbolsResolverImpl::instance().F; \
        return f ? (reinterpret_cast<F##_Type>(f))(Vars()) : ReturnFunc(); \
    }


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

#define DEFINE_IS_LOADED_CHECKER(FuncName) \
QT_BEGIN_NAMESPACE \
bool FuncName() { return SymbolsResolverImpl::instance().isLoaded(); } \
QT_END_NAMESPACE

#define DECLARE_IS_LOADED_CHECKER(FuncName) \
QT_BEGIN_NAMESPACE \
bool FuncName(); \
QT_END_NAMESPACE

// clang-format on

#endif // Q_SYMBOLSRESOLVEUTILS
