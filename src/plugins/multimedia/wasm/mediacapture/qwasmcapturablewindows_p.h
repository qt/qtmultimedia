// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMCAPTURABLEWINDOWS_P_H
#define QWASMCAPTURABLEWINDOWS_P_H

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

#include "private/qplatformcapturablewindows_p.h"
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

class QWasmCapturableWindows : public QPlatformCapturableWindows
{
public:
    QWasmCapturableWindows();
    QList<QCapturableWindow> windows() const override;

    bool isWindowValid(const QCapturableWindowPrivate &window) const override;
private:
    QList<QCapturableWindow> m_capurableWindows;
    void getDisplayMedia();
    static void getDisplayMedia(emscripten::val event);

};


QT_END_NAMESPACE
#endif // QWASMCAPTURABLEWINDOWS_P_H
