// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/mediabackendutils_p.h>

#include <QtQuickTest/quicktest.h>

struct TestSetupClass : public QObject
{
    TestSetupClass() { initIntegrationTestMain(); }
};

QUICK_TEST_MAIN_WITH_SETUP(qml, TestSetupClass);
