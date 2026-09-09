// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_MULTIMEDIAQUICK_QML_TESTHELPER_H
#define TST_MULTIMEDIAQUICK_QML_TESTHELPER_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class TestHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString mediaBackendName READ mediaBackendName CONSTANT FINAL)

public:
    static QString mediaBackendName();
};

QT_END_NAMESPACE

#endif // TST_MULTIMEDIAQUICK_QML_TESTHELPER_H
