// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "widget.h"
#include "fixture.h"

#include <QtCore/qsystemsemaphore.h>

#include <QtGui/qwindow.h>

#include <QtTest/qtest.h>

#include <QtWidgets/qapplication.h>

TestWidget::TestWidget(const QString &uuid, QScreen *screen)
{
    // Give each window a unique title so that we can uniquely identify it
    setWindowTitle(uuid);

    setScreen(screen ? screen : QApplication::primaryScreen());

    // Use frameless hint because on Windows UWP platform, the window titlebar is captured,
    // but the reference image acquired by 'QWindow::grab()' does not include titlebar.
    // This allows us to do pixel-perfect matching of captured content.
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setFixedSize(60, 40);
}

void TestWidget::setDisplayPattern(Pattern p)
{
    m_pattern = p;

    // The animated pattern needs continuous repaints to keep the content
    // changing. The other patterns are static.
    m_animated = (p == Pattern::Animated);

    repaint();
}

void TestWidget::setSize(QSize size)
{
    if (size == QApplication::primaryScreen()->size())
        setWindowState(Qt::WindowMaximized);
    else
        setFixedSize(size);
}

QImage TestWidget::grabImage()
{
    return grab().toImage();
}

void TestWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawRect(rect());

    switch (m_pattern) {
    case Pattern::ColoredSquares:
        drawColoredSquares(p);
        break;
    case Pattern::Grid:
        drawGrid(p);
        break;
    case Pattern::Animated:
        drawAnimationFrame(p);
        break;
    }

    p.end();

    if (m_animated) {
        ++m_animationTick;
        if (QWindow *window = windowHandle())
            window->requestUpdate();
    }
}

void TestWidget::drawColoredSquares(QPainter &p) const
{
    const std::vector<std::vector<Qt::GlobalColor>> colors = { { Qt::red, Qt::green, Qt::blue },
                                                               { Qt::white, Qt::white, Qt::white },
                                                               { Qt::blue, Qt::green, Qt::red } };

    const QSize squareSize = size() / 3;
    QRect rect{ QPoint{ 0, 0 }, squareSize };

    for (const auto &row : colors) {
        for (const auto &color : row) {
            p.setBrush(color);
            p.drawRect(rect);
            rect.moveLeft(rect.left() + rect.width());
        }
        rect.moveTo({ 0, rect.bottom() });
    }
}

void TestWidget::drawAnimationFrame(QPainter &p) const
{
    // Alternate between the colored squares and the grid pattern on every tick
    // so that the captured content changes from frame to frame.
    if (m_animationTick % 2 == 0)
        drawColoredSquares(p);
    else
        drawGrid(p);
}

void TestWidget::drawGrid(QPainter &p) const
{
    const QSize winSize = size();

    p.setPen(Qt::white);

    QLine vertical{ QPoint{ 5, 0 }, QPoint{ 5, winSize.height() } };
    while (vertical.x1() < winSize.width()) {
        p.drawLine(vertical);
        vertical.translate(10, 0);
    }
    QLine horizontal{ QPoint{ 0, 5 }, QPoint{ winSize.width(), 5 } };
    while (horizontal.y1() < winSize.height()) {
        p.drawLine(horizontal);
        horizontal.translate(0, 10);
    }
}

bool showCaptureWindow(const QString &windowTitle)
{
    const QNativeIpcKey key{ windowTitle };
    QSystemSemaphore windowVisible(key);

    TestWidget widget{ windowTitle };
    widget.show();

    // Wait for window to be visible and suitable for window capturing
    const bool result = QTest::qWaitForWindowExposed(&widget, s_testTimeout.count());
    if (!result)
        qDebug() << "Failed to show window";

    // Signal to host process that the window is visible
    windowVisible.release();

    // Keep window visible until a termination signal is received
    QApplication::exec();

    return result;
}

#include "moc_widget.cpp"
