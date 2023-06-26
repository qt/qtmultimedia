// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QX11CAPTURABLEWINDOWS_P_H
#define QX11CAPTURABLEWINDOWS_P_H

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
#include <mutex>

struct _XDisplay;
typedef struct _XDisplay Display;

QT_BEGIN_NAMESPACE

class QX11CapturableWindows : public QPlatformCapturableWindows
{
public:
    ~QX11CapturableWindows() override;

    QList<QCapturableWindow> windows() const override;

    bool isWindowValid(const QCapturableWindowPrivate &window) const override;

private:
    Display *display() const;

private:
    mutable std::once_flag m_displayOnceFlag;
    mutable Display *m_display = nullptr;
};

QT_END_NAMESPACE

#endif // QX11CAPTURABLEWINDOWS_P_H
