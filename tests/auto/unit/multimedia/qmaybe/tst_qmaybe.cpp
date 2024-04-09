// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <private/qmaybe_p.h>
#include <QtCore/private/quniquehandle_p.h>
#ifdef Q_OS_WINDOWS
#include <private/qcomptr_p.h>
#include <private/qcomobject_p.h>
#endif

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

namespace {

// Helpers used to verify interop with QUniqueHandle and ComPtr which
// overloads operator&
struct DummyHandleTraits
{
    using Type = int;
    static Type invalidValue() { return -1; }
    static bool close(Type /*handle*/) { return true; }
};

using DummyHandle = QUniqueHandle<DummyHandleTraits>;

#ifdef Q_OS_WINDOWS
struct DummyComObject : QComObject<IUnknown>
{
};
#endif

} // namespace

//clang-format off

class tst_QMaybe : public QObject
{
    Q_OBJECT

private slots:
    void operatorBool_returnsFalse_onlyWhenErrorSet()
    {
        {
            const QMaybe<QString, int> error{ -1 }; // TOOD: Is it safe to deduce expected/unexpected only based on type?
            QVERIFY(!static_cast<bool>(error));
        }

        {
            const QMaybe<QString, int> success{ "It worked!"_L1 };
            QVERIFY(static_cast<bool>(success));
        }
    }

    void value_returnsReferenceToValue_whenValueSet()
    {
        {
            QMaybe mutableVal{ 1 };
            mutableVal.value() = 2;
            QCOMPARE_EQ(*mutableVal, 2); // value() must have returned a mutable reference
        }

        {
            const QMaybe immutableVal{ 2 };
            QCOMPARE_EQ(*std::addressof(immutableVal.value()), 2); // value() must have returned a reference
            static_assert(std::is_const_v<std::remove_reference_t<decltype(immutableVal.value())>>); // And it is const
        }
    }

    void dereferenceOperator_returnsPointerToValue_whenValueTypeOverloadsAddressOfOperator()
    {
        {
            QMaybe<DummyHandle, int> mutableValue{ DummyHandle{ 1 } };
            QCOMPARE_EQ(mutableValue->get(), 1);
            QVERIFY(mutableValue->isValid()); // We did not accidentally call operator& that resets
                                              // QUniqueHandle
        }

        {
            const QMaybe<DummyHandle, int> immutableValue{ DummyHandle{ 2 } };
            QCOMPARE_EQ(immutableValue->get(), 2);
            QVERIFY(immutableValue->isValid()); // We did not accidentally call operator& that
                                                // resets QUniqueHandle
        }
    }

#ifdef Q_OS_WINDOWS
    void dereferenceOperator_returnsPointerToValue_whenValueIsComPtr()
    {
        // Similar test as with QUniqueHandle, but with ComPtr that is used
        // frequently on Windows and may behave slightly differently

        {
            QMaybe<ComPtr<DummyComObject>, HRESULT> mutableObject{
                makeComObject<DummyComObject>()
            };
            QCOMPARE_NE(mutableObject->Get(), nullptr);

            const ComPtr<IUnknown> unknownFromMutable = mutableObject.value();
            QVERIFY(unknownFromMutable); // We did not accidentally call operator& that resets
                                         // QUniqueHandle
        }

        {
            QMaybe<ComPtr<DummyComObject>, HRESULT> immutableObject{
                makeComObject<DummyComObject>()
            };
            QCOMPARE_NE(immutableObject->Get(), nullptr);

            const ComPtr<IUnknown> unknownFromImmutable = immutableObject.value();
            QVERIFY(unknownFromImmutable); // We did not accidentally call operator& that resets
                                           // QUniqueHandle
        }
    }
#endif
};

QTEST_APPLESS_MAIN(tst_QMaybe)

#include "tst_qmaybe.moc"

//clang-format on
