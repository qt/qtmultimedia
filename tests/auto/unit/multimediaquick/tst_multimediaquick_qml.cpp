// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmockintegration.h"
#include "tst_multimediaquick_qml_screencapturehelper.h"

#include <QtQml/qqml.h>
#include <QtQuickTest/quicktest.h>

QT_USE_NAMESPACE

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

struct TestSetupClass : public QObject
{
    TestSetupClass()
    {
        qmlRegisterSingletonInstance(
            "QtMultimediaTest",
            1,
            0,
            "ScreenCaptureHelper",
            new ScreenCaptureHelper);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(multimediaquick, TestSetupClass)
