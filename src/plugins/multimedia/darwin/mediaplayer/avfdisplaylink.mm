// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfdisplaylink_p.h"

#include <QtCore/qcoreapplication.h>

#ifdef QT_DEBUG_AVF
#include <QtCore/qdebug.h>
#endif



#import <Foundation/NSRunLoop.h>
#if !defined(QT_PLATFORM_UIKIT)
#import <AppKit/NSScreen.h>
#endif

QT_USE_NAMESPACE

@interface QT_MANGLE_NAMESPACE (DisplayLinkObserver) : NSObject
{
    AVFDisplayLink *_Nonnull m_avfDisplayLink;
    CADisplayLink *m_displayLink;
}
- (id)initWithAVFDisplayLink:(AVFDisplayLink *_Nonnull)link;
- (void)setDisplayLink:(CADisplayLink *)displayLink;
- (void)start;
- (void)stop;
- (void)displayLinkNotification:(CADisplayLink *)sender;
@end

@implementation QT_MANGLE_NAMESPACE (DisplayLinkObserver)

- (id)initWithAVFDisplayLink:(AVFDisplayLink *_Nonnull)link
{
    self = [super init];
    if (self)
        m_avfDisplayLink = link;
    return self;
}

- (void)dealloc
{
    [self setDisplayLink:nullptr];
    [super dealloc];
}

- (void)setDisplayLink:(CADisplayLink *)displayLink
{
    if (m_displayLink == displayLink)
        return;
    if (m_displayLink) {
        [m_displayLink invalidate];
        [m_displayLink release];
    }
    if (displayLink)
        m_displayLink = [displayLink retain];
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
    m_avfDisplayLink->displayLinkEvent();
}

@end

#ifdef QT_NAMESPACE
using DisplayLinkObserver = QT_MANGLE_NAMESPACE(DisplayLinkObserver);
#endif

AVFDisplayLink::AVFDisplayLink(QObject *parent)
    : QObject(parent)
{
#if defined(QT_PLATFORM_UIKIT)
    m_observer = [[DisplayLinkObserver alloc] initWithAVFDisplayLink:this];
    CADisplayLink *dl = [CADisplayLink displayLinkWithTarget:m_observer
                                                    selector:@selector(displayLinkNotification:)];
    [m_observer setDisplayLink:dl];
#else
    if (@available(macOS 15.0, *)) {
        m_observer = [[DisplayLinkObserver alloc] initWithAVFDisplayLink:this];
        CADisplayLink *_Nonnull dl =
                [NSScreen.mainScreen displayLinkWithTarget:m_observer
                                                  selector:@selector(displayLinkNotification:)];
        [m_observer setDisplayLink:dl];
        return;
    }
    if (!m_observer) {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_DEPRECATED
        CVDisplayLinkCreateWithCGDisplay(kCGDirectMainDisplay, &m_cvDisplayLink);
        if (m_cvDisplayLink) {
            CVDisplayLinkSetCurrentCGDisplay(m_cvDisplayLink, kCGDirectMainDisplay);
            CVDisplayLinkSetOutputCallback(m_cvDisplayLink,
                                           [](CVDisplayLinkRef, const CVTimeStamp *,
                                              const CVTimeStamp *, CVOptionFlags, CVOptionFlags *,
                                              void *displayLinkContext) -> CVReturn {
                static_cast<AVFDisplayLink *>(displayLinkContext)->displayLinkEvent();
                return kCVReturnSuccess;
            },
                                           this);
        }
        QT_WARNING_POP
    }
#endif
}

AVFDisplayLink::~AVFDisplayLink()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif

    stop();

    if (m_observer) {
        [m_observer release];
        m_observer = nil;
    }

#if !defined(QT_PLATFORM_UIKIT)
    if (m_cvDisplayLink) {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_DEPRECATED
        CVDisplayLinkRelease(m_cvDisplayLink);
        QT_WARNING_POP
        m_cvDisplayLink = nullptr;
    }
#endif
}

bool AVFDisplayLink::isValid() const
{
#if !defined(QT_PLATFORM_UIKIT)
    return m_observer || m_cvDisplayLink != nullptr;
#else
    return m_observer;
#endif
}

bool AVFDisplayLink::isActive() const
{
    return m_isActive;
}

void AVFDisplayLink::start()
{
    if (!m_isActive) {
        if (m_observer)
            [m_observer start];
#if !defined(QT_PLATFORM_UIKIT)
        else if (m_cvDisplayLink) {
            QT_WARNING_PUSH
            QT_WARNING_DISABLE_DEPRECATED
            CVDisplayLinkStart(m_cvDisplayLink);
            QT_WARNING_POP
        }
#endif
        m_isActive = true;
    }
}

void AVFDisplayLink::stop()
{
    if (m_isActive) {
        if (m_observer)
            [m_observer stop];
#if !defined(QT_PLATFORM_UIKIT)
        else if (m_cvDisplayLink) {
            QT_WARNING_PUSH
            QT_WARNING_DISABLE_DEPRECATED
            CVDisplayLinkStop(m_cvDisplayLink);
            QT_WARNING_POP
        }
#endif
        m_framePending = false;
        m_isActive = false;
    }
}

void AVFDisplayLink::displayLinkEvent()
{
    if (!m_framePending.exchange(true))
        qApp->postEvent(this, new QEvent(QEvent::User), Qt::HighEventPriority);
}

bool AVFDisplayLink::event(QEvent *event)
{
    switch (event->type()){
    case QEvent::User: {
        if (!m_framePending.exchange(false))
            return false;

        Q_EMIT tick();

        return false;
    }
    default:
        break;
    }
    return QObject::event(event);
}

#include "moc_avfdisplaylink_p.cpp"
