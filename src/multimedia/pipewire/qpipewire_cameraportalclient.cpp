// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_cameraportalclient_p.h"

#include <QtMultimedia/private/qpipewire_portal_p.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qtimer.h>
#include <QtCore/qthread.h>
#include <QtCore/quuid.h>
#include <QtDBus/qdbusconnection.h>
#include <QtDBus/qdbusinterface.h>
#include <QtDBus/qdbusreply.h>
#include <QtDBus/qdbusunixfiledescriptor.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcCameraPortal, "qt.multimedia.pipewire.camera.portal")

namespace {

class CameraPortalWorker : public QObject
{
    Q_OBJECT

public:
    q23::expected<QDBusUnixFileDescriptor, QString> run(std::chrono::milliseconds timeout)
    {
        const QString connectionName = u"qt_pipewire_camera_portal_%1"_s.arg(
                QUuid::createUuid().toString(QUuid::WithoutBraces));
        QDBusConnection bus =
                QDBusConnection::connectToBus(QDBusConnection::SessionBus, connectionName);
        auto disconnectFromBus = qScopeGuard([&] {
            QDBusConnection::disconnectFromBus(connectionName);
        });
        if (!bus.isConnected())
            return q23::unexpected(u"could not open a private session bus connection"_s);

        const QString token = allocatePortalRequestToken();
        const QString requestPath = portalRequestPath(bus, token);

        if (!bus.connect(u"org.freedesktop.portal.Desktop"_s, requestPath,
                         u"org.freedesktop.portal.Request"_s, u"Response"_s, this,
                         SLOT(onResponse(uint, QVariantMap)))) {
            return q23::unexpected(u"could not subscribe to the portal Request response"_s);
        }
        auto disconnectBus = qScopeGuard([&] {
            bus.disconnect(u"org.freedesktop.portal.Desktop"_s, requestPath,
                           u"org.freedesktop.portal.Request"_s, u"Response"_s, this,
                           SLOT(onResponse(uint, QVariantMap)));
        });

        QDBusInterface cameraInterface(u"org.freedesktop.portal.Desktop"_s,
                                       u"/org/freedesktop/portal/desktop"_s,
                                       u"org.freedesktop.portal.Camera"_s, bus);
        QDBusReply<QDBusObjectPath> accessReply =
                cameraInterface.call(u"AccessCamera"_s,
                                     QVariantMap{
                                             { u"handle_token"_s, token },
                                     });
        if (!accessReply.isValid()) {
            return q23::unexpected(u"AccessCamera failed: "_s + accessReply.error().message());
        }

        QEventLoop loop;
        m_loop = &loop;
        QTimer::singleShot(timeout, &loop, &QEventLoop::quit);
        loop.exec();
        m_loop = nullptr;

        if (!m_gotResponse)
            return q23::unexpected(u"timed out waiting for the camera access request"_s);
        if (m_response != 0)
            return q23::unexpected(
                    u"camera access request was not granted (response=%1)"_s.arg(m_response));

        QDBusReply<QDBusUnixFileDescriptor> fdReply =
                cameraInterface.call(u"OpenPipeWireRemote"_s, QVariantMap{});
        if (!fdReply.isValid())
            return q23::unexpected(u"OpenPipeWireRemote failed: "_s + fdReply.error().message());

        return fdReply.value();
    }

private Q_SLOTS:
    void onResponse(uint response, const QVariantMap &)
    {
        m_response = response;
        m_gotResponse = true;
        if (m_loop)
            m_loop->quit();
    }

private:
    QEventLoop *m_loop = nullptr;
    uint m_response = 1;
    bool m_gotResponse = false;
};

} // namespace

q23::expected<QDBusUnixFileDescriptor, QString>
requestCameraPortalFd(std::chrono::milliseconds timeout)
{
    // marshall request through a different thread so that we can block the calling thread until
    // the portal request has completed
    QThread thread;
    thread.start();

    auto worker = std::make_unique<CameraPortalWorker>();
    worker->moveToThread(&thread);

    q23::expected<QDBusUnixFileDescriptor, QString> result =
            q23::unexpected(u"the camera portal worker did not run"_s);
    QMetaObject::invokeMethod(worker.get(), [&] {
        result = worker->run(timeout);
    }, Qt::BlockingQueuedConnection);

    thread.quit();
    thread.wait();

    if (!result)
        qCDebug(lcCameraPortal) << "requestCameraPortalFd failed:" << result.error();

    return result;
}

} // namespace QtPipeWire

QT_END_NAMESPACE

#include "qpipewire_cameraportalclient.moc"
