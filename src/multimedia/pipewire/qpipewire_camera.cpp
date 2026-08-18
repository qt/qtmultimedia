// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_camera_p.h"

#include <QtMultimedia/private/qpipewire_cameracontextmanager_p.h>
#include <QtMultimedia/private/qpipewire_videostream_p.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcPipewireCamera, "qt.multimedia.pipewire.camera")

QPipeWireCamera::QPipeWireCamera(QCamera *camera) : QPlatformCamera(camera)
{
    Q_ASSERT(QCameraContextManager::isSupported());
}

QPipeWireCamera::~QPipeWireCamera()
{
    stopStream();
}

bool QPipeWireCamera::isActive() const
{
    return m_active;
}

void QPipeWireCamera::setActive(bool active)
{
    if (m_active == active)
        return;

    if (active && m_cameraDevice.isNull())
        return;

    if (m_cameraFormat.isNull())
        resolveCameraFormat({});

    m_active = active;

    if (m_active)
        startStream();
    else {
        stopStream();
        // Tells any attached sink to drop what it is showing
        emit newVideoFrame({});
    }

    emit activeChanged(m_active);
}

void QPipeWireCamera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;

    const bool wasActive = m_active;
    stopStream();

    m_cameraDevice = camera;
    resolveCameraFormat({});

    if (wasActive)
        startStream();
}

bool QPipeWireCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    if (!resolveCameraFormat(format))
        return true;

    if (m_active) {
        stopStream();
        startStream();
    }

    return true;
}

bool QPipeWireCamera::resolveCameraFormat(const QCameraFormat &format)
{
    QCameraFormat resolved = format;
    if (resolved.isNull())
        resolved = findBestCameraFormat(m_cameraDevice);

    if (resolved == m_cameraFormat)
        return false;

    m_cameraFormat = resolved;
    return true;
}

QVideoFrameFormat QPipeWireCamera::frameFormat() const
{
    // Until the stream has negotiated a format, report what was requested, so
    // that a sink has something to work with.
    if (m_stream) {
        QVideoFrameFormat negotiated = m_stream->frameFormat();
        if (negotiated.isValid())
            return negotiated;
    }

    return QPlatformCamera::frameFormat();
}

void QPipeWireCamera::startStream()
{
    Q_ASSERT(!m_stream);

    QCameraContextManager &context = *QCameraContextManager::instance();

    const std::optional<QByteArray> target =
            QCameraContextManager::deviceMonitor().findTargetObject(m_cameraDevice.id());
    if (!target) {
        const QByteArray id = m_cameraDevice.id();
        if (!id.startsWith("pw:")) {
            updateError(QCamera::CameraError,
                        u"The camera id \"%1\" does not name a PipeWire camera. Camera ids are "
                        "not interchangeable between backends."_s.arg(QString::fromUtf8(id)));
        } else {
            updateError(QCamera::CameraError,
                        u"No PipeWire camera with the id \"%1\" is present."_s.arg(
                                QString::fromUtf8(id)));
        }
        return;
    }

    m_stream = std::make_unique<QPipeWireVideoStream>(
            context, *target, m_cameraFormat.pixelFormat(), m_cameraFormat.resolution(),
            m_cameraFormat.maxFrameRate());

    connect(m_stream.get(), &QPipeWireVideoStream::newVideoFrame, this,
            &QPipeWireCamera::newVideoFrame, Qt::DirectConnection);
    connect(m_stream.get(), &QPipeWireVideoStream::errorOccurred, this,
            [this](const QString &errorString) {
        updateError(QCamera::CameraError, errorString);
    }, Qt::QueuedConnection);

    if (!m_stream->start()) {
        m_stream.reset();
        updateError(QCamera::CameraError, u"Could not start the PipeWire camera stream."_s);
        return;
    }

    qCDebug(lcPipewireCamera) << "streaming from" << *target << "as" << m_cameraFormat.pixelFormat()
                              << m_cameraFormat.resolution();
}

void QPipeWireCamera::stopStream()
{
    m_stream.reset();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
