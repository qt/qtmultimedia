// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qmockintegration.h>
#include <qmockvideosink.h>

#include <QtMultimediaTestLib/private/mediabackendutils_p.h>
#include <QtMultimediaWidgets/qvideowidget.h>
#include <QtMultimediaWidgets/private/qvideowidget_p.h>
#include <QtMultimedia/private/qvideowindow_p.h>
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/qvideosink.h>
#include <QtWidgets/qapplication.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qversionnumber.h>
#if QT_CONFIG(vulkan)
#  include <QtGui/qvulkaninstance.h>
#endif

#include <memory>
#include <optional>
#include <utility>
#include <vector>

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QVideoWidget : public QObject
{
    Q_OBJECT

private slots:
    void nullObject_data();
    void nullObject();

    void show_data();
    void show();
    void fullScreen_data();
    void fullScreen();
    void aspectRatio_data();
    void aspectRatio();
    void sizeHint_data();
    void sizeHint();
#if 0
    void brightness_data() { color_data(); }
    void brightness();
    void contrast_data() { color_data(); }
    void contrast();
    void hue_data() { color_data(); }
    void hue();
    void saturation_data() { color_data(); }
    void saturation();
#endif

    void paint_data();
    void paint();

private:
//    void color_data();
};

class QtTestVideoWidget : public QVideoWidget
{
public:
    QtTestVideoWidget(QWidget *parent = nullptr)
        : QVideoWidget(parent)
    {
        resize(320, 240);
    }

    QVideoWindow *videoWindow() const { return d_ptr->videoWindow; }
};

namespace {

bool probeBackend(QRhi::Implementation backend)
{
    switch (backend) {
#if QT_CONFIG(opengl)
    case QRhi::OpenGLES2: {
        if (!isOpenGLSupported())
            return false;
        const std::unique_ptr<QOffscreenSurface> fallbackSurface(
                QRhiGles2InitParams::newFallbackSurface());
        QRhiGles2InitParams params;
        params.fallbackSurface = fallbackSurface.get();
        return QRhi::probe(backend, &params);
    }
#endif
#if QT_CONFIG(vulkan)
    case QRhi::Vulkan: {
        QVulkanInstance instance;
        instance.setApiVersion(QVersionNumber(1, 1));
        instance.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
        if (!instance.create())
            return false;
        QRhiVulkanInitParams params;
        params.inst = &instance;
        return QRhi::probe(backend, &params);
    }
#endif
#if QT_CONFIG(metal)
    case QRhi::Metal: {
        QRhiMetalInitParams params;
        return QRhi::probe(backend, &params);
    }
#endif
#if defined(Q_OS_WIN)
    case QRhi::D3D11: {
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        return QRhi::probe(backend, &params);
    }
    case QRhi::D3D12: {
        QRhiD3D12InitParams params;
        return QRhi::probe(backend, &params);
    }
#endif
    default:
        return false;
    }
}

std::vector<std::pair<const char *, QRhi::Implementation>> backendRows()
{
    std::vector<std::pair<const char *, QRhi::Implementation>> candidates;
#if QT_CONFIG(opengl)
    if (isOpenGLSupported())
        candidates.emplace_back("OpenGL", QRhi::OpenGLES2);
#endif
#if QT_CONFIG(vulkan)
    candidates.emplace_back("Vulkan", QRhi::Vulkan);
#endif
#if QT_CONFIG(metal)
    candidates.emplace_back("Metal", QRhi::Metal);
#endif
#if defined(Q_OS_WIN)
    candidates.emplace_back("D3D11", QRhi::D3D11);
    candidates.emplace_back("D3D12", QRhi::D3D12);
#endif

    std::vector<std::pair<const char *, QRhi::Implementation>> rows;
    for (const auto &[name, backend] : candidates) {
        if (probeBackend(backend))
            rows.emplace_back(name, backend);
    }

    return rows;
}

void addBackendRows()
{
    QTest::addColumn<QRhi::Implementation>("backend");
    const auto rows = backendRows();
    if (rows.empty())
        QSKIP("No QRhi backend is available on this system");
    for (const auto &[name, backend] : rows)
        QTest::newRow(name) << backend;
}

// Selects the graphics API of the QVideoWindow in the widgets created while it is alive. A
// Vulkan window needs an instance, which must outlive the window: declare this before the widget.
class BackendSetup
{
public:
    explicit BackendSetup(QRhi::Implementation backend)
    {
#if QT_CONFIG(vulkan)
        if (backend == QRhi::Vulkan) {
            m_vulkanInstance = std::make_unique<QVulkanInstance>();
            m_vulkanInstance->setApiVersion(QVersionNumber(1, 1));
            m_vulkanInstance->setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
            if (!m_vulkanInstance->create()) {
                m_vulkanInstance.reset();
                return;
            }
        }
#endif
        QVideoWindowPrivate::setGraphicsApiOverride(backend);
    }

    ~BackendSetup() { QVideoWindowPrivate::setGraphicsApiOverride(std::nullopt); }

    BackendSetup(const BackendSetup &) = delete;
    BackendSetup &operator=(const BackendSetup &) = delete;

    void attach(QVideoWindow *window) const
    {
#if QT_CONFIG(vulkan)
        if (m_vulkanInstance)
            window->setVulkanInstance(m_vulkanInstance.get());
#else
        Q_UNUSED(window);
#endif
    }

private:
#if QT_CONFIG(vulkan)
    std::unique_ptr<QVulkanInstance> m_vulkanInstance;
#endif
};

// Shows the widget and returns the RHI of its window, or null if the window has none.
QRhi *showWidget(QtTestVideoWidget &widget)
{
    widget.show();
    if (!QTest::qWaitForWindowExposed(widget.videoWindow()))
        return nullptr;
    return QVideoWindowPrivate::get(widget.videoWindow())->rhi();
}

} // namespace

void tst_QVideoWidget::nullObject_data()
{
    addBackendRows();
}

void tst_QVideoWidget::nullObject()
{
    QFETCH(QRhi::Implementation, backend);

    BackendSetup setup(backend);
    QtTestVideoWidget widget;
    setup.attach(widget.videoWindow());

    QRhi *rhi = showWidget(widget);
    if (!rhi)
        QSKIP("The video window has no QRhi for this backend on this system");
    QCOMPARE(rhi->backend(), backend);

    widget.setFullScreen(true);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);

    widget.setAspectRatioMode(Qt::IgnoreAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::IgnoreAspectRatio);
}

void tst_QVideoWidget::show_data()
{
    addBackendRows();
}

void tst_QVideoWidget::show()
{
    QFETCH(QRhi::Implementation, backend);

    BackendSetup setup(backend);
    QtTestVideoWidget widget;
    setup.attach(widget.videoWindow());

    QRhi *rhi = showWidget(widget);
    if (!rhi)
        QSKIP("The video window has no QRhi for this backend on this system");
    QCOMPARE(rhi->backend(), backend);

    widget.resize(640, 480);
    QCOMPARE(widget.size(), QSize(640, 480));

    widget.move(10, 10);
    QCOMPARE(widget.size(), QSize(640, 480));

    widget.hide();
}

void tst_QVideoWidget::aspectRatio_data()
{
    addBackendRows();
}

void tst_QVideoWidget::aspectRatio()
{
    QFETCH(QRhi::Implementation, backend);

    BackendSetup setup(backend);
    QtTestVideoWidget widget;
    setup.attach(widget.videoWindow());
    QMediaPlayer player;
    player.setVideoOutput(&widget);

    // Test the aspect ratio defaults to keeping the aspect ratio.
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);

    // Test the control has been informed of the aspect ratio change, post show.
    QRhi *rhi = showWidget(widget);
    if (!rhi)
        QSKIP("The video window has no QRhi for this backend on this system");
    QCOMPARE(rhi->backend(), backend);
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);

    // Test an aspect ratio change is enforced immediately while visible.
    widget.setAspectRatioMode(Qt::IgnoreAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::IgnoreAspectRatio);

    // Test an aspect ratio set while not visible is respected.
    widget.hide();
    widget.setAspectRatioMode(Qt::KeepAspectRatio);
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);
    widget.show();
    QCOMPARE(widget.aspectRatioMode(), Qt::KeepAspectRatio);
}

void tst_QVideoWidget::sizeHint_data()
{
    QTest::addColumn<QSize>("frameSize");
    QTest::addColumn<QRect>("viewport");
    QTest::addColumn<QSize>("expectedSize");
    QTest::addColumn<QRhi::Implementation>("backend");

    const auto rows = backendRows();
    if (rows.empty())
        QSKIP("No QRhi backend is available on this system");
    for (const auto &[name, backend] : rows) {
        const QByteArray tag = QByteArray("640x480 ") + name;
        QTest::newRow(tag.constData())
                << QSize(640, 480) << QRect(0, 0, 640, 480) << QSize(640, 480) << backend;
    }

//    QTest::newRow("800x600, (80,60, 640x480) viewport")
//            << QSize(800, 600)
//            << QRect(80, 60, 640, 480)
//            << QSize(640, 480);
}

void tst_QVideoWidget::sizeHint()
{
    QFETCH(QSize, frameSize);
//    QFETCH(QRect, viewport);
    QFETCH(QSize, expectedSize);
    QFETCH(QRhi::Implementation, backend);

    BackendSetup setup(backend);
    QtTestVideoWidget widget;
    setup.attach(widget.videoWindow());
    QMediaPlayer player;

    player.setVideoOutput(&widget);
    auto mockSink = QMockIntegration::instance()->lastVideoSink();

    QRhi *rhi = showWidget(widget);
    if (!rhi)
        QSKIP("The video window has no QRhi for this backend on this system");
    QCOMPARE(rhi->backend(), backend);
    mockSink->setNativeSize(frameSize);

    QCOMPARE(widget.sizeHint(), expectedSize);
}

void tst_QVideoWidget::fullScreen_data()
{
    addBackendRows();
}

void tst_QVideoWidget::fullScreen()
{
    QFETCH(QRhi::Implementation, backend);

    BackendSetup setup(backend);
    QtTestVideoWidget widget;
    setup.attach(widget.videoWindow());
    QMediaPlayer player;
    player.setVideoOutput(&widget);
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(widget.videoWindow()));
    QRhi *rhi = QVideoWindowPrivate::get(widget.videoWindow())->rhi();
    if (!rhi)
        QSKIP("The video window has no QRhi for this backend on this system");
    QCOMPARE(rhi->backend(), backend);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    Qt::WindowFlags windowFlags = widget.windowFlags();

    QSignalSpy spy(&widget, &QVideoWidget::fullScreenChanged);

    // Test showing full screen with setFullScreen(true).
    widget.setFullScreen(true);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.value(0).value(0).toBool(), true);

    // Test returning to normal with setFullScreen(false).
    widget.setFullScreen(false);
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.value(1).value(0).toBool(), false);
    QCOMPARE(widget.windowFlags(), windowFlags);

    // Test showing full screen with showFullScreen().
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.size(), 3);
    QCOMPARE(spy.value(2).value(0).toBool(), true);

    // Test returning to normal with showNormal().
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.size(), 4);
    QCOMPARE(spy.value(3).value(0).toBool(), false);
    QCOMPARE(widget.windowFlags(), windowFlags);

    // Test setFullScreen(false) and showNormal() do nothing when isFullScreen() == false.
    widget.setFullScreen(false);
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.size(), 4);
    widget.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    QCOMPARE(widget.isFullScreen(), false);
    QCOMPARE(spy.size(), 4);

    // Test setFullScreen(true) and showFullScreen() do nothing when isFullScreen() == true.
    widget.showFullScreen();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
    widget.setFullScreen(true);
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.size(), 5);
    widget.showFullScreen();
    QCOMPARE(widget.isFullScreen(), true);
    QCOMPARE(spy.size(), 5);
}

static const uchar rgb32ImageData[] =
{
    0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00
};

void tst_QVideoWidget::paint_data()
{
    addBackendRows();
}

void tst_QVideoWidget::paint()
{
    QFETCH(QRhi::Implementation, backend);

    // Counts the frames the window renders. Declared before the widget, which it outlives.
    int renders = 0;

    BackendSetup setup(backend);
    QtTestVideoWidget widget;
    setup.attach(widget.videoWindow());
    QMediaPlayer player;
    player.setVideoOutput(&widget);
    widget.resize(640,480);

    QRhi *rhi = showWidget(widget);
    if (!rhi)
        QSKIP("The video window has no QRhi for this backend on this system");
    QCOMPARE(rhi->backend(), backend);

    QVideoWindowPrivate::get(widget.videoWindow())
            ->setPostRenderHookForTests([&renders](QRhi &, QRhiCommandBuffer &) {
        ++renders;
    });

    QVideoFrameFormat format(QSize(2, 2), QVideoFrameFormat::Format_XRGB8888);
    QVideoFrame frame(format);
    QVERIFY(frame.map(QVideoFrame::ReadWrite));
    uchar *data = frame.bits(0);
    memcpy(data, rgb32ImageData, sizeof(rgb32ImageData));
    frame.unmap();

    auto *sink = widget.videoSink();
    sink->setVideoFrame(frame);

    // QVideoWindow only renders when an update is requested.
    QTRY_VERIFY(([&] {
        widget.videoWindow()->requestUpdate();
        return renders > 0;
    })());

    QVideoWindowPrivate::get(widget.videoWindow())->setPostRenderHookForTests({});
    QCoreApplication::processEvents(QEventLoop::AllEvents);
}

QTEST_MAIN(tst_QVideoWidget)

#include "tst_qvideowidget.moc"
