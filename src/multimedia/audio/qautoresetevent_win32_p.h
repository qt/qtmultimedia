// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QAUTORESETEVENT_WIN32_P_H
#define QAUTORESETEVENT_WIN32_P_H

#include <QtCore/qwineventnotifier.h>
#include <QtMultimedia/qtmultimediaexports.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

class Q_MULTIMEDIA_EXPORT QAutoResetEventWin32 : public QObject
{
    Q_OBJECT

public:
    explicit QAutoResetEventWin32(QObject *parent = nullptr);
    ~QAutoResetEventWin32();

    bool isValid() const;
    void set();

Q_SIGNALS:
    void activated();

private:
    QWinEventNotifier m_notifier;
    Qt::HANDLE m_handle;
};

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QAUTORESETEVENT_WIN32_P_H
