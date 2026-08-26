// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/mediabackendutils_p.h>

#include <QtQuickTest/quicktest.h>

#include <QtQml/qqml.h>

class TestHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isMediaBackendPluginLoaded READ isMediaBackendPluginLoaded CONSTANT FINAL)
    Q_PROPERTY(bool isWMFPlatform READ isWMFPlatform CONSTANT FINAL)
public:
    [[nodiscard]] static bool isMediaBackendPluginLoaded()
    {
        return BackendUtilsImpl::isMediaBackendPluginLoaded();
    }

    [[nodiscard]] static bool isWMFPlatform()
    {
        return BackendUtilsImpl::isWMFPlatform();
    }
};

struct TestSetupClass : public QObject
{
    TestSetupClass()
    {
        initIntegrationTestMain();

        qmlRegisterSingletonInstance(
            "QtMultimediaTest",
            1,
            0,
            "TestHelper",
            new TestHelper);
    }
};

QUICK_TEST_MAIN_WITH_SETUP(qml, TestSetupClass);

#include "tst_multimedia_integration_qml.moc"
