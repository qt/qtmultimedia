// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/qwindowcapture.h>

#include <QtCore/private/qobject_p.h>

#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>

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

    \include qwindowcapture-limitations.qdocinc {content} {Q}

    \sa QMediaCaptureSession, QCapturableWindow
*/
/*!
    \qmltype WindowCapture
    \nativetype QWindowCapture
    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml
    \since 6.6

    \brief This type is used for capturing a window.

    WindowCapture captures a window. It is managed by
    MediaCaptureSession where the captured window can be displayed
    in a video preview object or recorded to a file.

    \include qwindowcapture-limitations.qdocinc {content} {}

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
                [this](QPlatformSurfaceCapture::Error error, const QString &errorString) {
            emit errorOccurred(toWindowCaptureError(error), errorString);
        });
        connect(platformCapture,
                qOverload<QCapturableWindow>(&QPlatformSurfaceCapture::sourceChanged), this,
                &QWindowCapture::windowChanged);

        connect(platformCapture, &QPlatformSurfaceCapture::frameRateChanged, this,
                &QWindowCapture::frameRateChanged);

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

    \note On macOS, invoking this method will trigger the "Screen Recording" permission
    dialog. If permissions have not yet been granted, this method will return an empty list.
    Invoking it multiple times will bring this dialog to the foreground.
*/
/*!
    \fn QList<QCapturableWindow> QWindowCapture::capturableWindows()

    Returns a list of QCapturableWindow objects that is available for capturing.

    \note On macOS, invoking this method will trigger the "Screen Recording" permission
    dialog. If permissions have not yet been granted, this method will return an empty list.
    Invoking it multiple times will bring this dialog to the foreground.
 */
QList<QCapturableWindow> QWindowCapture::capturableWindows()
{
    return QPlatformMediaIntegration::instance()->capturableWindowsList();
}

QMediaCaptureSession *QWindowCapture::captureSession() const
{
    Q_D(const QWindowCapture);

    return d->captureSession;
}

/*!
    \qmlproperty Window QtMultimedia::WindowCapture::window
    Describes the window for capturing.

    Setting this property to an invalid window on an active
    WindowCapture will cause it to go inactive and emit
    an error.

    \sa capturableWindows
*/

/*!
    \property QWindowCapture::window
    \brief the window for capturing.

    Setting this property to an invalid window on an active
    QWindowCapture will cause it to go inactive and emit
    an error.

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
    \since 6.12
    \property QWindowCapture::frameRate
    \brief The target window capture framerate.

    Actual frame rate depends on the platform. For platforms with fixed rate capture, this
    frame rate is followed. For platforms with variable rate capture, this frame rate is either
    used as the polling rate (maximum frame rate) or completely ignored.

    If left unset, a platform-dependent default is used.

    Any changes to this property are applied the next time the QWindowCapture goes active.
*/
void QWindowCapture::setFrameRate(std::optional<qreal> frameRate)
{
    Q_D(QWindowCapture);

    if (d->platformWindowCapture)
        d->platformWindowCapture->setFrameRate(frameRate);
}

std::optional<qreal> QWindowCapture::frameRate() const
{
    Q_D(const QWindowCapture);

    return d->platformWindowCapture ? d->platformWindowCapture->frameRate() : std::nullopt;
}

void QWindowCapture::resetFrameRate()
{
    setFrameRate(std::nullopt);
}

/*!
    \qmlmethod void QtMultimedia::WindowCapture::start()

    Starts capturing the \l window.

    This is equivalent to setting the \l active property to \c true.
*/

/*!
    \fn void QWindowCapture::start()

    Starts capturing the \l window.

    This is equivalent to setting the \l active property to true.
*/

/*!
    \qmlmethod void QtMultimedia::WindowCapture::stop()

    Stops capturing.

    This is equivalent to setting the \l active property to \c false.
*/

/*!
    \fn void QWindowCapture::stop()

    Stops capturing.

    This is equivalent to setting the \l active property to false.
*/

/*!
    \qmlsignal QtMultimedia::WindowCapture::errorChanged()

    This signal is emitted when the \l{error} or \l{errorString} properties are changed.

    This signal is not emitted whenever multiple identical errors are raised. To track such
    errors, use the signal \l errorOccurred.
*/

/*!
    \fn void QWindowCapture::errorChanged()

    This signal is emitted when the \l{error} or \l{errorString} properties are changed.

    This signal is not emitted whenever multiple identical errors are raised. To track such
    errors, use the signal \l errorOccurred.
*/

/*!
    \qmlproperty enumeration QtMultimedia::WindowCapture::error
    Returns a code of the last error.

    \qmlenumeratorsfrom QWindowCapture::Error
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
    \qmlsignal QtMultimedia::WindowCapture::errorOccurred(int error, string errorString)

    Signals when an \a error occurs, along with the \a errorString.

    For the error parameter, see the enumeration table in
    \l {QtMultimedia::WindowCapture::error}{error} for what values may
    be passed.

    \sa {QtMultimedia::WindowCapture::error}{error}
*/
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
            : QLatin1StringView("Capturing is not supported on this platform");
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
