// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideosource.h"
#include "qgstreamervideosource_p.h"

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class QGStreamerVideoSource
    \since 6.12

    \brief The QGStreamerVideoSource class provides a video source backed by
    a custom GStreamer element.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video

    \warning The GStreamerVideoSource class offers limited compatibility guarantees.
    There are no source or binary compatibility guarantees for these classes,
    meaning the API is only guaranteed to work with the Qt version the application
    has been developed against. However, incompatible changes are aimed to be kept
    at a minimum and will only be made in minor releases.
    To use the class in an application, link to \c Qt::MultimediaPrivate (if using CMake),
    and include the header #include <spi/qgstreamervideosource.h>.

    Construct QGStreamerVideoSource from a GStreamer bin description
    or from an existing \c GstElement and attach it to
    QMediaCaptureSession by \l QMediaCaptureSession::setNativeVideoSource().

    For any standard case, we recommend using QCamera instead of this class,
    unless you need GStreamer-specific configuration to handle any custom
    video source.

    By default, the class is available in meta-qt6 builds. In custom builds
    with GStreamer support, the \c gstreamer_qt_api feature can enable it.

    QtMultimedia must be run with the \c gstreamer media backend for this class to work.

    \code
    QMediaCaptureSession session;
    QGStreamerVideoSource videoSource(QStringLiteral("videotestsrc name=testsrc"));

    session.setNativeVideoSource(&videoSource);
    videoSource.start();
    \endcode

    \sa QMediaCaptureSession, QCamera
*/

/*!
    \qmltype GStreamerVideoSource
    \nativetype QGStreamerVideoSource
    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml

    \brief A video source backed by a custom GStreamer bin.

    Construct the video source from a GStreamer bin description
    and attach it to CaptureSession using its
    \l{CaptureSession::}{nativeVideoSource} property.

    For any standard case, we recommend using \l Camera instead of this type,
    unless you need GStreamer-specific configuration to handle any custom
    video source.

    By default, the type is available only in meta-qt6 builds. In custom builds
    with GStreamer support, the \c gstreamer_qt_api feature can enable it.

    QtMultimedia must be run with the \c gstreamer media backend for this class to work.

\qml
    Item {
        CaptureSession {
            nativeVideoSource: GStreamerVideoSource {
                gstElementDescription: "videotestsrc name=testsrc"
                active: true
            }
            videoOutput: preview
        }

        VideoOutput {
            id: preview
        }
    }
\endqml

    \sa CaptureSession, Camera
*/


void QGStreamerVideoSourcePrivate::createPlatformCamera(QGStreamerVideoSource *source,
                                                        GstElementOrDescription elementOrDesc,
                                                        bool active)
{
    Q_ASSERT(!platformCamera);

    auto maybePlatformCamera = QPlatformMediaIntegration::instance()->createGStreamerVideoSource(
            source, elementOrDesc);
    if (!maybePlatformCamera) {
        qWarning() << "Failed to initialize QGStreamerVideoSource" << maybePlatformCamera.error();
        return;
    }

    if (auto gstBinDesc = std::get_if<QString>(&elementOrDesc))
        gstBinDescription = std::move(*gstBinDesc);

    platformCamera = *maybePlatformCamera;

    if (active)
        platformCamera->setActive(true);

    QObject::connect(platformCamera, &QPlatformVideoSource::activeChanged, source,
                     &QGStreamerVideoSource::activeChanged);
}

/*!
    Constructs a QGStreamerVideoSource from the GStreamer bin
    description \a gstBinDescription and a \a parent.

    The GStreamer backend interprets the description and creates
    a bin that can act as a video source for a capture session.
    Use method \l gstElement() to access the created bin.

    See \l{https://gstreamer.freedesktop.org/documentation/tools/gst-launch.html?gi-language=c#pipeline-description}{GStreamer documentation}
    for the syntax of the description string.

    If it's unable to parse the specified description, preventing the \c GstBin
    from being created, the constructed QGStreamerVideoSource is non-functional,
    \l gstElement() returns \c nullptr, and \l gstBinDescription() returns
    an empty string.

    \note QGStreamerVideoSource doesn't perform any additional validation
    of the provided bin description, so if the created \c GstBin doesn't represent
    a video input for \c GstPipeline, QGStreamerVideoSource and
    the associated QMediaCaptureSession may misbehave.

    For better control of the underlying GStreamer element, you can
    construct QGStreamerVideoSource from an existing \c GstElement instead.

    \sa gstElement(), gstBinDescription()
*/
QGStreamerVideoSource::QGStreamerVideoSource(const QString &gstBinDescription, QObject *parent)
    : QGStreamerVideoSource(parent)
{
    Q_D(QGStreamerVideoSource);
    d->createPlatformCamera(this, gstBinDescription);
}

/*!
    Constructs a QGStreamerVideoSource from an existing \a gstElement and a
    \a parent.

    The element must provide a video source that a capture session can use.
    The \c GstElement type is forward-declared in the \c QGStreamerVideoSource header,
    so the corresponding type from the GStreamer library must be used.

    If the specified element is null, the constructed video source is
    non-functional.

    \note The class doesn't perform any additional validation
    of the provided element, so if the specified element doesn't represent
    a video input for \c GstPipeline, QGStreamerVideoSource and
    the associated QMediaCaptureSession may misbehave.

    \note QGStreamerVideoSource shares ownership of the provided \c GstElement
    with the caller.
*/
QGStreamerVideoSource::QGStreamerVideoSource(GstElement *gstElement, QObject *parent)
    : QGStreamerVideoSource(parent)
{
    Q_D(QGStreamerVideoSource);
    d->createPlatformCamera(this, gstElement);
}

QGStreamerVideoSource::QGStreamerVideoSource(QObject *parent)
    : QObject(*new QGStreamerVideoSourcePrivate, parent)
{
}

QGStreamerVideoSource::~QGStreamerVideoSource() = default;


/*! \qmlproperty bool QtMultimedia::GStreamerVideoSource::active

    Describes whether the video source is currently active.

    If the video source has been connected to a \l CaptureSession,
    this property indicates whether the underlying \c GstElement
    is attached to the \c GstPipeline within the media capture session.
*/

/*! \property QGStreamerVideoSource::active

    Describes whether the video source is currently active.

    If the video source has been connected to a \l QMediaCaptureSession,
    this property indicates whether the underlying \c GstElement
    is attached to the \c GstPipeline within the media capture session.
*/

bool QGStreamerVideoSource::isActive() const
{
    Q_D(const QGStreamerVideoSource);
    return d->platformCamera && d->platformCamera->isActive();
}

/*! \qmlmethod void GStreamerVideoSource::start()

    Starts the video source.

    Same as setting the \l active property to \c true.
*/

/*! \fn void QGStreamerVideoSource::start()

    Starts the video source.

    Same as setting the \l active property to \c true.
*/

/*! \qmlmethod void GStreamerVideoSource::stop()

    Stops the video source.

    Same as setting the \l active property to \c false.
*/

/*! \fn void QGStreamerVideoSource::stop()

    Stops the video source.

    Same as setting the \l active property to \c false.
*/

/*! \qmlproperty string QtMultimedia::GStreamerVideoSource::gstBinDescription

    Describes the GStreamer bin description associated with the video source.

    In QML, this property is required.

    The GStreamer backend interprets the description and creates a bin,
    that can act as a video source for a capture session.

    See \l{https://gstreamer.freedesktop.org/documentation/tools/gst-launch.html?gi-language=c#pipeline-description}{GStreamer documentation}
    for the syntax of the description string.

    If the specified description is invalid, preventing a bin from
    being created, the constructed \c GStreamerVideoSource is non-functional,
    and the property returns an empty string.

    \note GStreamerVideoSource doesn't perform any additional validation
    of the provided bin description, so if the created \c GstBin doesn't represent
    a video input for \c GstPipeline, GStreamerVideoSource and
    the associated CaptureSession may misbehave.
*/

/*! \property QGStreamerVideoSource::gstBinDescription

    Describes the GStreamer bin description associated with the video source.

    If the video source is non-functional or has been constructed
    from an existing \c GstElement, the property string is empty.
*/
QString QGStreamerVideoSource::gstBinDescription() const
{
    Q_D(const QGStreamerVideoSource);
    return d->gstBinDescription;
}

/*!
    Returns the underlying \c GstElement used by the video source,
    or \c nullptr if the video source is non-functional.

    If the video source has been constructed from a GStreamer bin description,
    the returned \c GstElement is convertable to the \c GstBin subclass.

    \note The method does not transfer ownership of the \c GstElement.
*/
GstElement *QGStreamerVideoSource::gstElement() const
{
    Q_D(const QGStreamerVideoSource);
    return d->platformCamera ? d->platformCamera->rawGstElement() : nullptr;
}

void QGStreamerVideoSource::setActive(bool active)
{
    Q_D(const QGStreamerVideoSource);
    if (d->platformCamera)
        d->platformCamera->setActive(active);
}

QPlatformCamera *QGStreamerVideoSource::platformVideoSource() const
{
    Q_D(const QGStreamerVideoSource);
    return d->platformCamera;
}

QMediaCaptureSession *QGStreamerVideoSource::captureSession() const
{
    Q_D(const QGStreamerVideoSource);
    return d->captureSession;
}

void QGStreamerVideoSource::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QGStreamerVideoSource);
    d->captureSession = session;
}

QT_END_NAMESPACE

#include "moc_qgstreamervideosource.cpp"
