// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtMultimedia/qvideoframe.h>

QT_USE_NAMESPACE

class tst_qvideoframe_nogui : public QObject
{
    Q_OBJECT

private slots:
    void toImage_convertsToQImage_whenNoRhiInstanceIsAvailable()
    {
        QImage expected{ 2, 2, QImage::Format_ARGB32_Premultiplied };

        expected.setPixelColor(0, 0, Qt::red);
        expected.setPixelColor(1, 0, Qt::green);
        expected.setPixelColor(0, 1, Qt::blue);
        expected.setPixelColor(1, 1, Qt::yellow);

        QVideoFrame frame{ expected };
        QImage actual = frame.toImage();

        QCOMPARE_EQ(expected, actual);
    }
};

// Do not initialize Gui, because we want to run the tests without a
// QGuiApplication.
QTEST_GUILESS_MAIN(tst_qvideoframe_nogui)

#include "tst_qvideoframe_nogui.moc"
