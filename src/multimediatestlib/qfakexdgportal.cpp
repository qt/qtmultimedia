// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qfakexdgportal_p.h"

#include <private/qsyntheticvideoscene_p.h>
#include <private/qsyntheticpipewirevideosource_p.h>

#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qpipewire_support_p.h>
#include <QtCore/qchronotimer.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qhash.h>
#include <QtCore/qspan.h>
#include <QtCore/qtimer.h>
#include <QtCore/private/quniquehandle_types_p.h>
#include <QtDBus/qdbusabstractadaptor.h>
#include <QtDBus/qdbusargument.h>
#include <QtDBus/qdbusconnection.h>
#include <QtDBus/qdbuscontext.h>
#include <QtDBus/qdbuserror.h>
#include <QtDBus/qdbusmessage.h>
#include <QtDBus/qdbusmetatype.h>
#include <QtDBus/qdbusextratypes.h>
#include <QtDBus/qdbusunixfiledescriptor.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <optional>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace {

// results["streams"] has D-Bus signature a(ua{sv}), and "position"/"size" inside
// the per-stream property map are (ii). This mirrors what
// QPipeWireCaptureHelper::updateStreams() demarshals.
struct FakePortalPoint
{
    qint32 x = 0;
    qint32 y = 0;
};

struct FakePortalStream
{
    QtPipeWire::ObjectId nodeId{ 0 };
    QVariantMap properties;
};

QDBusArgument &operator<<(QDBusArgument &argument, const FakePortalPoint &point)
{
    argument.beginStructure();
    argument << point.x << point.y;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, FakePortalPoint &point)
{
    argument.beginStructure();
    argument >> point.x >> point.y;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const FakePortalStream &stream)
{
    argument.beginStructure();
    argument << stream.nodeId.value << stream.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, FakePortalStream &stream)
{
    argument.beginStructure();
    argument >> stream.nodeId.value >> stream.properties;
    argument.endStructure();
    return argument;
}

// Opens a bare connection to the PipeWire daemon. This is all the real portal's
// OpenPipeWireRemote has to give us: pw_context_connect_fd() performs the protocol
// handshake itself, so an un-negotiated connected socket is exactly right.
q23::expected<QUniqueFileDescriptorHandle, QString> connectToPipeWire()
{
    QByteArray runtimeDir = qgetenv("PIPEWIRE_RUNTIME_DIR");
    if (runtimeDir.isEmpty())
        runtimeDir = qgetenv("XDG_RUNTIME_DIR");
    QByteArray coreName = qgetenv("PIPEWIRE_CORE");
    if (coreName.isEmpty())
        coreName = "pipewire-0"_ba;

    const QByteArray path = runtimeDir + '/' + coreName;

    sockaddr_un address{
        .sun_family = AF_UNIX,
        .sun_path = {},
    };
    if (size_t(path.size()) >= sizeof(address.sun_path)) {
        return q23::unexpected(u"pipewire socket path exceeds sockaddr_un limits: "_s
                               + QString::fromLocal8Bit(path));
    }
    memcpy(address.sun_path, path.constData(), size_t(path.size()));

    QUniqueFileDescriptorHandle fd{ ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0) };
    if (!fd)
        return q23::unexpected(u"socket(): "_s + QString::fromLocal8Bit(strerror(errno)));
    if (::connect(fd.get(), reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        return q23::unexpected(u"connect("_s + QString::fromLocal8Bit(path) + u"): "_s
                               + QString::fromLocal8Bit(strerror(errno)));
    }
    return fd;
}

class FakePortalRequest : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.portal.Request")
public:
    using QObject::QObject;

public Q_SLOTS:
    Q_SCRIPTABLE void Close() { }

Q_SIGNALS:
    Q_SCRIPTABLE void Response(uint response, QVariantMap results);
};

class FakePortalSession : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.portal.Session")
public:
    using QObject::QObject;

public Q_SLOTS:
    Q_SCRIPTABLE void Close() { Q_EMIT Closed(); }

Q_SIGNALS:
    Q_SCRIPTABLE void Closed();
};

/*!
    The object exported at /org/freedesktop/portal/desktop.

    It carries no D-Bus interface of its own: every portal interface is exported by
    a QDBusAbstractAdaptor attached to it. Adaptors are required here rather than
    merely tidier, because org.freedesktop.portal.ScreenCast and
    org.freedesktop.portal.Camera both define OpenPipeWireRemote -- with different
    signatures -- and both define a version property. A plain QObject dispatches by
    name and signature across its whole metaobject, so the two would be told apart
    only by signature coincidence; adaptors dispatch strictly per interface.

    QDBusContext lives here rather than on the adaptors: QDBusContextPrivate::set()
    redirects to parent() for adaptors, and the documentation says explicitly not to
    inherit QDBusContext and QDBusAbstractAdaptor in the same class.
*/
class FakePortalObject : public QObject, public QDBusContext
{
    Q_OBJECT

public:
    /*!
        Which conversation a request belongs to. A new conversation supersedes
        anything still pending from an earlier one, and the interfaces must not
        cancel each other's responses.
    */
    enum class RequestScope { ScreenCast, Camera };

    FakePortalObject(QSyntheticPipeWireVideoSource &source, QSize frameSize,
                     std::chrono::milliseconds responseDelay, QObject *parent)
        : QObject(parent), m_source(source), m_frameSize(frameSize), m_responseDelay(responseDelay)
    {
    }

    QSyntheticPipeWireVideoSource &source() const { return m_source; }
    QSize frameSize() const { return m_frameSize; }

    //! QDBusContext::sendErrorReply() is protected, so adaptors cannot reach it.
    void failWith(QDBusError::ErrorType type, const QString &message)
    {
        sendErrorReply(type, message);
    }

    //! Exports a session object for \a token, reusing one that already exists.
    QString ensureSession(const QString &token)
    {
        const QString sessionPath =
                u"/org/freedesktop/portal/desktop/session/%1/%2"_s.arg(callerName(), token);

        if (!m_sessions.contains(sessionPath)) {
            auto *session = new FakePortalSession(this);
            QDBusConnection::sessionBus().registerObject(sessionPath, session,
                                                         QDBusConnection::ExportScriptableContents);
            m_sessions.insert(sessionPath, session);
        }
        return sessionPath;
    }

    /*!
        Drops responses still pending in \a scope. Without this, a response left over
        from a previous capture cycle is delivered into the new one --
        QPipeWireCaptureHelper subscribes to Response on every object path and
        dispatches purely on its own state, so a stale response corrupts its state
        machine and aborts the process.
    */
    void supersedePending(RequestScope scope) { ++generationFor(scope); }

    QDBusObjectPath respond(const QVariantMap &options, const QVariantMap &results,
                            RequestScope scope)
    {
        const QString token =
                options.value(u"handle_token"_s, u"qt%1"_s.arg(++m_fallbackToken)).toString();
        const QString path =
                u"/org/freedesktop/portal/desktop/request/%1/%2"_s.arg(callerName(), token);

        // QPipeWireCaptureHelper::getRequestToken() caches its token and reuses it
        // for SelectSources and Start, so the same path shows up more than once.
        // The object must be reused: registering a second object on a taken path
        // fails, and unregistering after each Response tears down the signal relay
        // before the message reaches the bus -- in both cases the Response is
        // silently lost and the client stalls.
        FakePortalRequest *request = m_requests.value(path);
        if (!request) {
            request = new FakePortalRequest(this);
            if (!QDBusConnection::sessionBus().registerObject(
                        path, request, QDBusConnection::ExportScriptableContents)) {
                qWarning() << "FakePortalObject: cannot export request object at" << path;
                delete request;
                return QDBusObjectPath(path);
            }
            m_requests.insert(path, request);
        }

        const uint responseCode = std::exchange(m_nextResponseCode, 0u);
        const quint64 generation = generationFor(scope);

        // Deliberately deferred. QPipeWireCaptureHelper assigns m_operationState
        // only after its blocking call returns, and does so on a different thread
        // from the one this signal is delivered on; responding synchronously can
        // beat it and leave the client's state machine stuck.
        QTimer::singleShot(m_responseDelay, request,
#if __cplusplus >= 202002L
                           [=, this] {
#else
                           [=] {
#endif
            if (generation != generationFor(scope))
                return; // superseded by a newer conversation
            Q_EMIT request->Response(responseCode, results);
        });

        return QDBusObjectPath(path);
    }

    // Test hooks. Deliberately not D-Bus slots here: exporting them would add
    // non-standard methods to a standard portal interface. They are reached through
    // FakeXdgPortalControl instead.
    void rememberSelectSourcesOptions(const QVariantMap &options)
    {
        m_selectSourcesOptions = options;
    }
    QVariantMap takeSelectSourcesOptions() { return std::exchange(m_selectSourcesOptions, {}); }
    void setNextResponseCode(uint code) { m_nextResponseCode = code; }
    void closeSessions()
    {
        for (FakePortalSession *session : std::as_const(m_sessions))
            Q_EMIT session->Closed();
    }

private:
    quint64 &generationFor(RequestScope scope)
    {
        return scope == RequestScope::ScreenCast ? m_screenCastGeneration : m_cameraGeneration;
    }

    // Portal spec: the caller's unique bus name with ':' dropped and '.' -> '_'.
    QString callerName() const
    {
        QString name = message().service();
        if (name.startsWith(u':'))
            name.remove(0, 1);
        return name.replace(u'.', u'_');
    }

    QSyntheticPipeWireVideoSource &m_source;
    const QSize m_frameSize;
    const std::chrono::milliseconds m_responseDelay;
    QHash<QString, FakePortalRequest *> m_requests;
    QHash<QString, FakePortalSession *> m_sessions;
    QVariantMap m_selectSourcesOptions;
    uint m_nextResponseCode = 0;
    quint64 m_screenCastGeneration = 0;
    quint64 m_cameraGeneration = 0;
    int m_fallbackToken = 0;
};

class FakeScreenCastAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.portal.ScreenCast")
    Q_PROPERTY(uint version READ version)
    Q_PROPERTY(uint AvailableSourceTypes READ availableSourceTypes)
    Q_PROPERTY(uint AvailableCursorModes READ availableCursorModes)

public:
    explicit FakeScreenCastAdaptor(FakePortalObject *portal)
        : QDBusAbstractAdaptor(portal), m_portal(portal)
    {
    }

    uint version() const { return 5; }
    uint availableSourceTypes() const { return 3; } // MONITOR | WINDOW
    uint availableCursorModes() const { return 7; } // HIDDEN | EMBEDDED | METADATA

    // Adaptors export all public slots, signals and properties, so no Q_SCRIPTABLE
    // is needed and ExportAdaptors alone is enough at registration time.
public Q_SLOTS:
    QDBusObjectPath CreateSession(const QVariantMap &options)
    {
        m_portal->supersedePending(FakePortalObject::RequestScope::ScreenCast);

        const QString sessionPath =
                m_portal->ensureSession(options.value(u"session_handle_token"_s).toString());

        return m_portal->respond(options, QVariantMap{ { u"session_handle"_s, sessionPath } },
                                 FakePortalObject::RequestScope::ScreenCast);
    }

    QDBusObjectPath SelectSources(const QDBusObjectPath &session, const QVariantMap &options)
    {
        Q_UNUSED(session);
        m_portal->rememberSelectSourcesOptions(options);
        return m_portal->respond(options, {}, FakePortalObject::RequestScope::ScreenCast);
    }

    QDBusObjectPath Start(const QDBusObjectPath &session, const QString &parentWindow,
                          const QVariantMap &options)
    {
        Q_UNUSED(session);
        Q_UNUSED(parentWindow);

        const QSize frameSize = m_portal->frameSize();

        FakePortalStream stream;
        stream.nodeId = m_portal->source().nodeId();
        stream.properties = QVariantMap{
            { u"id"_s, u"qt-fake-monitor-0"_s },
            { u"position"_s, QVariant::fromValue(FakePortalPoint{ 0, 0 }) },
            { u"size"_s,
              QVariant::fromValue(FakePortalPoint{ frameSize.width(), frameSize.height() }) },
            { u"source_type"_s, uint(1) }, // MONITOR
        };

        return m_portal->respond(
                options,
                QVariantMap{
                        { u"streams"_s, QVariant::fromValue(QList<FakePortalStream>{ stream }) } },
                FakePortalObject::RequestScope::ScreenCast);
    }

    QDBusUnixFileDescriptor OpenPipeWireRemote(const QDBusObjectPath &session,
                                               const QVariantMap &options)
    {
        Q_UNUSED(session);
        Q_UNUSED(options);

        auto fd = connectToPipeWire();
        if (!fd) {
            m_portal->failWith(QDBusError::Failed, fd.error());
            return QDBusUnixFileDescriptor();
        }

        return QDBusUnixFileDescriptor(fd->get()); // duplicates the descriptor
    }

private:
    FakePortalObject *m_portal;
};

//! Control interface used by QFakeXdgPortalFixture to drive the helper.
class FakeXdgPortalControl : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.qtmultimedia.FakeXdgPortal")

public:
    FakeXdgPortalControl(QSyntheticPipeWireVideoSource &source, FakePortalObject &portal,
                         QSize frameSize, QObject *parent)
        : QObject(parent), m_source(source), m_portal(portal), m_frameSize(frameSize)
    {
    }

public Q_SLOTS:
    Q_SCRIPTABLE void SetScene(const QVariantMap &sceneMap)
    {
        SyntheticVideoScene scene = SyntheticVideoScene::fromVariantMap(sceneMap);
        // The stream format was negotiated at startup, so the frame size is fixed
        // for the lifetime of the helper.
        scene.frameSize = m_frameSize;
        m_source.setScene(scene);
    }

    Q_SCRIPTABLE uint NodeId() { return m_source.nodeId().value; }

    //! What the client asked for in SelectSources, so tests can assert on it.
    Q_SCRIPTABLE QVariantMap TakeSelectSourcesOptions()
    {
        return m_portal.takeSelectSourcesOptions();
    }

    //! Makes the next portal request answer with \a code, e.g. 1 for "cancelled".
    Q_SCRIPTABLE void SetNextResponseCode(uint code) { m_portal.setNextResponseCode(code); }

    //! Revokes the session mid-stream, as a compositor would.
    Q_SCRIPTABLE void CloseSessions() { m_portal.closeSessions(); }

private:
    QSyntheticPipeWireVideoSource &m_source;
    FakePortalObject &m_portal;
    const QSize m_frameSize;
};

} // namespace

namespace {

struct CLIArgs
{
    QSize frameSize;
    std::chrono::milliseconds responseDelay;
    int frameRate = 0;
};

std::optional<CLIArgs> parseArgs(int argc, char **argv)
{
    QCommandLineOption frameSizeOption(u"frame-size"_s, u"virtual screen size, WxH"_s, u"size"_s,
                                       u"1920x1080"_s);
    QCommandLineOption responseDelayOption(u"response-delay"_s,
                                           u"milliseconds before answering a portal request"_s,
                                           u"ms"_s, u"25"_s);
    QCommandLineOption frameRateOption(u"frame-rate"_s, u"frames per second to publish"_s, u"fps"_s,
                                       u"60"_s);
    QCommandLineParser parser;
    parser.addOptions({ frameSizeOption, responseDelayOption, frameRateOption });
    parser.addOption(
            QCommandLineOption(QString::fromLatin1(QtFakeXdgPortal::HelperArgument).mid(2)));

    using namespace QtMultimediaPrivate;

    QStringList arguments = views::transform(QSpan(argv, argc), [](const char *arg) {
        return QString::fromLocal8Bit(arg);
    }) | ranges::to<QStringList>();

    parser.process(arguments);

    const QStringList dimensions = parser.value(frameSizeOption).split(u'x');
    const QSize frameSize(dimensions.value(0).toInt(), dimensions.value(1).toInt());
    if (frameSize.isEmpty()) {
        qCritical("fake xdg portal helper: invalid --frame-size");
        return std::nullopt;
    }

    return CLIArgs{
        .frameSize = frameSize,
        .responseDelay = std::chrono::milliseconds(parser.value(responseDelayOption).toInt()),
        .frameRate = std::max(1, parser.value(frameRateOption).toInt()),
    };
}

} // namespace

namespace QtFakeXdgPortal {

int runPortalHelper(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const std::optional<CLIArgs> args = parseArgs(argc, argv);
    if (!args)
        return 1;
    const QSize frameSize = args->frameSize;

    qDBusRegisterMetaType<FakePortalPoint>();
    qDBusRegisterMetaType<FakePortalStream>();
    qDBusRegisterMetaType<QList<FakePortalStream>>();

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCritical("fake xdg portal helper: no session bus");
        return 1;
    }

    SyntheticVideoScene scene;
    scene.frameSize = frameSize;

    QSyntheticPipeWireVideoSource source;
    if (auto result = source.start(scene); !result) {
        qCritical("fake xdg portal helper: %s", qPrintable(result.error()));
        return 1;
    }

    // Clock the graph ourselves: with no session manager there is no other driver.
    QChronoTimer frameTimer(1000ms / args->frameRate);
    frameTimer.callOnTimeout(&app, [&] {
        source.renderNextFrame();
    });
    frameTimer.start();

    auto *portal = new FakePortalObject(source, frameSize, args->responseDelay, &app);
    new FakeScreenCastAdaptor(portal);

    if (!bus.registerObject(u"/org/freedesktop/portal/desktop"_s, portal,
                            QDBusConnection::ExportAdaptors)) {
        qCritical("fake xdg portal helper: cannot export the portal object");
        return 1;
    }

    auto *control = new FakeXdgPortalControl(source, *portal, frameSize, &app);
    bus.registerObject(u"/org/qtproject/qtmultimedia/FakeXdgPortal"_s, control,
                       QDBusConnection::ExportScriptableContents);

    // Claim the name only once the node exists, so that a client which sees the
    // service is guaranteed to get a usable node id out of Start().
    if (!bus.registerService(u"org.freedesktop.portal.Desktop"_s)) {
        qCritical("fake xdg portal helper: cannot own org.freedesktop.portal.Desktop: %s",
                  qPrintable(bus.lastError().message()));
        return 1;
    }

    // Handshake with QFakeXdgPortalFixture, which waits for this line.
    std::printf("QT_FAKE_XDG_PORTAL_READY %u\n", source.nodeId().value);
    std::fflush(stdout);

    const int result = app.exec();
    source.stop();
    return result;
}

} // namespace QtFakeXdgPortal

QT_END_NAMESPACE

#include "qfakexdgportal.moc"
