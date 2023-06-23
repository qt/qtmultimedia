// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCAPTURABLEWINDOW_P_H
#define QCAPTURABLEWINDOW_P_H

#include <QtGui/qwindowdefs.h>
#include <QtCore/QSharedData>
#include <QtMultimedia/qcapturablewindow.h>

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

QT_BEGIN_NAMESPACE

class QCapturableWindowPrivate : public QSharedData {
public:
    using Id = size_t;

    QString description;
    Id id = 0;

    static const QCapturableWindowPrivate *handle(const QCapturableWindow &window)
    {
        return window.d.get();
    }

    QCapturableWindow create() { return QCapturableWindow(this); }
};

QT_END_NAMESPACE

#endif // QCAPTURABLEWINDOW_P_H
