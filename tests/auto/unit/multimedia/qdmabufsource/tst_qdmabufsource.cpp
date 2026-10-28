// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qcaptureslotregistry_p.h>
#include <QtMultimedia/private/qdmabufallocator_p.h>
#include <QtMultimedia/private/qdmabufsource_p.h>
#include <QtMultimedia/private/qdmabufvideobuffer_p.h>
#include <QtMultimedia/private/qmaterializingvideobuffer_p.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qvarlengtharray.h>

#include <sys/mman.h>
#include <unistd.h>

#include <memory>
#include <vector>

using namespace QtMultimediaPrivate;

namespace {

// Backs a DMA-BUF with an anonymous memfd. The mapping and the sync ioctl (which only logs on
// failure) behave the same as for a real DMA-BUF, so no allocator or device is required.
std::shared_ptr<QDmaBuf> makeBuffer(const QByteArray &content)
{
    const int fd = memfd_create("tst_qdmabufsource", MFD_CLOEXEC);
    if (fd < 0)
        return {};

    QUniqueFileDescriptorHandle handle{ fd };
    if (ftruncate(fd, content.size()) != 0)
        return {};
    if (pwrite(fd, content.constData(), size_t(content.size()), 0) != content.size())
        return {};

    return std::make_shared<QDmaBuf>(std::move(handle), content.size());
}

// Y plane (value yValue) followed by the interleaved UV plane (value uvValue).
QByteArray semiPlanarContent(qsizetype ySize, qsizetype uvSize, char yValue, char uvValue)
{
    QByteArray content(ySize, yValue);
    content.append(QByteArray(uvSize, uvValue));
    return content;
}

class TestProducer : public CaptureSlotRegistry<TestProducer, quint32>
{
public:
    TestProducer() : CaptureSlotRegistry(4, 2) { }

    void recycleSlot(quint32 slot) { recycled.push_back(slot); }

    std::vector<quint32> recycled;
};

} // namespace

class tst_QDmaBufSource : public QObject
{
    Q_OBJECT

private slots:
    void dmaBufLayoutForFormat_data();
    void dmaBufLayoutForFormat();

    void singlePlane_mapsPackedPlane();
    void multiPlane_mapsEachPlaneAtItsOffset();
    void map_canBeRepeatedAfterUnmap();
    void source_keepsBufferAliveWhileHeld();

    void zeroCopy_multiPlane_exposesPlanesAndHoldsSlot();
    void zeroCopy_underPressure_copiesAllPlanes();
};

void tst_QDmaBufSource::dmaBufLayoutForFormat_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("format");
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("bytesPerLine");
    QTest::addColumn<quint32>("sizeImage");
    // Flattened (offset, size) pairs, one per plane. Empty means no layout.
    QTest::addColumn<QList<qsizetype>>("expected");

    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12 << QSize(640, 480) << 640
                          << quint32(460800) << QList<qsizetype>{ 0, 307200, 307200, 153600 };
    QTest::newRow("NV21") << QVideoFrameFormat::Format_NV21 << QSize(640, 480) << 640
                          << quint32(460800) << QList<qsizetype>{ 0, 307200, 307200, 153600 };
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010 << QSize(640, 480) << 1280
                          << quint32(921600) << QList<qsizetype>{ 0, 614400, 614400, 307200 };
    QTest::newRow("P016") << QVideoFrameFormat::Format_P016 << QSize(640, 480) << 1280
                          << quint32(921600) << QList<qsizetype>{ 0, 614400, 614400, 307200 };
    QTest::newRow("NV12-oddHeight")
            << QVideoFrameFormat::Format_NV12 << QSize(640, 481) << 640 << quint32(462080)
            << QList<qsizetype>{ 0, 307840, 307840, 154240 };
    QTest::newRow("NV12-paddedPitch")
            << QVideoFrameFormat::Format_NV12 << QSize(640, 480) << 704 << quint32(506880)
            << QList<qsizetype>{ 0, 337920, 337920, 168960 };
    QTest::newRow("NV12-sizeImageTooSmall") << QVideoFrameFormat::Format_NV12 << QSize(640, 480)
                                            << 704 << quint32(460800) << QList<qsizetype>{};
    QTest::newRow("NV12-zeroPitch") << QVideoFrameFormat::Format_NV12 << QSize(640, 480) << 0
                                    << quint32(460800) << QList<qsizetype>{};
    QTest::newRow("YUYV-packed") << QVideoFrameFormat::Format_YUYV << QSize(640, 480) << 1280
                                 << quint32(614400) << QList<qsizetype>{ 0, 614400 };
    QTest::newRow("YUV420P-fullyPlanar") << QVideoFrameFormat::Format_YUV420P << QSize(640, 480)
                                         << 640 << quint32(460800) << QList<qsizetype>{};
    QTest::newRow("Jpeg-compressed") << QVideoFrameFormat::Format_Jpeg << QSize(640, 480) << 640
                                     << quint32(460800) << QList<qsizetype>{};
}

void tst_QDmaBufSource::dmaBufLayoutForFormat()
{
    QFETCH(QVideoFrameFormat::PixelFormat, format);
    QFETCH(QSize, size);
    QFETCH(int, bytesPerLine);
    QFETCH(quint32, sizeImage);
    QFETCH(QList<qsizetype>, expected);

    const auto layout =
            QtMultimediaPrivate::dmaBufLayoutForFormat(format, size, bytesPerLine, sizeImage);
    if (expected.isEmpty()) {
        QCOMPARE(layout.has_value(), false);
        return;
    }

    QVERIFY(layout.has_value());
    QCOMPARE(layout->size() * 2, expected.size());
    for (qsizetype plane = 0; plane < layout->size(); ++plane) {
        QCOMPARE((*layout)[plane].offset, uint32_t(expected[2 * plane]));
        QCOMPARE((*layout)[plane].size, expected[2 * plane + 1]);
        QCOMPARE((*layout)[plane].pitch, uint32_t(bytesPerLine));
    }
}

void tst_QDmaBufSource::singlePlane_mapsPackedPlane()
{
    const qsizetype size = 1280 * 480;
    const auto buf = makeBuffer(QByteArray(size, '\x5a'));
    QVERIFY(buf);

    DmaBufSource source(buf, DmaBufLayout{ DmaBufPlaneLayout{ 0, 1280, size } });
    const auto mapData = source.mapReadOnly();
    QVERIFY(mapData.has_value());

    QCOMPARE(mapData->planeCount, 1);
    QCOMPARE(mapData->bytesPerLine[0], 1280);
    QCOMPARE(mapData->dataSize[0], int(size));
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData->data[0]), 16),
             QByteArray(16, '\x5a'));

    QAbstractVideoBuffer::MapData unmapped = *mapData;
    source.unmap(unmapped);
}

void tst_QDmaBufSource::multiPlane_mapsEachPlaneAtItsOffset()
{
    const auto buf = makeBuffer(semiPlanarContent(307200, 153600, 0x10, char(0x80)));
    QVERIFY(buf);

    const auto layout = QtMultimediaPrivate::dmaBufLayoutForFormat(QVideoFrameFormat::Format_NV12,
                                                                   QSize(640, 480), 640, 460800);
    QVERIFY(layout.has_value());

    DmaBufSource source(buf, *layout);
    auto mapData = source.mapReadOnly();
    QVERIFY(mapData.has_value());

    QCOMPARE(mapData->planeCount, 2);
    QCOMPARE(mapData->bytesPerLine[0], 640);
    QCOMPARE(mapData->bytesPerLine[1], 640);
    QCOMPARE(mapData->dataSize[0], 307200);
    QCOMPARE(mapData->dataSize[1], 153600);
    QCOMPARE(mapData->data[1] - mapData->data[0], qptrdiff(307200));

    QCOMPARE(mapData->data[0][0], uchar(0x10));
    QCOMPARE(mapData->data[0][307199], uchar(0x10));
    QCOMPARE(mapData->data[1][0], uchar(0x80));
    QCOMPARE(mapData->data[1][153599], uchar(0x80));

    source.unmap(*mapData);
}

void tst_QDmaBufSource::map_canBeRepeatedAfterUnmap()
{
    const auto buf = makeBuffer(semiPlanarContent(307200, 153600, 0x01, char(0x02)));
    QVERIFY(buf);

    const auto layout = QtMultimediaPrivate::dmaBufLayoutForFormat(QVideoFrameFormat::Format_NV12,
                                                                   QSize(640, 480), 640, 460800);
    QVERIFY(layout.has_value());
    DmaBufSource source(buf, *layout);

    for (int round = 0; round < 2; ++round) {
        auto mapData = source.mapReadOnly();
        QVERIFY(mapData.has_value());
        QCOMPARE(mapData->data[1][0], uchar(0x02));
        source.unmap(*mapData);
    }
}

void tst_QDmaBufSource::source_keepsBufferAliveWhileHeld()
{
    auto buf = makeBuffer(QByteArray(1280 * 480, '\0'));
    QVERIFY(buf);
    std::weak_ptr<QDmaBuf> weak = buf;

    {
        DmaBufSource source(buf, DmaBufLayout{ DmaBufPlaneLayout{ 0, 1280, 614400 } });
        buf.reset();
        QVERIFY(!weak.expired());
    }

    QVERIFY(weak.expired());
}

void tst_QDmaBufSource::zeroCopy_multiPlane_exposesPlanesAndHoldsSlot()
{
    auto producer = std::make_shared<TestProducer>();
    const auto buf = makeBuffer(semiPlanarContent(307200, 153600, 0x10, char(0x80)));
    QVERIFY(buf);

    const QVideoFrameFormat::PixelFormat format = QVideoFrameFormat::Format_NV12;
    const QSize size(640, 480);
    const auto layout = QtMultimediaPrivate::dmaBufLayoutForFormat(format, size, 640, 460800);
    QVERIFY(layout.has_value());

    const auto drmFormats = dmaBufFourccFromPixelFormat(format);
    QVarLengthArray<DmaBufPlane, 4> planes;
    for (qsizetype plane = 0; plane < layout->size(); ++plane)
        planes.push_back(
                { buf->fd(), (*layout)[plane].offset, (*layout)[plane].pitch, drmFormats[plane] });

    using Buffer = MaterializingVideoBuffer<DmaBufSource, TestProducer, QDmaBufVideoBuffer>;
    auto buffer = Buffer::wrapOrMaterialize(
            *producer, 1, DmaBufSource(buf, *layout), QVideoFrameFormat(size, format), format, size,
            QSpan<const DmaBufPlane>(planes.data(), planes.size()), buf);

    QVERIFY(buffer);
    QVERIFY(dynamic_cast<QDmaBufVideoBuffer *>(buffer.get()));
    QCOMPARE(producer->outstandingSlotCount(), std::size_t(1));

    auto mapData = buffer->map(QVideoFrame::ReadOnly);
    QCOMPARE(mapData.planeCount, 2);
    QCOMPARE(mapData.data[0][0], uchar(0x10));
    QCOMPARE(mapData.data[1][0], uchar(0x80));
    buffer->unmap();

    buffer.reset();
    QCOMPARE(producer->outstandingSlotCount(), std::size_t(0));
    QCOMPARE(producer->recycled, std::vector<quint32>{ 1 });
}

void tst_QDmaBufSource::zeroCopy_underPressure_copiesAllPlanes()
{
    auto producer = std::make_shared<TestProducer>();
    const auto buf = makeBuffer(semiPlanarContent(307200, 153600, 0x10, char(0x80)));
    QVERIFY(buf);

    const QVideoFrameFormat::PixelFormat format = QVideoFrameFormat::Format_NV12;
    const QSize size(640, 480);
    const auto layout = QtMultimediaPrivate::dmaBufLayoutForFormat(format, size, 640, 460800);
    QVERIFY(layout.has_value());

    const auto drmFormats = dmaBufFourccFromPixelFormat(format);
    QVarLengthArray<DmaBufPlane, 4> planes;
    for (qsizetype plane = 0; plane < layout->size(); ++plane)
        planes.push_back(DmaBufPlane{
                buf->fd(),
                (*layout)[plane].offset,
                (*layout)[plane].pitch,
                drmFormats[plane],
        });
    const QSpan<const DmaBufPlane> planeSpan(planes.data(), planes.size());

    using Buffer = MaterializingVideoBuffer<DmaBufSource, TestProducer, QDmaBufVideoBuffer>;
    // minFreeSlots is 2 of 4, so the third reservation is refused and copies eagerly.
    std::vector<std::unique_ptr<QAbstractVideoBuffer>> held;
    for (quint32 slot = 1; slot <= 2; ++slot) {
        held.push_back(Buffer::wrapOrMaterialize(*producer, slot, DmaBufSource(buf, *layout), {},
                                                 format, size, planeSpan, buf));
        QVERIFY(dynamic_cast<QDmaBufVideoBuffer *>(held.back().get()));
    }

    auto copied = Buffer::wrapOrMaterialize(*producer, 3, DmaBufSource(buf, *layout), {}, format,
                                            size, planeSpan, buf);
    QVERIFY(copied);
    QVERIFY(!dynamic_cast<QDmaBufVideoBuffer *>(copied.get()));
    QCOMPARE(producer->recycled, std::vector<quint32>{ 3 });

    auto mapData = copied->map(QVideoFrame::ReadOnly);
    QCOMPARE(mapData.planeCount, 2);
    QCOMPARE(mapData.dataSize[0], 307200);
    QCOMPARE(mapData.dataSize[1], 153600);
    QCOMPARE(mapData.data[0][0], uchar(0x10));
    QCOMPARE(mapData.data[1][0], uchar(0x80));
    copied->unmap();
}

QTEST_APPLESS_MAIN(tst_QDmaBufSource)

#include "tst_qdmabufsource.moc"
