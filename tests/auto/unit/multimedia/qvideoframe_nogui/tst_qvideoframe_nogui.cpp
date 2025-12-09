// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtMultimedia/qvideoframe.h>

QT_USE_NAMESPACE

class tst_qvideoframe_nogui : public QObject
{
    Q_OBJECT

private slots:
    void toImage_convertsToQImage_whenNoRhiInstanceIsAvailable()
    {
        QVideoFrameFormat format{{ 4, 4 }, QVideoFrameFormat::Format_ARGB8888};
        QVideoFrame frame{ format };
        QImage actual = frame.toImage();

        QVERIFY(!actual.isNull());
    }
};

// Do not initialize Gui, because we want to run the tests without a
// QGuiApplication.
QTEST_GUILESS_MAIN(tst_qvideoframe_nogui)

#include "tst_qvideoframe_nogui.moc"
