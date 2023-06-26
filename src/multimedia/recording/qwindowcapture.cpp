// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowcapture.h"
#include "qplatformmediaintegration_p.h"
#include "qmediacapturesession.h"
#include "private/qobject_p.h"
#include "private/qplatformsurfacecapture_p.h"

QT_BEGIN_NAMESPACE

static QWindowCapture::Error toWindowCaptureError(QPlatformSurfaceCapture::Error error)
{
    return static_cast<QWindowCapture::Error>(error);
}

class QWindowCapturePrivate : public QObjectPrivate
{
public:
    QMediaCaptureSession *captureSession = nullptr;
    std::unique_ptr<QPlatformSurfaceCapture> platformWindowCapture;
};

/*!
    \class QWindowCapture
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video
    \since 6.6

    \brief This class is used for capturing a window.

    The class captures a window. It is managed by
    the QMediaCaptureSession class where the captured window can be displayed
    in a video preview object or recorded to a file.

    \sa QMediaCaptureSession, QCapturableWindow
*/
/*!
    \qmltype WindowCapture
    \instantiates QWindowCapture
    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml
    \since 6.6

    \brief This type is used for capturing a window.

    WindowCapture captures a window. It is managed by
    MediaCaptureSession where the captured window can be displayed
    in a video preview object or recorded to a file.

    \sa CaptureSession, CapturableWindow
*/

/*!
    \enum QWindowCapture::Error

    Enumerates error codes that can be signaled by the QWindowCapture class.
    errorString() provides detailed information about the error cause.

    \value NoError                      No error
    \value InternalError                Internal window capturing driver error
    \value CapturingNotSupported        Window capturing is not supported
    \value CaptureFailed                Capturing window failed
    \value NotFound                     Selected window not found
*/

/*!
    Constructs a new QWindowCapture object with \a parent.
*/
QWindowCapture::QWindowCapture(QObject *parent) : QObject(*new QWindowCapturePrivate, parent)
{
    Q_D(QWindowCapture);

    qRegisterMetaType<QCapturableWindow>();

    auto platformCapture = QPlatformMediaIntegration::instance()->createWindowCapture(this);

    if (platformCapture) {
        connect(platformCapture, &QPlatformSurfaceCapture::activeChanged, this,
                &QWindowCapture::activeChanged);
        connect(platformCapture, &QPlatformSurfaceCapture::errorChanged, this,
                &QWindowCapture::errorChanged);
        connect(platformCapture, &QPlatformSurfaceCapture::errorOccurred, this,
                [this](QPlatformSurfaceCapture::Error error, QString errorString) {
                    emit errorOccurred(toWindowCaptureError(error), errorString);
                });
        connect(platformCapture,
                qOverload<QCapturableWindow>(&QPlatformSurfaceCapture::sourceChanged), this,
                &QWindowCapture::windowChanged);

        d->platformWindowCapture.reset(platformCapture);
    }
}

/*!
    Destroys the object.
 */
QWindowCapture::~QWindowCapture()
{
    Q_D(QWindowCapture);

    d->platformWindowCapture.reset();

    if (d->captureSession)
        d->captureSession->setWindowCapture(nullptr);
}

/*!
    \qmlmethod list<CapturableWindow> QtMultimedia::WindowCapture::capturableWindows()

    Returns a list of CapturableWindow objects that is available for capturing.
*/
/*!
    \fn QList<QCapturableWindow> QWindowCapture::capturableWindows()

    Returns a list of QCapturableWindow objects that is available for capturing.
 */
QList<QCapturableWindow> QWindowCapture::capturableWindows()
{
    return QPlatformMediaIntegration::instance()->capturableWindows();
}

QMediaCaptureSession *QWindowCapture::captureSession() const
{
    Q_D(const QWindowCapture);

    return d->captureSession;
}

/*!
    \qmlproperty Window QtMultimedia::WindowCapture::window
    Describes the window for capturing.

    \sa QtMultimedia::WindowCapture::capturableWindows
*/

/*!
    \property QWindowCapture::window
    \brief the window for capturing.

    \sa QWindowCapture::capturableWindows
*/
QCapturableWindow QWindowCapture::window() const
{
    Q_D(const QWindowCapture);

    return d->platformWindowCapture ? d->platformWindowCapture->source<QCapturableWindow>()
                                    : QCapturableWindow();
}

void QWindowCapture::setWindow(QCapturableWindow window)
{
    Q_D(QWindowCapture);

    if (d->platformWindowCapture)
        d->platformWindowCapture->setSource(window);
}

/*!
    \qmlproperty bool QtMultimedia::WindowCapture::active
    Describes whether the capturing is currently active.
*/

/*!
    \property QWindowCapture::active
    \brief whether the capturing is currently active.

    \sa start(), stop()
*/
bool QWindowCapture::isActive() const
{
    Q_D(const QWindowCapture);

    return d->platformWindowCapture && d->platformWindowCapture->isActive();
}

void QWindowCapture::setActive(bool active)
{
    Q_D(QWindowCapture);

    if (d->platformWindowCapture)
        d->platformWindowCapture->setActive(active);
}

/*!
    \qmlmethod QtMultimedia::WindowCapture::start
*/

/*!
    \fn void QWindowCapture::start()

    Starts capturing the \l window.

    This is equivalent to setting the \l active property to true.
*/

/*!
    \qmlmethod QtMultimedia::WindowCapture::stop
*/

/*!
    \fn void QWindowCapture::stop()

    Stops capturing.

    This is equivalent to setting the \l active property to false.
*/


/*!
    \qmlproperty string QtMultimedia::WindowCapture::error
    Returns a code of the last error.
*/

/*!
    \property QWindowCapture::error
    \brief the code of the last error.
*/
QWindowCapture::Error QWindowCapture::error() const
{
    Q_D(const QWindowCapture);

    return d->platformWindowCapture ? toWindowCaptureError(d->platformWindowCapture->error())
                                    : CapturingNotSupported;
}

/*!
    \fn void QWindowCapture::errorOccurred(QWindowCapture::Error error, const QString &errorString)

    Signals when an \a error occurs, along with the \a errorString.
*/
/*!
    \qmlproperty string QtMultimedia::WindowCapture::errorString
    Returns a human readable string describing the cause of error.
*/

/*!
    \property QWindowCapture::errorString
    \brief a human readable string describing the cause of error.
*/
QString QWindowCapture::errorString() const
{
    Q_D(const QWindowCapture);

    return d->platformWindowCapture
            ? d->platformWindowCapture->errorString()
            : QLatin1StringView("Capturing is not support on this platform");
}

void QWindowCapture::setCaptureSession(QMediaCaptureSession *captureSession)
{
    Q_D(QWindowCapture);

    d->captureSession = captureSession;
}

QPlatformSurfaceCapture *QWindowCapture::platformWindowCapture() const
{
    Q_D(const QWindowCapture);

    return d->platformWindowCapture.get();
}

QT_END_NAMESPACE

#include "moc_qwindowcapture.cpp"
