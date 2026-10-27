// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimedia/private/qdmabufallocator_p.h>
#include <QtMultimedia/private/qmultimedia_dmabuf_support_p.h>

#include <QtTest/qtest.h>

#include <sys/mman.h>
#include <unistd.h>

#include <cstring>

using namespace QtMultimediaPrivate;

class tst_QMultimediaDmaBufSupport : public QObject
{
    Q_OBJECT

private slots:
    void copiesRange_data();
    void copiesRange();
    void explicitBufferSizeMatchesFdSize();
    void rejectsOutOfRangeReads();
    void copiesEmptyRange();
};

namespace {

QByteArray writePattern(const QDmaBuf &buffer)
{
    QByteArray pattern(qsizetype(buffer.size()), '\0');
    for (qsizetype i = 0; i < pattern.size(); ++i)
        pattern[i] = char((i * 7 + 13) % 256);

    void *mapping = mmap(nullptr, size_t(buffer.size()), PROT_READ | PROT_WRITE, MAP_SHARED,
                         buffer.fd(), 0);
    if (mapping == MAP_FAILED)
        return {};

    std::memcpy(mapping, pattern.constData(), size_t(pattern.size()));
    munmap(mapping, size_t(buffer.size()));
    return pattern;
}

} // namespace

void tst_QMultimediaDmaBufSupport::copiesRange_data()
{
    QTest::addColumn<int>("offset");
    QTest::addColumn<int>("size");

    const int pageSize = int(sysconf(_SC_PAGESIZE));

    QTest::newRow("whole") << 0 << 8 * pageSize;
    QTest::newRow("page-aligned") << pageSize << 2 * pageSize;
    QTest::newRow("unaligned") << pageSize + 13 << pageSize + 100;
    QTest::newRow("tail") << 8 * pageSize - 137 << 137;
}

void tst_QMultimediaDmaBufSupport::copiesRange()
{
    QFETCH(int, offset);
    QFETCH(int, size);

    auto allocator = QDmaBufAllocator::create();
    if (!allocator)
        QSKIP(qPrintable(allocator.error()));

    const size_t requestedSize = 8 * size_t(sysconf(_SC_PAGESIZE));
    auto bufferResult = allocator->allocate(qsizetype(requestedSize));
    if (!bufferResult)
        QSKIP(qPrintable(bufferResult.error()));

    const std::shared_ptr<QDmaBuf> buffer = *bufferResult;
    QCOMPARE_GE(size_t(buffer->size()), requestedSize);

    const QByteArray pattern = writePattern(*buffer);
    QCOMPARE_EQ(pattern.size(), qsizetype(buffer->size()));

    const auto data = copyDmaBufData(buffer->fd(), off_t(offset), size_t(size));
    QVERIFY(data.has_value());
    QCOMPARE_EQ(*data, pattern.mid(offset, size));
}

void tst_QMultimediaDmaBufSupport::explicitBufferSizeMatchesFdSize()
{
    auto allocator = QDmaBufAllocator::create();
    if (!allocator)
        QSKIP(qPrintable(allocator.error()));

    auto bufferResult = allocator->allocate(qsizetype(8 * size_t(sysconf(_SC_PAGESIZE))));
    if (!bufferResult)
        QSKIP(qPrintable(bufferResult.error()));

    const std::shared_ptr<QDmaBuf> buffer = *bufferResult;
    const size_t bufferSize = size_t(buffer->size());

    const auto fromFd = copyDmaBufData(buffer->fd(), off_t(13), bufferSize - 13);
    const auto withExplicitSize =
            copyDmaBufData(buffer->fd(), off_t(13), bufferSize - 13, bufferSize);

    QVERIFY(fromFd.has_value());
    QVERIFY(withExplicitSize.has_value());
    QCOMPARE_EQ(*fromFd, *withExplicitSize);
}

void tst_QMultimediaDmaBufSupport::rejectsOutOfRangeReads()
{
    auto allocator = QDmaBufAllocator::create();
    if (!allocator)
        QSKIP(qPrintable(allocator.error()));

    auto bufferResult = allocator->allocate(qsizetype(8 * size_t(sysconf(_SC_PAGESIZE))));
    if (!bufferResult)
        QSKIP(qPrintable(bufferResult.error()));

    const std::shared_ptr<QDmaBuf> buffer = *bufferResult;
    const size_t bufferSize = size_t(buffer->size());

    QCOMPARE_EQ(copyDmaBufData(buffer->fd(), off_t(bufferSize - 10), 20).has_value(), false);
    QCOMPARE_EQ(copyDmaBufData(buffer->fd(), off_t(bufferSize), 1, bufferSize).has_value(), false);
    QCOMPARE_EQ(copyDmaBufData(buffer->fd(), off_t(-1), 1).has_value(), false);
}

void tst_QMultimediaDmaBufSupport::copiesEmptyRange()
{
    auto allocator = QDmaBufAllocator::create();
    if (!allocator)
        QSKIP(qPrintable(allocator.error()));

    auto bufferResult = allocator->allocate(qsizetype(8 * size_t(sysconf(_SC_PAGESIZE))));
    if (!bufferResult)
        QSKIP(qPrintable(bufferResult.error()));

    const auto data = copyDmaBufData((*bufferResult)->fd(), off_t(0), size_t(0));
    QVERIFY(data.has_value());
    QCOMPARE_EQ(data->size(), qsizetype(0));
}

QTEST_APPLESS_MAIN(tst_QMultimediaDmaBufSupport)

#include "tst_qmultimedia_dmabuf_support.moc"
