// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimedia/private/qdmabufallocator_p.h>

#include <QtTest/qtest.h>

#include <sys/mman.h>
#include <unistd.h>

#include <cstring>

using namespace QtMultimediaPrivate;

class tst_QDmaBufAllocator : public QObject
{
    Q_OBJECT

private slots:
    void allocateAndAccess();
    void bufferOutlivesAllocator();
};

namespace {

// Maps the whole buffer, writes a pattern through the mapping and reads it back.
void checkBufferIsReadWrite(const QDmaBuf &buf)
{
    void *mapping =
            mmap(nullptr, size_t(buf.size()), PROT_READ | PROT_WRITE, MAP_SHARED, buf.fd(), 0);
    QCOMPARE_NE(mapping, MAP_FAILED);
    if (mapping == MAP_FAILED)
        return;

    auto *bytes = static_cast<unsigned char *>(mapping);
    std::memset(bytes, 0x5a, size_t(buf.size()));
    QCOMPARE_EQ(bytes[0], 0x5a);
    QCOMPARE_EQ(bytes[buf.size() - 1], 0x5a);

    QCOMPARE_EQ(munmap(mapping, size_t(buf.size())), 0);
}

} // namespace

void tst_QDmaBufAllocator::allocateAndAccess()
{
    auto allocator = QDmaBufAllocator::create();
    if (!allocator)
        QSKIP(qPrintable(allocator.error()));

    const qsizetype requested = 1000;
    auto buf = allocator->allocate(requested);
    QCOMPARE(buf.has_value(), true);

    QCOMPARE_GE((*buf)->size(), requested);
    QCOMPARE_EQ((*buf)->size() % sysconf(_SC_PAGESIZE), 0);
    QCOMPARE_GE((*buf)->fd(), 0);

    checkBufferIsReadWrite(**buf);
}

void tst_QDmaBufAllocator::bufferOutlivesAllocator()
{
    std::shared_ptr<QDmaBuf> buf;
    {
        auto allocator = QDmaBufAllocator::create();
        if (!allocator)
            QSKIP(qPrintable(allocator.error()));

        auto result = allocator->allocate(4096);
        QCOMPARE(result.has_value(), true);
        buf = *result;
    }

    checkBufferIsReadWrite(*buf);
}

QTEST_APPLESS_MAIN(tst_QDmaBufAllocator)

#include "tst_qdmabufallocator.moc"
