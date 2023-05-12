// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowcapture.h"
#include "qplatformmediaintegration_p.h"

QT_BEGIN_NAMESPACE

class QWindowCapturePrivate : public QObjectData
{
public:
    // TODO add impl
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
QWindowCapture::QWindowCapture(QObject *parent) : QObject(parent) { }

/*!
    Destroys the object.
 */
QWindowCapture::~QWindowCapture() = default;

/*!
    \qmlmethod list<CapturableWindow> QtMultimedia::CapturableWindow::capturableWindows()

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
    return nullptr;
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
    return {};
}

void QWindowCapture::setWindow(QCapturableWindow /*window*/) { }

/*!
    \qmlproperty bool QtMultimedia::WindowCapture::active
    Describes whether the capturing is currently active.
*/

/*!
    \property QWindowCapture::active
    \brief whether the capturing is currently active.
*/
bool QWindowCapture::isActive() const
{
    return false;
}

void QWindowCapture::setActive(bool /*active*/) { }

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
    return NoError;
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
    return {};
}

QT_END_NAMESPACE

#include "moc_qwindowcapture.cpp"
