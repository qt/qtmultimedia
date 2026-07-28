// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QINTEGRATIONTESTBASE_P_H
#define QINTEGRATIONTESTBASE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QIntegrationTestBase : public QObject
{
    Q_OBJECT
public:
    static void initMain();
};

QT_END_NAMESPACE

#endif // QINTEGRATIONTESTBASE_P_H
