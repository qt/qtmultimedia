// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINCAPTURABLEWINDOWS_P_H
#define QWINCAPTURABLEWINDOWS_P_H

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

#include <QtMultimedia/private/qplatformcapturablewindows_p.h>

QT_BEGIN_NAMESPACE

class QWinCapturableWindows : public QPlatformCapturableWindows
{
public:
    QList<QCapturableWindow> windows() const override;

    bool isWindowValid(const QCapturableWindowPrivate &window) const override;
};

QT_END_NAMESPACE

#endif // QWINCAPTURABLEWINDOWS_P_H
