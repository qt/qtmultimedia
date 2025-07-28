// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <qmediaplayer.h>
#include <qvideoframe.h>
#include <qdebug.h>

#include <private/mediafileselector_p.h>
#include <private/mediabackendutils_p.h>
#include <private/testvideosink_p.h>
#include "private/qvideotexturehelper_p.h"
#include "private/qvideowindow_p.h"
#include <thread>


QT_USE_NAMESPACE

class tst_QVideoFrameBackend : public QObject
{
    Q_OBJECT

public slots:
    void initTestCase();
    void init() { }
    void cleanup() { }

private slots:
    void testMediaFilesAreSupported();

    void toImage_retainsThePreviousMappedState_data();
    void toImage_retainsThePreviousMappedState();

    void toImage_rendersUpdatedFrame_afterMappingInWriteModeAndModifying_data();
    void toImage_rendersUpdatedFrame_afterMappingInWriteModeAndModifying();

    void toImage_returnsImage_whenCalledFromSeparateThreadAndWhileRenderingToWindow();

private:
    QVideoFrame createDefaultFrame() const;

    QVideoFrame createMediaPlayerFrame() const;

    using FrameCreator = decltype(&tst_QVideoFrameBackend::createDefaultFrame);

    template <typename F>
    void addMediaPlayerFrameTestData(F &&f);

private:
    MaybeUrl m_oneRedFrameVideo{ q23::unexpect };
    MaybeUrl m_colorsVideo{ q23::unexpect };
    MediaFileSelector m_mediaSelector;
};

QVideoFrame tst_QVideoFrameBackend::createDefaultFrame() const
{
    return QVideoFrame(QVideoFrameFormat(QSize(10, 20), QVideoFrameFormat::Format_ARGB8888));
}

QVideoFrame tst_QVideoFrameBackend::createMediaPlayerFrame() const
{
    if (!m_oneRedFrameVideo)
        return {};

    TestVideoSink sink;
    QMediaPlayer player;

    player.setVideoOutput(&sink);
    player.setSource(*m_oneRedFrameVideo);

    player.play();

    return sink.waitForFrame();
}

template <typename F>
void tst_QVideoFrameBackend::addMediaPlayerFrameTestData(F &&f)
{
    if (!m_oneRedFrameVideo) {
        qWarning() << "Skipping test data with mediaplayer as the source cannot be open."
                      "\nSee the test case 'testMediaFilesAreSupported' for details";
        return;
    }

    if (isGStreamerPlatform()) {
        qWarning() << "createMediaPlayerFrame spuriously fails with gstreamer";
        return;
    }

    f();
}

void tst_QVideoFrameBackend::initTestCase()
{
#ifdef Q_OS_ANDROID
    qWarning() << "Skip media selection, QTBUG-118571";
    return;
#endif

    m_oneRedFrameVideo = m_mediaSelector.select("qrc:/testdata/one_red_frame.mp4");
    m_colorsVideo = m_mediaSelector.select("qrc:/testdata/colors.mp4");
}

void tst_QVideoFrameBackend::testMediaFilesAreSupported()
{
#ifdef Q_OS_ANDROID
    QSKIP("Skip test cases with mediaPlayerFrame on Android CI, because of QTBUG-118571");
#endif

    QCOMPARE(m_mediaSelector.dumpErrors(), "");
}

void tst_QVideoFrameBackend::toImage_retainsThePreviousMappedState_data()
{
    QTest::addColumn<FrameCreator>("frameCreator");
    QTest::addColumn<QVideoFrame::MapMode>("initialMapMode");

    // clang-format off
    QTest::addRow("defaulFrame.notMapped") << &tst_QVideoFrameBackend::createDefaultFrame
                                           << QVideoFrame::NotMapped;
    QTest::addRow("defaulFrame.readOnly") << &tst_QVideoFrameBackend::createDefaultFrame
                                          << QVideoFrame::ReadOnly;

    addMediaPlayerFrameTestData([]()
    {
        QTest::addRow("mediaPlayerFrame.notMapped")
                << &tst_QVideoFrameBackend::createMediaPlayerFrame
                << QVideoFrame::NotMapped;
        QTest::addRow("mediaPlayerFrame.readOnly")
                << &tst_QVideoFrameBackend::createMediaPlayerFrame
                << QVideoFrame::ReadOnly;
    });

    // clang-format on
}

void tst_QVideoFrameBackend::toImage_retainsThePreviousMappedState()
{
    QFETCH(const FrameCreator, frameCreator);
    QFETCH(const QVideoFrame::MapMode, initialMapMode);
    const bool initiallyMapped = initialMapMode != QVideoFrame::NotMapped;

    QVideoFrame frame = std::invoke(frameCreator, this);
    QVERIFY(frame.isValid());

    frame.map(initialMapMode);
    QCOMPARE(frame.mapMode(), initialMapMode);

    QImage image = frame.toImage();
    QVERIFY(!image.isNull());

    QCOMPARE(frame.mapMode(), initialMapMode);
    QCOMPARE(frame.isMapped(), initiallyMapped);
}

void tst_QVideoFrameBackend::toImage_rendersUpdatedFrame_afterMappingInWriteModeAndModifying_data()
{
    QTest::addColumn<FrameCreator>("frameCreator");
    QTest::addColumn<QVideoFrame::MapMode>("mapMode");

    // clang-format off
    QTest::addRow("defaulFrame.writeOnly") << &tst_QVideoFrameBackend::createDefaultFrame
                                           << QVideoFrame::WriteOnly;
    QTest::addRow("defaulFrame.readWrite") << &tst_QVideoFrameBackend::createDefaultFrame
                                           << QVideoFrame::ReadWrite;

    addMediaPlayerFrameTestData([]()
    {
        QTest::addRow("mediaPlayerFrame.writeOnly")
                << &tst_QVideoFrameBackend::createMediaPlayerFrame
                << QVideoFrame::WriteOnly;
        QTest::addRow("mediaPlayerFrame.readWrite")
                << &tst_QVideoFrameBackend::createMediaPlayerFrame
                << QVideoFrame::ReadWrite;
    });
    // clang-format on
}

void tst_QVideoFrameBackend::toImage_rendersUpdatedFrame_afterMappingInWriteModeAndModifying()
{
    QFETCH(const FrameCreator, frameCreator);
    QFETCH(const QVideoFrame::MapMode, mapMode);

    // Arrange

    QVideoFrame frame = std::invoke(frameCreator, this);
    QVERIFY(frame.isValid());

    QImage originalImage = frame.toImage();
    QVERIFY(!originalImage.isNull());

    // Act: map the frame in write mode and change the top level pixel
    frame.map(mapMode);
    QVERIFY(frame.isWritable());

    QCOMPARE_NE(frame.pixelFormat(), QVideoFrameFormat::Format_Invalid);

    const QVideoTextureHelper::TextureDescription *textureDescription =
            QVideoTextureHelper::textureDescription(frame.pixelFormat());
    QVERIFY(textureDescription);

    uchar *firstPlaneBits = frame.bits(0);
    QVERIFY(firstPlaneBits);

    for (int i = 0; i < textureDescription->strideFactor; ++i)
        firstPlaneBits[i] = ~firstPlaneBits[i];

    frame.unmap();

    // get an image from modified frame
    QImage modifiedImage = frame.toImage();

    // Assert

    QVERIFY(!frame.isMapped());
    QCOMPARE_NE(originalImage.pixel(0, 0), modifiedImage.pixel(0, 0));
    QCOMPARE(originalImage.pixel(1, 0), modifiedImage.pixel(1, 0));
    QCOMPARE(originalImage.pixel(1, 1), modifiedImage.pixel(1, 1));
}

void tst_QVideoFrameBackend::toImage_returnsImage_whenCalledFromSeparateThreadAndWhileRenderingToWindow()
{
    if (isCI()) {
#ifdef Q_OS_MACOS
        QSKIP("SKIP on macOS because of crash and error \"Failed to create QWindow::MetalSurface. Metal is not supported by any of the GPUs in this system.\"");
#elif defined(Q_OS_ANDROID)
        QSKIP("SKIP initTestCase on CI, because of QTBUG-118571");
#endif
    }
    // Arrange
    QVideoWindow window;
    window.show();

    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QMediaPlayer player;
    player.setVideoOutput(&window);

    const QVideoSink *sink = window.videoSink();
    std::vector<QImage> images;

    // act
    connect(sink, &QVideoSink::videoFrameChanged, sink, [&](const QVideoFrame &frame) {

        // Run toImage on separate thread to exercise special code path
        QImage image;
        auto t = std::thread([&] { image = frame.toImage(); });
        t.join();

        if (!image.isNull())
            images.push_back(image);
    });

    // Arrange some more
    player.setSource(*m_colorsVideo);
    player.setLoops(10);
    player.play();

    // assert
    QTRY_COMPARE_GE_WITH_TIMEOUT(images.size(), 10u, std::chrono::seconds(60) );
}

QTEST_MAIN(tst_QVideoFrameBackend)
#include "tst_qvideoframebackend.moc"
