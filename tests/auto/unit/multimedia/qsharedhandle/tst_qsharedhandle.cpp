// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qsharedhandle_p.h>

#include <QTest>

// NOLINTBEGIN(readability-convert-member-functions-to-static)

QT_USE_NAMESPACE

struct MockResource
{
    int id{};

    friend bool comparesEqual(const MockResource &lhs, const MockResource &rhs) noexcept
    {
        return lhs.id == rhs.id;
    }

    friend Qt::strong_ordering compareThreeWay(const MockResource &lhs,
                                               const MockResource &rhs) noexcept
    {
        return qCompareThreeWay(lhs.id, rhs.id);
    }

    Q_DECLARE_STRONGLY_ORDERED(MockResource)
};

struct GlobalResource
{
    void reset()
    {
        m_openResourceRefCounts.clear();
        m_allocator = 0;
    }

    MockResource open()
    {
        return MockResource{
            m_allocator++,
        };
    }

    MockResource openAndRef() { return ref(open()); }

    MockResource ref(MockResource handle)
    {
        if (m_openResourceRefCounts.find(handle.id) != m_openResourceRefCounts.end())
            ++m_openResourceRefCounts[handle.id];
        else
            m_openResourceRefCounts[handle.id] = 1;
        return handle;
    }

    bool unref(MockResource handle)
    {
        QTEST_ASSERT(m_openResourceRefCounts.find(handle.id) != m_openResourceRefCounts.end());
        auto record = m_openResourceRefCounts.find(handle.id);
        record->second--;
        if (record->second == 0)
            m_openResourceRefCounts.erase(record);
        return true;
    }

    int refcount(const MockResource &handle)
    {
        if (auto it = m_openResourceRefCounts.find(handle.id); it != m_openResourceRefCounts.end())
            return it->second;
        return 0;
    }

    int openResourceCount() const { return int(m_openResourceRefCounts.size()); }

    std::map<int, int> m_openResourceRefCounts;
    int m_allocator = 0;
};

static GlobalResource g_resource;

static constexpr MockResource invalidResource{
    -1,
};

struct MockResourceTraits
{
    using Type = MockResource;

    static Type invalidValue() noexcept { return invalidResource; }

    static Type ref(Type handle) noexcept { return g_resource.ref(handle); }
    static bool unref(Type handle) noexcept { return g_resource.unref(handle); }
};

using Handle = QtPrivate::QSharedHandle<MockResourceTraits>;

class tst_QSharedHandle : public QObject
{
    Q_OBJECT

    static int refcount(const Handle &handle) { return g_resource.refcount(handle.get()); }
    Handle makeHandle() const
    {
        return Handle{
            g_resource.openAndRef(),
            Handle::HasRef,
        };
    }

private slots:

    void init() const { g_resource.reset(); }

    void cleanup() const { g_resource.reset(); }

    void defaultConstructor_initializesToInvalidHandle() const
    {
        const Handle h;
        QCOMPARE_EQ(h.get(), MockResourceTraits::invalidValue());
    }

    void constructor_initializesToValid_whenCalledWithValidHandle() const
    {
        const auto res = g_resource.openAndRef();
        const Handle h{ res, Handle::HasRef };
        QCOMPARE_EQ(h.get(), res);
        QCOMPARE_EQ(refcount(h), 1);
    }

    void constructor_withNeedsRefIncreasesRefcount() const
    {
        const auto res = g_resource.open();
        const Handle h{ res, Handle::NeedsRef };
        QCOMPARE_EQ(h.get(), res);
        QCOMPARE_EQ(refcount(h), 1);
    }

    void copyConstructor_increaseRefcount() const
    {
        const auto res = g_resource.openAndRef();
        const Handle h{ res, Handle::HasRef };
        const Handle h2(h);
        QCOMPARE_EQ(refcount(h), 2);
        QCOMPARE_EQ(refcount(h2), 2);
    }

    void moveConstructor_movesOwnershipAndResetsSource() const
    {
        Handle source = makeHandle();
        const Handle dest{ std::move(source) };

        QVERIFY(!source.isValid());
        QVERIFY(dest.isValid());
        QCOMPARE_EQ(refcount(dest), 1);
    }

    void moveAssignment_movesOwnershipAndResetsSource() const
    {
        Handle source = makeHandle();
        Handle dest;
        dest = { std::move(source) };

        QVERIFY(!source.isValid());
        QVERIFY(dest.isValid());
        QCOMPARE_EQ(refcount(dest), 1);
    }

    void moveAssignment_maintainsOwnershipWhenSelfAssigning() const
    {
        Handle source = makeHandle();

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_CLANG("-Wself-move")
        QT_WARNING_DISABLE_GCC("-Wself-move")
        source = std::move(source); // NOLINT(clang-diagnostic-self-move)
        QT_WARNING_POP

        QVERIFY(source.isValid());
        QCOMPARE_EQ(refcount(source), 1);
    }

    void isValid_returnsFalse_onlyWhenHandleIsInvalid() const
    {
        const Handle invalid;
        QVERIFY(!invalid.isValid());

        const Handle valid = makeHandle();
        QVERIFY(valid.isValid());
    }

    void reset_resetsHandle() const
    {
        const Handle reference = makeHandle();
        Handle dut = reference;
        QCOMPARE_EQ(refcount(reference), 2);

        dut.reset(g_resource.openAndRef(), Handle::HasRef);

        QCOMPARE_EQ(refcount(reference), 1);
    }

    void reset_toNull_resetsSource() const
    {
        Handle dut = makeHandle();
        QCOMPARE_EQ(refcount(dut), 1);
        dut.reset();
        QVERIFY(!dut);

        QCOMPARE_EQ(refcount(dut), 0);
    }

    void destructor_callsClose_whenHandleIsValid()
    {
        {
            const Handle h0 = makeHandle();
            const Handle h1 = makeHandle();
            const Handle h2 = makeHandle();
            QCOMPARE(g_resource.openResourceCount(), 3);
        }

        QCOMPARE(g_resource.openResourceCount(), 0);
    }

    void operatorBool_returnsFalse_onlyWhenHandleIsInvalid() const
    {
        const Handle invalid;
        QVERIFY(!invalid);

        const Handle valid = makeHandle();
        QVERIFY(valid);
    }

    void get_returnsValue() const
    {
        const Handle invalid;
        QCOMPARE_EQ(invalid.get(), invalidResource);

        const auto resource = g_resource.openAndRef();
        const Handle valid{ resource, Handle::HasRef };
        QCOMPARE_EQ(valid.get(), resource);
    }

    void release_returnsInvalidResource_whenCalledOnInvalidHandle() const
    {
        Handle h;
        QCOMPARE_EQ(h.release(), invalidResource);
    }

    void release_releasesOwnershipAndReturnsResource_whenHandleOwnsObject() const
    {
        Handle resource = makeHandle();
        Handle released{};
        {
            Handle h{ resource };
            released = Handle{ h.release(), Handle::HasRef };
            QCOMPARE_EQ(refcount(h), 0);
        }
        QCOMPARE_EQ(refcount(resource), 2);
        QCOMPARE_EQ(resource, released);
    }

    void swap_swapsOwnership() const
    {
         { // Swapping valid and invalid handle
             Handle h0{ g_resource.open(), Handle::NeedsRef };
             Handle h1;

             h0.swap(h1);

             QVERIFY(!h0.isValid());
             QVERIFY(h1.isValid());
        }
        { // Swapping valid handles
            const auto resource0 = g_resource.open();
            const auto resource1 = g_resource.open();

            Handle h0{ resource0, Handle::NeedsRef };
            Handle h1{ resource1, Handle::NeedsRef };

            h0.swap(h1);

            QCOMPARE_EQ(h0.get(), resource1);
            QCOMPARE_EQ(h1.get(), resource0);
        }
        { // std::swap
            const auto resource0 = g_resource.open();
            const auto resource1 = g_resource.open();

            Handle h0{ resource0, Handle::NeedsRef };
            Handle h1{ resource1, Handle::NeedsRef };

            std::swap(h0, h1);

            QCOMPARE_EQ(h0.get(), resource1);
            QCOMPARE_EQ(h1.get(), resource0);
        }
        { // swap
            const auto resource0 = g_resource.open();
            const auto resource1 = g_resource.open();

            Handle h0{ resource0, Handle::NeedsRef };
            Handle h1{ resource1, Handle::NeedsRef };

            swap(h0, h1);

            QCOMPARE_EQ(h0.get(), resource1);
            QCOMPARE_EQ(h1.get(), resource0);
        }
    }

    void comparison() const
    {
        Handle handle0 = makeHandle();
        Handle handle1 = makeHandle();
        Handle handle2 = makeHandle();
        QVERIFY(handle0.get().id == 0);
        QVERIFY(handle1.get().id == 1);
        QVERIFY(handle2.get().id == 2);

        QVERIFY(handle1 == handle1);
        QVERIFY(handle2 > handle1);
        QVERIFY(handle1 >= handle1);
        QVERIFY(handle1 != handle0);
        QVERIFY(handle0 < handle1);
        QVERIFY(handle0 <= handle1);
    }

    void addressOf_returnsAddressOfHandle() const
    {
        Handle h;
        auto fn = [&](MockResource *ptr) {
            QCOMPARE_EQ((void *)ptr, (void *)&h);
            *ptr = g_resource.openAndRef();
        };

        fn(&h);

        QVERIFY(h.isValid());
    }
};

QTEST_MAIN(tst_QSharedHandle)
#include "tst_qsharedhandle.moc"
