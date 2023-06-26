// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcapturablewindow.h"
#include "qcapturablewindow_p.h"
#include "qplatformmediaintegration_p.h"

QT_BEGIN_NAMESPACE

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QCapturableWindowPrivate)

/*!
    \class QCapturableWindow
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video
    \since 6.6

    \brief Used for getting the basic information of a capturable window.

    The class contains a set of window information, except the method
    QCapturableWindow::isValid which pulls the current state
    whenever it's called.

    \sa QWindowCapture
*/
/*!
    \qmlvaluetype CapturableWindow
    \instantiates QCapturableWindow
    \brief The CapturableWindow type is used getting basic
    of a window that is available for capturing via WindowCapture.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml
    \since 6.6

    The class contains a dump of window information, except the property
    'isValid' which pulls the actual window state every time.

    \sa WindowCapture
*/

/*!
    \fn  QCapturableWindow::QCapturableWindow(QCapturableWindow &&other)

    Constructs a QCapturableWindow by moving from \a other.
*/

/*!
    \fn void QCapturableWindow::swap(QCapturableWindow &other) noexcept

    Swaps the current window information with \a other.
*/

/*!
    \fn  QCapturableWindow &QCapturableWindow::operator=(QCapturableWindow &&other)

    Moves \a other into this QCapturableWindow.
*/

/*!
    Constructs a null capturable window information that doesn't refer to any window.
*/
QCapturableWindow::QCapturableWindow() = default;

/*!
    Destroys the window information.
 */
QCapturableWindow::~QCapturableWindow() = default;

/*!
    Construct a new window information using \a other QCapturableWindow.
*/
QCapturableWindow::QCapturableWindow(const QCapturableWindow &other) = default;

/*!
    Assigns the \a other window information to this QCapturableWindow.
*/
QCapturableWindow& QCapturableWindow::operator=(const QCapturableWindow &other) = default;

/*!
    \fn bool QCapturableWindow::operator==(const QCapturableWindow &lhs, const QCapturableWindow &rhs)

    Returns \c true if window information \a lhs and \a rhs refer to the same window,
    otherwise returns \c false.
*/

/*!
    \fn bool QCapturableWindow::operator!=(const QCapturableWindow &lhs, const QCapturableWindow &rhs)

    Returns \c true if window information \a lhs and \a rhs refer to different windows,
    otherwise returns \c false.
*/
bool operator==(const QCapturableWindow &lhs, const QCapturableWindow &rhs) noexcept
{
    return lhs.d == rhs.d || (lhs.d && rhs.d && lhs.d->id == rhs.d->id);
}

/*!
    \qmlproperty string QtMultimedia::CapturableWindow::isValid

    This property identifies whether a window information is valid.

    An invalid window information refers to non-existing window or doesn't refer to any one.
*/

/*!
    Identifies whether a window information is valid.

    An invalid window information refers to non-existing window or doesn't refer to any one.

    Returns true if the window is valid, and false if it is not.
*/
bool QCapturableWindow::isValid() const
{
    return d && QPlatformMediaIntegration::instance()->isCapturableWindowValid(*d);
}

/*!
    \qmlproperty string QtMultimedia::CapturableWindow::description

    This property holds the description of the reffered window.
*/

/*!
    Returns a description of the window. In most cases it represents the window title.
*/
QString QCapturableWindow::description() const
{
    if (!d)
        return {};

    if (d->description.isEmpty() && d->id)
        return QLatin1String("Window 0x") + QString::number(d->id, 16);

    return d->description;
}

QCapturableWindow::QCapturableWindow(QCapturableWindowPrivate *capturablePrivate)
    : d(capturablePrivate)
{
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QCapturableWindow &window)
{
    dbg << QString::fromUtf8("Capturable window '%1'").arg(window.description());
    return dbg;
}
#endif


QT_END_NAMESPACE

#include "moc_qcapturablewindow.cpp"
