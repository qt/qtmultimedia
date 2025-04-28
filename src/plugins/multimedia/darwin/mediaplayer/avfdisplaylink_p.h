// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFDISPLAYLINK_H
#define AVFDISPLAYLINK_H

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
#include <QtCore/qmutex.h>

#if defined(QT_PLATFORM_UIKIT)
#include <CoreVideo/CVBase.h>
#else
#include <QuartzCore/CVDisplayLink.h>
#endif

#include <optional>

QT_BEGIN_NAMESPACE

class AVFDisplayLink final : public QObject
{
    Q_OBJECT
public:
    explicit AVFDisplayLink(QObject *parent = nullptr);
    ~AVFDisplayLink() override;
    bool isValid() const;
    bool isActive() const;

public Q_SLOTS:
    void start();
    void stop();

Q_SIGNALS:
    void tick(const CVTimeStamp &ts);

public:
    void displayLinkEvent(const CVTimeStamp *);

protected:
    bool event(QEvent *) override;

private:
#if defined(QT_PLATFORM_UIKIT)
    void *m_displayLink{};
#else
    CVDisplayLinkRef m_displayLink{};
#endif
    QMutex m_displayLinkMutex;
    bool m_isActive{};
    std::optional<CVTimeStamp> m_frameTimeStamp; // GUARDED_BY(m_displayLinkMutex)
};

QT_END_NAMESPACE

#endif // AVFDISPLAYLINK_H
