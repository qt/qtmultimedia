/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef AVFDISPLAYLINK_H
#define AVFDISPLAYLINK_H

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
#include <CoreVideo/CVBase.h>
#else
#include <QuartzCore/CVDisplayLink.h>
#endif

QT_BEGIN_NAMESPACE

class AVFDisplayLink : public QObject
{
    Q_OBJECT
public:
    explicit AVFDisplayLink(QObject *parent = nullptr);
    virtual ~AVFDisplayLink();
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
    virtual bool event(QEvent *) override;

private:
#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
    void *m_displayLink;
#else
    CVDisplayLinkRef m_displayLink;
#endif
    QMutex m_displayLinkMutex;
    bool m_pendingDisplayLinkEvent;
    bool m_isActive;
    CVTimeStamp m_frameTimeStamp;
};

QT_END_NAMESPACE

#endif // AVFDISPLAYLINK_H
