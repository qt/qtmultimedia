// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAENGINENOTIFY_P_H
#define QMEDIAENGINENOTIFY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/private/qcomobject_p.h>

#include <mfmediaengine.h>

QT_BEGIN_NAMESPACE

class QMediaEngineNotify : public QObject, public QComObject<IMFMediaEngineNotify>
{
    Q_OBJECT
public:
    // IMFMediaEngineNotify
    STDMETHODIMP EventNotify(DWORD event, DWORD_PTR param1, DWORD param2) override;

Q_SIGNALS:
    void eventNotified(DWORD event, DWORD_PTR param1, DWORD param2);
};

QT_END_NAMESPACE

#endif // QMEDIAENGINENOTIFY_P_H
