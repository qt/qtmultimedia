// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfdisplaylink_p.h"
#include <QtCore/qcoreapplication.h>

#include <mutex>

#ifdef QT_DEBUG_AVF
#include <QtCore/qdebug.h>
#endif

#if defined(QT_PLATFORM_UIKIT)
#import <QuartzCore/CADisplayLink.h>
#import <Foundation/NSRunLoop.h>
#define _m_displayLink static_cast<DisplayLinkObserver*>(m_displayLink)
#else
#endif

QT_USE_NAMESPACE

#if defined(QT_PLATFORM_UIKIT)
@interface DisplayLinkObserver : NSObject

- (void)start;
- (void)stop;
- (void)displayLinkNotification:(CADisplayLink *)sender;

@end

@implementation DisplayLinkObserver
{
    AVFDisplayLink *m_avfDisplayLink;
    CADisplayLink *m_displayLink;
}

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
        m_displayLink = nullptr;
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
static CVReturn CVDisplayLinkCallback([[maybe_unused]] CVDisplayLinkRef displayLink,
                                      [[maybe_unused]] const CVTimeStamp *inNow,
                                      const CVTimeStamp *inOutputTime,
                                      [[maybe_unused]] CVOptionFlags flagsIn,
                                      [[maybe_unused]] CVOptionFlags *flagsOut,
                                      void *displayLinkContext)
{
    AVFDisplayLink *link = (AVFDisplayLink *)displayLinkContext;

    link->displayLinkEvent(inOutputTime);
    return kCVReturnSuccess;
}
#endif

AVFDisplayLink::AVFDisplayLink(QObject *parent)
    : QObject(parent)
{
#if defined(QT_PLATFORM_UIKIT)
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
#if defined(QT_PLATFORM_UIKIT)
        [_m_displayLink release];
#else
        CVDisplayLinkRelease(m_displayLink);
#endif
        m_displayLink = nullptr;
    }
}

bool AVFDisplayLink::isValid() const
{
    return m_displayLink != nullptr;
}

bool AVFDisplayLink::isActive() const
{
    return m_isActive;
}

void AVFDisplayLink::start()
{
    if (m_displayLink && !m_isActive) {
#if defined(QT_PLATFORM_UIKIT)
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
#if defined(QT_PLATFORM_UIKIT)
        [_m_displayLink stop];
#else
        CVDisplayLinkStop(m_displayLink);
#endif
        m_isActive = false;

        // cancel pending events
        std::lock_guard guard{ m_displayLinkMutex };
        m_frameTimeStamp = std::nullopt;
    }
}

void AVFDisplayLink::displayLinkEvent(const CVTimeStamp *ts)
{
    // This function is called from a
    // thread != gui thread. So we post the event.
    // But we need to make sure that we don't post faster
    // than the event loop can eat:
    std::unique_lock guard{ m_displayLinkMutex };

    bool pending = m_frameTimeStamp.has_value();
#if defined(QT_PLATFORM_UIKIT)
    Q_UNUSED(ts);
    m_frameTimeStamp = CVTimeStamp{};
#else
    m_frameTimeStamp = *ts;
#endif
    guard.unlock();

    if (!pending)
        qApp->postEvent(this, new QEvent(QEvent::User), Qt::HighEventPriority);
}

bool AVFDisplayLink::event(QEvent *event)
{
    switch (event->type()){
    case QEvent::User: {
        std::unique_lock guard{ m_displayLinkMutex };
        if (!m_frameTimeStamp)
            return false;

        CVTimeStamp ts = *m_frameTimeStamp;
        m_frameTimeStamp = std::nullopt;
        guard.unlock();

        Q_EMIT tick(ts);

        return false;
    }
    default:
        break;
    }
    return QObject::event(event);
}

#include "moc_avfdisplaylink_p.cpp"
