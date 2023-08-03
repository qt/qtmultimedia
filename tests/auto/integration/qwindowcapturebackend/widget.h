// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WINDOW_CAPTURE_WIDGET_H
#define WINDOW_CAPTURE_WIDGET_H

#include <qwidget.h>
#include <qscreen.h>
#include <qpainter.h>
#include <quuid.h>

/*!
    Window capable of drawing test patterns used for capture tests
 */
class TestWidget : public QWidget
{
    Q_OBJECT

public:
    enum Pattern { ColoredSquares, Grid };

    TestWidget(const QString &uuid = QUuid::createUuid().toString(), QScreen *screen = nullptr);

    void setDisplayPattern(Pattern p);
    void setSize(QSize size);
    QImage grabImage();

public slots:
    void togglePattern();

protected:
    void paintEvent(QPaintEvent * /*event*/) override;

private:
    void drawColoredSquares(QPainter &p);
    void drawGrid(QPainter &p) const;

    Pattern m_pattern = ColoredSquares;
};

bool showCaptureWindow(const QString &windowTitle);

#endif
