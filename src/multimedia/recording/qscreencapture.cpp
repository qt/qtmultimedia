// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qscreencapture.h"
#include "qmediacapturesession.h"
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformsurfacecapture_p.h>
#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

static QScreenCapture::Error toScreenCaptureError(QPlatformSurfaceCapture::Error error)
{
    return static_cast<QScreenCapture::Error>(error);
}

class QScreenCapturePrivate : public QObjectPrivate
{
public:
    QMediaCaptureSession *captureSession = nullptr;
    std::unique_ptr<QPlatformSurfaceCapture> platformScreenCapture;
};

/*!
    \class QScreenCapture
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video
    \since 6.5

    \brief This class is used for capturing a screen.

    The class captures a screen. It is managed by
    the QMediaCaptureSession class where the captured screen can be displayed
    in a video preview object or recorded to a file.

    \snippet multimedia-snippets/media.cpp Media recorder

    \include qscreencapture-limitations.qdocinc {content} {Q}

    \sa QWindowCapture, QMediaCaptureSession
*/
/*!
    \qmltype ScreenCapture
    \nativetype QScreenCapture
    \brief This type is used for capturing a screen.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml

    ScreenCapture captures a screen. It is managed by
    MediaCaptureSession where the captured screen can be displayed
    in a video preview object or recorded to a file.

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

    \include qscreencapture-limitations.qdocinc {content} {}

    \sa WindowCapture, CaptureSession
*/

QScreenCapture::QScreenCapture(QObject *parent)
    : QObject(*new QScreenCapturePrivate, parent)
{
    Q_D(QScreenCapture);

    auto platformCapture = QPlatformMediaIntegration::instance()->createScreenCapture(this);
    if (platformCapture) {
        connect(platformCapture, &QPlatformSurfaceCapture::activeChanged, this,
                &QScreenCapture::activeChanged);
        connect(platformCapture, &QPlatformSurfaceCapture::errorChanged, this,
                &QScreenCapture::errorChanged);
        connect(platformCapture, &QPlatformSurfaceCapture::errorOccurred, this,
                [this](QPlatformSurfaceCapture::Error error, QString errorString) {
                    emit errorOccurred(toScreenCaptureError(error), errorString);
                });

        connect(platformCapture,
                qOverload<QPlatformSurfaceCapture::ScreenSource>(
                        &QPlatformSurfaceCapture::sourceChanged),
                this, &QScreenCapture::screenChanged);

        d->platformScreenCapture.reset(platformCapture);
    }
}

QScreenCapture::~QScreenCapture()
{
    Q_D(QScreenCapture);

    // Reset platformScreenCapture in the destructor to avoid having broken ref in the object.
    d->platformScreenCapture.reset();

    if (d->captureSession)
        d->captureSession->setScreenCapture(nullptr);
}

/*!
    \enum QScreenCapture::Error

    Enumerates error codes that can be signaled by the QScreenCapture class.
    errorString() provides detailed information about the error cause.

    \value NoError                      No error
    \value InternalError                Internal screen capturing driver error
    \value CapturingNotSupported        Capturing is not supported
    \value CaptureFailed                Capturing screen failed
    \value NotFound                     Selected screen not found
*/

/*!
    Returns the capture session this QScreenCapture is connected to.

    Use QMediaCaptureSession::setScreenCapture() to connect the camera to
    a session.
*/
QMediaCaptureSession *QScreenCapture::captureSession() const
{
    Q_D(const QScreenCapture);

    return d->captureSession;
}

/*!
    \qmlproperty bool QtMultimedia::ScreenCapture::active
    Describes whether the capturing is currently active.
*/

/*!
    \property QScreenCapture::active
    \brief whether the capturing is currently active.
*/
void QScreenCapture::setActive(bool active)
{
    Q_D(QScreenCapture);

    if (d->platformScreenCapture)
        d->platformScreenCapture->setActive(active);
}

bool QScreenCapture::isActive() const
{
    Q_D(const QScreenCapture);

    return d->platformScreenCapture && d->platformScreenCapture->isActive();
}

/*!
    \qmlproperty Screen QtMultimedia::ScreenCapture::screen
    Describes the screen for capturing.
*/

/*!
    \property QScreenCapture::screen
    \brief the screen for capturing.
*/

void QScreenCapture::setScreen(QScreen *screen)
{
    Q_D(QScreenCapture);

    if (d->platformScreenCapture)
        d->platformScreenCapture->setSource(QPlatformSurfaceCapture::ScreenSource(screen));
}

QScreen *QScreenCapture::screen() const
{
    Q_D(const QScreenCapture);

    return d->platformScreenCapture
            ? d->platformScreenCapture->source<QPlatformSurfaceCapture::ScreenSource>()
            : nullptr;
}

/*!
    \qmlproperty enumeration QtMultimedia::ScreenCapture::error
    Returns a code of the last error.
*/

/*!
    \property QScreenCapture::error
    \brief the code of the last error.
*/
QScreenCapture::Error QScreenCapture::error() const
{
    Q_D(const QScreenCapture);

    return d->platformScreenCapture ? toScreenCaptureError(d->platformScreenCapture->error())
                                    : CapturingNotSupported;
}

/*!
    \fn void QScreenCapture::errorOccurred(QScreenCapture::Error error, const QString &errorString)

    Signals when an \a error occurs, along with the \a errorString.
*/
/*!
    \qmlproperty string QtMultimedia::ScreenCapture::errorString
    Returns a human readable string describing the cause of error.
*/

/*!
    \property QScreenCapture::errorString
    \brief a human readable string describing the cause of error.
*/
QString QScreenCapture::errorString() const
{
    Q_D(const QScreenCapture);

    return d->platformScreenCapture ? d->platformScreenCapture->errorString()
                                    : QLatin1StringView("Capturing is not support on this platform");
}
/*!
    \fn void QScreenCapture::start()

    Starts screen capture.
*/
/*!
    \fn void QScreenCapture::stop()

    Stops screen capture.
*/
/*!
    \internal
*/
void QScreenCapture::setCaptureSession(QMediaCaptureSession *captureSession)
{
    Q_D(QScreenCapture);

    d->captureSession = captureSession;
}

/*!
    \internal
*/
class QPlatformSurfaceCapture *QScreenCapture::platformScreenCapture() const
{
    Q_D(const QScreenCapture);

    return d->platformScreenCapture.get();
}

QT_END_NAMESPACE

#include "moc_qscreencapture.cpp"
