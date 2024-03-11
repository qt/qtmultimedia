// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <qmediaplayer.h>
#include <qvideoframe.h>
#include <qdebug.h>

#include "mediafileselector.h"
#include "testvideosink.h"

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

private:
    QVideoFrame createDefaultFrame() const;

    QVideoFrame createMediaPlayerFrame() const;

    using FrameCreator = decltype(&tst_QVideoFrameBackend::createDefaultFrame);

    template <typename F>
    void addMediaPlayerFrameTestData(F &&f);

private:
    MaybeUrl m_oneRedFrameVideo = QUnexpect{};
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

    f();
}

void tst_QVideoFrameBackend::initTestCase()
{
#ifdef Q_OS_ANDROID
    qWarning() << "Skip media selection, QTBUG-118571";
    return;
#endif

    m_oneRedFrameVideo = m_mediaSelector.select("qrc:/testdata/one_red_frame.mp4");
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

    auto frame = std::invoke(frameCreator, this);
    QVERIFY(frame.isValid());

    frame.map(initialMapMode);
    QCOMPARE(frame.mapMode(), initialMapMode);

    auto image = frame.toImage();
    QVERIFY(!image.isNull());

    QCOMPARE(frame.mapMode(), initialMapMode);
    QCOMPARE(frame.isMapped(), initiallyMapped);
}

QTEST_MAIN(tst_QVideoFrameBackend)
#include "tst_qvideoframebackend.moc"
