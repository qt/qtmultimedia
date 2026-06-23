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

#include <atomic>

#import <QuartzCore/CADisplayLink.h>
#if !defined(QT_PLATFORM_UIKIT)
#include <QuartzCore/CVDisplayLink.h>
#endif

@class QT_MANGLE_NAMESPACE(DisplayLinkObserver);

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
    void tick();

public:
    void displayLinkEvent();

protected:
    bool event(QEvent *) override;

private:
    QT_MANGLE_NAMESPACE(DisplayLinkObserver) *m_observer = {};
#if !defined(QT_PLATFORM_UIKIT)
    CVDisplayLinkRef m_cvDisplayLink{};
#endif
    bool m_isActive{};
    std::atomic<bool> m_framePending{false};
};

QT_END_NAMESPACE

#endif // AVFDISPLAYLINK_H
