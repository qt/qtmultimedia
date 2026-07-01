// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WINDOW_CAPTURE_WIDGET_H
#define WINDOW_CAPTURE_WIDGET_H

#include <QtCore/qtimer.h>
#include <QtCore/quuid.h>

#include <QtGui/qpainter.h>
#include <QtGui/qscreen.h>

#include <QtWidgets/qwidget.h>

/*!
    Window capable of drawing test patterns used for capture tests
 */
class TestWidget : public QWidget
{
    Q_OBJECT

public:
    enum class Pattern {
        ColoredSquares,
        Grid,
        /*
            Continuous animation that switches between ColoredSquares
            and Grid at 60 FPS. Note: New paint events is driven by
            QTimer on main thread. The animation stops if the main
            thread is blocked.
         */
        Animated,
    };

    TestWidget(const QString &uuid = QUuid::createUuid().toString(), QScreen *screen = nullptr);

    void setDisplayPattern(Pattern p);
    void setSize(QSize size);
    QImage grabImage();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void drawColoredSquares(QPainter &p) const;
    void drawGrid(QPainter &p) const;
    void drawAnimationFrame(QPainter &p) const;

    Pattern m_pattern = Pattern::ColoredSquares;

    QTimer m_animationTimer;
    unsigned m_animationTick = 0;
};

bool showCaptureWindow(const QString &windowTitle);

#endif
