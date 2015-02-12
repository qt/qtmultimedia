/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfdisplaylink.h"
#include <QtCore/qcoreapplication.h>

#ifdef QT_DEBUG_AVF
#include <QtCore/qdebug.h>
#endif

#if defined(Q_OS_IOS)
#import <QuartzCore/CADisplayLink.h>
#import <Foundation/NSRunLoop.h>
#define _m_displayLink static_cast<DisplayLinkObserver*>(m_displayLink)
#else
#endif

QT_USE_NAMESPACE

#if defined(Q_OS_IOS)
@interface DisplayLinkObserver : NSObject
{
    AVFDisplayLink *m_avfDisplayLink;
    CADisplayLink *m_displayLink;
}

- (void)start;
- (void)stop;
- (void)displayLinkNotification:(CADisplayLink *)sender;

@end

@implementation DisplayLinkObserver

- (id)initWithAVFDisplayLink:(AVFDisplayLink *)link
{
    self = [super init];

    if (self) {
        m_avfDisplayLink = link;
        m_displayLink = [[CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkNotification:)] retain];
    }

    return self;
}

- (void) dealloc
{
    if (m_displayLink) {
        [m_displayLink release];
        m_displayLink = NULL;
    }

    [super dealloc];
}

- (void)start
{
    [m_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)stop
{
    [m_displayLink removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
}

- (void)displayLinkNotification:(CADisplayLink *)sender
{
    Q_UNUSED(sender);
    m_avfDisplayLink->displayLinkEvent(nullptr);
}

@end
#else
static CVReturn CVDisplayLinkCallback(CVDisplayLinkRef displayLink,
                                 const CVTimeStamp *inNow,
                                 const CVTimeStamp *inOutputTime,
                                 CVOptionFlags flagsIn,
                                 CVOptionFlags *flagsOut,
                                 void *displayLinkContext)
{
    Q_UNUSED(displayLink);
    Q_UNUSED(inNow);
    Q_UNUSED(flagsIn);
    Q_UNUSED(flagsOut);

    AVFDisplayLink *link = (AVFDisplayLink *)displayLinkContext;

    link->displayLinkEvent(inOutputTime);
    return kCVReturnSuccess;
}
#endif

AVFDisplayLink::AVFDisplayLink(QObject *parent)
    : QObject(parent)
    , m_displayLink(0)
    , m_pendingDisplayLinkEvent(false)
    , m_isActive(false)
{
#if defined(Q_OS_IOS)
    m_displayLink = [[DisplayLinkObserver alloc] initWithAVFDisplayLink:this];
#else
    // create display link for the main display
    CVDisplayLinkCreateWithCGDisplay(kCGDirectMainDisplay, &m_displayLink);
    if (m_displayLink) {
        // set the current display of a display link.
        CVDisplayLinkSetCurrentCGDisplay(m_displayLink, kCGDirectMainDisplay);

        // set the renderer output callback function
        CVDisplayLinkSetOutputCallback(m_displayLink, &CVDisplayLinkCallback, this);
    }
#endif
}

AVFDisplayLink::~AVFDisplayLink()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif

    if (m_displayLink) {
        stop();
#if defined(Q_OS_IOS)
        [_m_displayLink release];
#else
        CVDisplayLinkRelease(m_displayLink);
#endif
        m_displayLink = NULL;
    }
}

bool AVFDisplayLink::isValid() const
{
    return m_displayLink != 0;
}

bool AVFDisplayLink::isActive() const
{
    return m_isActive;
}

void AVFDisplayLink::start()
{
    if (m_displayLink && !m_isActive) {
#if defined(Q_OS_IOS)
        [_m_displayLink start];
#else
        CVDisplayLinkStart(m_displayLink);
#endif
        m_isActive = true;
    }
}

void AVFDisplayLink::stop()
{
    if (m_displayLink && m_isActive) {
#if defined(Q_OS_IOS)
        [_m_displayLink stop];
#else
        CVDisplayLinkStop(m_displayLink);
#endif
        m_isActive = false;
    }
}

void AVFDisplayLink::displayLinkEvent(const CVTimeStamp *ts)
{
    // This function is called from a
    // thread != gui thread. So we post the event.
    // But we need to make sure that we don't post faster
    // than the event loop can eat:
    m_displayLinkMutex.lock();
    bool pending = m_pendingDisplayLinkEvent;
    m_pendingDisplayLinkEvent = true;
#if defined(Q_OS_IOS)
    Q_UNUSED(ts);
    memset(&m_frameTimeStamp, 0, sizeof(CVTimeStamp));
#else
    m_frameTimeStamp = *ts;
#endif
    m_displayLinkMutex.unlock();

    if (!pending)
        qApp->postEvent(this, new QEvent(QEvent::User), Qt::HighEventPriority);
}

bool AVFDisplayLink::event(QEvent *event)
{
    switch (event->type()){
        case QEvent::User:  {
                m_displayLinkMutex.lock();
                m_pendingDisplayLinkEvent = false;
                CVTimeStamp ts = m_frameTimeStamp;
                m_displayLinkMutex.unlock();

                Q_EMIT tick(ts);

                return false;
            }
            break;
        default:
            break;
    }
    return QObject::event(event);
}
