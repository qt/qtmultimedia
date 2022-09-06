// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qscreencapture.h"
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformscreencapture_p.h>

QT_BEGIN_NAMESPACE

struct QScreenCapturePrivate
{
    QMediaCaptureSession *captureSession = nullptr;
    QPlatformScreenCapture *platformScreenCapture = nullptr;
};

/*!
    \class QScreenCapture
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video

    \brief The QScreenCapture class is used for capturing a screen view or
    a window view.

    The class captures a screen view or window view. It is managed by
    the QMediaCaptureSession class where the captured view can be displayed
    in a window or recorded to a file.

    \snippet multimedia-snippets/media.cpp Media recorder
*/
/*!
    \qmltype ScreenCapture
    \instantiates QScreenCapture
    \brief The ScreenCapture type is used for capturing a screen view or
    a window view.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml

    ScreenCapture captures a screen view or a window view. It is managed by
    MediaCaptureSession where the captured view can be displayed in a window
    or recorded to a file.

    \since 6.5
    The code below shows a simple capture session with ScreenCapture playing
    back the captured primary screen view in VideoOutput.

\qml
    CaptureSession {
        id: captureSession
        screenCapture: ScreenCapture {
            id: screenCapture
            active: true
        }
        videoOutput: VideoOutput {
            id: videoOutput
        }
    }
\endqml

    \sa ScreenCapture, CaptureSession
*/

QScreenCapture::QScreenCapture(QObject *parent)
    : QObject(parent)
    , d(new QScreenCapturePrivate)
{
    d->platformScreenCapture = QPlatformMediaIntegration::instance()->createScreenCapture(this);
}

QScreenCapture::~QScreenCapture()
{
    delete d->platformScreenCapture;
    delete d;
}

/*!
    \enum QScreenCapture::Error

    Enumerates error codes that can be signaled by the QScreenCapture class.
    errorString() provides detailed information about the error cause.

    \value NoError                      No error
    \value InternalError                Internal screen capturing driver error
    \value CapturingNotSupported        Capturing is not supported
    \value WindowCapturingNotSupported  Window capturing is not supported
    \value CaptureFailed                Capturing screen or window view failed
    \value NotFound                     Selected screen or window not found
*/

/*!
    Returns the capture session this QScreenCapture is connected to.

    Use QMediaCaptureSession::setScreenCapture() to connect the camera to
    a session.
*/
QMediaCaptureSession *QScreenCapture::captureSession() const
{
    return d->captureSession;
}

/*!
    \qmlproperty Window QtMultimedia::ScreenCapture::window
    Describes the window for capturing.
*/

/*!
    \property QScreenCapture::window
    Describes the window for capturing.
*/
void QScreenCapture::setWindow(QWindow *window)
{
    if (d->platformScreenCapture) {
        d->platformScreenCapture->setScreen(nullptr);
        d->platformScreenCapture->setWindowId(0);
        d->platformScreenCapture->setWindow(window);
    }
}

QWindow *QScreenCapture::window() const
{
    return d->platformScreenCapture ? d->platformScreenCapture->window()
                                    : nullptr;
}

/*!
    \qmlproperty Window QtMultimedia::ScreenCapture::windowId
    Describes the window ID for capturing.
*/

/*!
    \property QScreenCapture::windowId
    Describes the window ID for capturing.
*/
void QScreenCapture::setWindowId(WId id)
{
    if (d->platformScreenCapture) {
        d->platformScreenCapture->setScreen(nullptr);
        d->platformScreenCapture->setWindow(nullptr);
        d->platformScreenCapture->setWindowId(id);
    }
}

WId QScreenCapture::windowId() const
{
    return d->platformScreenCapture ? d->platformScreenCapture->windowId()
                                    : 0;
}

/*!
    \qmlproperty bool QtMultimedia::ScreenCapture::active
    Describes whether the capturing is currently active.
*/

/*!
    \property QScreenCapture::active
    Describes whether the capturing is currently active.
*/
void QScreenCapture::setActive(bool active)
{
    if (d->platformScreenCapture)
        d->platformScreenCapture->setActive(active);
}

bool QScreenCapture::isActive() const
{
    return d->platformScreenCapture && d->platformScreenCapture->isActive();
}

/*!
    \qmlproperty bool QtMultimedia::ScreenCapture::screen
    Describes the screen for capturing.
*/

/*!
    \property QScreenCapture::screen
    Describes the screen for capturing.
*/

void QScreenCapture::setScreen(QScreen *screen)
{
    if (d->platformScreenCapture) {
        d->platformScreenCapture->setWindow(nullptr);
        d->platformScreenCapture->setWindowId(0);
        d->platformScreenCapture->setScreen(screen);
    }
}

QScreen *QScreenCapture::screen() const
{
    return d->platformScreenCapture ? d->platformScreenCapture->screen()
                                    : nullptr;
}

/*!
    \qmlproperty string QScreenCapture::ScreenCapture::error
    Returns a code of the last error.
*/

/*!
    \property QScreenCapture::error
    Returns a code of the last error.
*/
QScreenCapture::Error QScreenCapture::error() const
{
    return d->platformScreenCapture ? d->platformScreenCapture->error()
                                    : CapturingNotSupported;
}

/*!
    \qmlproperty string QScreenCapture::ScreenCapture::errorString
    Returns a human readable string describing the cause of error.
*/

/*!
    \property QScreenCapture::errorString
    Returns a human readable string describing the cause of error.
*/
QString QScreenCapture::errorString() const
{
    return d->platformScreenCapture ? d->platformScreenCapture->errorString()
                                    : QLatin1StringView("Capturing is not support on this platform");
}

/*!
    \internal
*/
void QScreenCapture::setCaptureSession(QMediaCaptureSession *captureSession)
{
    d->captureSession = captureSession;
}

/*!
    \internal
*/
class QPlatformScreenCapture *QScreenCapture::platformScreenCapture() const
{
    return d->platformScreenCapture;
}

QT_END_NAMESPACE
