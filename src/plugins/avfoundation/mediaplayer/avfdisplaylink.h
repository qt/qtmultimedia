/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFDISPLAYLINK_H
#define AVFDISPLAYLINK_H

#include <QtCore/qobject.h>
#include <QtCore/qmutex.h>

#if defined(Q_OS_IOS)
#include <CoreVideo/CVBase.h>
#else
#include <QuartzCore/CVDisplayLink.h>
#endif

QT_BEGIN_NAMESPACE

class AVFDisplayLink : public QObject
{
    Q_OBJECT
public:
    explicit AVFDisplayLink(QObject *parent = 0);
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
    virtual bool event(QEvent *);

private:
#if defined(Q_OS_IOS)
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
