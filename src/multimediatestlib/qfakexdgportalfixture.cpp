// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qfakexdgportalfixture_p.h"

#include <private/qfakexdgportal_p.h>

#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qpipewire_support_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qprocess.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qthread.h>
#include <QtCore/qvarlengtharray.h>
#include <QtDBus/qdbusconnection.h>
#include <QtDBus/qdbusinterface.h>

#include <algorithm>
#include <chrono>
#include <iterator>
#include <string_view>
#include <vector>

#ifdef Q_OS_LINUX
#  include <sys/prctl.h>
#endif

#include <unistd.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

namespace {

// Marks a process that has already been re-executed, so we never recurse.
constexpr const char *PrivateBusMarker = "QT_MULTIMEDIA_TEST_XDG_PORTAL_PRIVATE_BUS";
constexpr const char *FixtureModeVariable = "QT_MULTIMEDIA_TEST_XDG_PORTAL_FIXTURE";
// Where the empty XDG data directory used to disarm D-Bus activation lives, so
// that stop() can clean it up again.
constexpr const char *PrivateDataDirVariable = "QT_MULTIMEDIA_TEST_XDG_PORTAL_DATA_DIR";

constexpr auto StartupTimeout = 10s;
constexpr int StartupTimeoutMs = int(std::chrono::milliseconds(StartupTimeout).count());

/*
    Deliberately not /usr/share/pipewire/minimal.conf: that one loads
    module-protocol-pulse, which binds $XDG_RUNTIME_DIR/pulse/native and aborts the
    whole daemon with EADDRINUSE whenever a developer's own pipewire-pulse is
    running. We only need enough modules to host one video stream and link it.
    Dropping spa-node-factory, access or rt from this list makes clients hang in
    "connecting", so the set is load-bearing.
*/
/*
    A session bus with no <servicedir> at all, so it can activate nothing.
    Emptying XDG_DATA_DIRS is not enough: dbus-daemon's standard_session_servicedirs
    also covers a compiled-in directory, so the real xdg-desktop-portal still got
    activated. That mattered beyond just racing us for the bus name -- the activated
    services inherit the test's stdout and outlive it, so ctest's execute_process
    never saw EOF and hung long after the test itself had passed.
*/
constexpr const char *BusConfiguration = R"(<!DOCTYPE busconfig PUBLIC
 "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <keep_umask/>
  <listen>unix:tmpdir=/tmp</listen>
  <!-- deliberately no <servicedir>: nothing may be activated on this bus -->
  <policy context="default">
    <allow send_destination="*" eavesdrop="true"/>
    <allow eavesdrop="true"/>
    <allow own="*"/>
  </policy>
</busconfig>
)";

constexpr const char *DaemonConfiguration = R"(
context.properties = {
    link.max-buffers = 16
    core.daemon       = true
    core.name         = pipewire-0
}

context.spa-libs = {
    support.* = support/libspa-support
}

context.modules = [
    { name = libpipewire-module-rt
        args = { nice.level = -11, rt.prio = 88 }
        flags = [ ifexists nofail ]
    }
    { name = libpipewire-module-protocol-native }
    { name = libpipewire-module-metadata }
    { name = libpipewire-module-spa-node-factory }
    { name = libpipewire-module-client-node }
    { name = libpipewire-module-access }
    { name = libpipewire-module-adapter }
    { name = libpipewire-module-link-factory }
]

context.objects = []
context.exec = []
)";

QString findPipeWireDaemon()
{
    QString path = QStandardPaths::findExecutable(u"pipewire"_s);
    if (path.isEmpty())
        path = QStandardPaths::findExecutable(u"pipewire"_s,
                                              { u"/usr/bin"_s, u"/usr/local/bin"_s });
    return path;
}

void killChildWithParent()
{
#ifdef Q_OS_LINUX
    // Ensures an aborted or crashed test never leaves daemons behind on CI.
    ::prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
}

bool waitForProcessOutput(QProcess &process, QByteArray *accumulated, QByteArrayView needle,
                          std::chrono::milliseconds timeout)
{
    QDeadlineTimer deadline(timeout);
    while (!deadline.hasExpired()) {
        if (process.state() == QProcess::NotRunning && process.bytesAvailable() == 0)
            return false;
        if (process.waitForReadyRead(100))
            accumulated->append(process.readAllStandardOutput());
        if (accumulated->contains(needle))
            return true;
    }
    return false;
}

} // namespace

class QFakeXdgPortalFixture::Private
{
public:
    QString runtimeDir;
    QProcessEnvironment environment;
    QProcess daemon;
    QProcess helper;
    QByteArray helperOutput;
    QSize frameSize;
    QtPipeWire::ObjectId nodeId{ 0 };
    bool active = false;

    void removeRuntimeDir()
    {
        if (!runtimeDir.isEmpty()) {
            QDir(runtimeDir).removeRecursively();
            runtimeDir.clear();
        }
    }
};

QFakeXdgPortalFixture::QFakeXdgPortalFixture() : d(std::make_unique<Private>()) { }

QFakeXdgPortalFixture::~QFakeXdgPortalFixture()
{
    stop();
}

bool QFakeXdgPortalFixture::isHelperProcess(int argc, char **argv)
{
    return std::any_of(argv + 1, argv + argc, [](const char *arg) {
        return std::string_view(arg) == QtFakeXdgPortal::HelperArgument;
    });
}

int QFakeXdgPortalFixture::runHelperProcess(int argc, char **argv)
{
    return QtFakeXdgPortal::runPortalHelper(argc, argv);
}

bool QFakeXdgPortalFixture::isRequested()
{
#ifndef Q_OS_LINUX
    return false;
#else
    // This file is only compiled when QT_FEATURE_pipewire_screencapture is on.
    const QString mode = qEnvironmentVariable(FixtureModeVariable, u"auto"_s).toLower();
    if (mode == u"none" || mode == u"0")
        return false;
    if (mode == u"fake" || mode == u"1")
        return true;

    // auto: only where the real portal would need a human. On X11 the native
    // X11 capture backend is used and needs no portal at all.
    return qEnvironmentVariable("XDG_SESSION_TYPE").toLower() != u"x11";
#endif
}

bool QFakeXdgPortalFixture::prerequisitesAvailable(QString *unavailableReason)
{
    const auto missing = [&](const QString &reason) {
        if (unavailableReason)
            *unavailableReason = reason;
        return false;
    };

    if (findPipeWireDaemon().isEmpty())
        return missing(u"the 'pipewire' daemon was not found in PATH"_s);
    if (QStandardPaths::findExecutable(u"dbus-run-session"_s).isEmpty())
        return missing(u"'dbus-run-session' was not found in PATH"_s);
    return true;
}

bool QFakeXdgPortalFixture::isRunningOnPrivateBus()
{
    return qEnvironmentVariableIsSet(PrivateBusMarker);
}

void QFakeXdgPortalFixture::reexecUnderPrivateBusIfRequested(int argc, char **argv)
{
    if (isRunningOnPrivateBus() || isHelperProcess(argc, argv) || !isRequested())
        return;
    if (!prerequisitesAvailable(nullptr))
        return; // start() reports this properly; here we simply do not re-exec

    const QString dbusRunSession = QStandardPaths::findExecutable(u"dbus-run-session"_s);

    // Assembled into its own QProcessEnvironment and handed to execve() below,
    // rather than qputenv()'d into this process: if execve() fails (the binary
    // could in principle vanish between the check above and here), this process
    // keeps running -- as the fallback path after execve() does -- and should
    // find its original, untouched environment there.
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();

    // QApplication asks for org.freedesktop.portal.Desktop while setting up its
    // platform services. On a bus that can activate things, that starts the real
    // xdg-desktop-portal -- which claims the name before our helper exists, and
    // whose processes inherit our stdout and outlive us. Give the bus a config
    // with no service directories at all, and empty the XDG data dirs too.
    QByteArray dataDirTemplate = "/tmp/qtportal.XXXXXX"_ba;
    QString busConfigurationPath;
    if (::mkdtemp(dataDirTemplate.data())) {
        const QString dataDir = QString::fromLocal8Bit(dataDirTemplate);
        environment.insert(u"XDG_DATA_DIRS"_s, dataDir);
        environment.insert(u"XDG_DATA_HOME"_s, dataDir);
        environment.insert(QString::fromLatin1(PrivateDataDirVariable), dataDir);

        busConfigurationPath = dataDir + u"/session.conf"_s;
        QFile busConfiguration(busConfigurationPath);
        if (busConfiguration.open(QIODevice::WriteOnly))
            busConfiguration.write(BusConfiguration);
        else
            busConfigurationPath.clear();
    }
    if (busConfigurationPath.isEmpty()) {
        qWarning() << "QFakeXdgPortalFixture: could not disarm D-Bus activation; the real"
                   << "xdg-desktop-portal may claim the portal name first";
    }

    environment.insert(QString::fromLatin1(PrivateBusMarker), u"1"_s);
    // The fixture only makes sense against the PipeWire capture backend, and the
    // value is cached on first use inside the FFmpeg integration.
    environment.insert(u"QT_SCREEN_CAPTURE_BACKEND"_s, u"pipewire"_s);

    QVarLengthArray<char *, 64> arguments;
    QByteArray program = dbusRunSession.toLocal8Bit();
    QByteArray configOption = "--config-file="_ba + busConfigurationPath.toLocal8Bit();
    QByteArray separator = "--"_ba;
    arguments.push_back(program.data());
    if (!busConfigurationPath.isEmpty())
        arguments.push_back(configOption.data());
    arguments.push_back(separator.data());
    std::copy(argv, argv + argc, std::back_inserter(arguments));
    arguments.push_back(nullptr);

    // execve() takes the replacement environment explicitly, so the "KEY=VALUE"
    // entries need to survive as long as the exec call itself: keep their storage
    // in environmentStorage, and only take addresses into it once it is done
    // growing.
    const QStringList environmentList = environment.toStringList();
    std::vector<QByteArray> environmentStorage;
    environmentStorage.reserve(environmentList.size());
    QtMultimediaPrivate::ranges::transform(environmentList, std::back_inserter(environmentStorage),
                                           [](const QString &entry) {
        return entry.toLocal8Bit();
    });

    // Plain std::transform, not the ranges wrapper above: its C++17 fallback binds
    // the source element as const, which would return a read-only char* here.
    QVarLengthArray<char *, 64> envp;
    std::transform(environmentStorage.begin(), environmentStorage.end(), std::back_inserter(envp),
                   [](QByteArray &entry) {
        return entry.data();
    });
    envp.push_back(nullptr);

    ::execve(program.constData(), arguments.data(), envp.data());

    // Only reached if execve() failed; carry on without the private bus and let
    // start() produce a comprehensible error. This process's own environment was
    // never touched, so there is nothing to undo here.
    qWarning() << "QFakeXdgPortalFixture: could not re-exec under dbus-run-session";
}

/*
    (Re)launches the portal helper for a given virtual screen size. The size is
    baked into the pw_stream format at connect time, so changing it means starting
    a new helper -- which is cheap, and keeps the synthetic screen exactly the size
    a test expects rather than forcing tests to accommodate the fixture.
*/
q23::expected<void, QString> QFakeXdgPortalFixture::startHelper(QSize frameSize)
{
    stopHelper();

    // The helper is this very binary, re-executed in portal mode.
    d->helper.setProcessEnvironment(d->environment);
    d->helper.setChildProcessModifier(killChildWithParent);
    d->helper.setProgram(QCoreApplication::applicationFilePath());
    d->helper.setArguments({
            QString::fromLatin1(QtFakeXdgPortal::HelperArgument),
            u"--frame-size"_s,
            u"%1x%2"_s.arg(frameSize.width()).arg(frameSize.height()),
    });
    d->helper.start();
    if (!d->helper.waitForStarted(StartupTimeoutMs))
        return q23::unexpected(u"could not start the fake portal helper"_s);

    if (!waitForProcessOutput(d->helper, &d->helperOutput,
                              QByteArrayView("QT_FAKE_XDG_PORTAL_READY"), StartupTimeout)) {
        return q23::unexpected(u"the fake portal helper did not become ready: "_s
                               + QString::fromLocal8Bit(d->helper.readAllStandardError()));
    }

    d->nodeId = QtPipeWire::ObjectId{ d->helperOutput.split(' ').value(1).trimmed().toUInt() };
    d->frameSize = frameSize;
    return {};
}

void QFakeXdgPortalFixture::stopHelper()
{
    if (d->helper.state() != QProcess::NotRunning) {
        d->helper.terminate();
        if (!d->helper.waitForFinished(2000))
            d->helper.kill();
        d->helper.waitForFinished(2000);
    }
    d->helperOutput.clear();
    d->nodeId = QtPipeWire::ObjectId{ 0 };
    d->frameSize = {};
}

q23::expected<void, QString> QFakeXdgPortalFixture::start(const SyntheticVideoScene &initialScene)
{
    const auto fail = [this](const QString &reason) {
        stop();
        return q23::unexpected(reason);
    };

    if (d->active)
        return fail(u"already started"_s);

    QString unavailableReason;
    if (!prerequisitesAvailable(&unavailableReason))
        return fail(unavailableReason);

    if (!isRunningOnPrivateBus()) {
        return fail(u"not running on a private session bus; "
                    "main() must call reexecUnderPrivateBusIfRequested()"_s);
    }

    // sockaddr_un.sun_path is only 108 bytes, and a build-tree path blows that
    // limit, so the runtime directory has to live somewhere short.
    QByteArray runtimeTemplate = "/tmp/qtpw.XXXXXX"_ba;
    if (!::mkdtemp(runtimeTemplate.data()))
        return fail(u"could not create a private pipewire runtime directory"_s);
    d->runtimeDir = QString::fromLocal8Bit(runtimeTemplate);

    const QString configurationPath = d->runtimeDir + u"/qt-fake-xdg-portal.conf"_s;
    QFile configuration(configurationPath);
    if (!configuration.open(QIODevice::WriteOnly))
        return fail(u"could not write "_s + configurationPath);
    configuration.write(DaemonConfiguration);
    configuration.close();

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(u"PIPEWIRE_RUNTIME_DIR"_s, d->runtimeDir);
    environment.remove(u"PIPEWIRE_REMOTE"_s);

    d->daemon.setProcessEnvironment(environment);
    d->daemon.setChildProcessModifier(killChildWithParent);
    d->daemon.setProgram(findPipeWireDaemon());
    d->daemon.setArguments({ u"-c"_s, configurationPath });
    d->daemon.start();
    if (!d->daemon.waitForStarted(StartupTimeoutMs))
        return fail(u"could not start the private pipewire daemon"_s);

    const QString socketPath = d->runtimeDir + u"/pipewire-0"_s;
    QDeadlineTimer deadline(StartupTimeout);
    while (!QFile::exists(socketPath)) {
        if (deadline.hasExpired() || d->daemon.state() == QProcess::NotRunning) {
            return fail(u"the private pipewire daemon did not create %1: %2"_s.arg(
                    socketPath, QString::fromLocal8Bit(d->daemon.readAllStandardError())));
        }
        QThread::msleep(20);
    }

    d->environment = environment;

    if (auto result = startHelper(initialScene.frameSize); !result)
        return fail(result.error());

    d->active = true;

    return setScene(initialScene);
}

void QFakeXdgPortalFixture::stop()
{
    stopHelper();

    if (d->daemon.state() != QProcess::NotRunning) {
        d->daemon.terminate();
        if (!d->daemon.waitForFinished(2000))
            d->daemon.kill();
        d->daemon.waitForFinished(2000);
    }

    d->removeRuntimeDir();
    if (qEnvironmentVariableIsSet(PrivateDataDirVariable)) {
        QDir(qEnvironmentVariable(PrivateDataDirVariable)).removeRecursively();
        qunsetenv(PrivateDataDirVariable);
    }
    d->helperOutput.clear();
    d->nodeId = QtPipeWire::ObjectId{ 0 };
    d->active = false;
}

bool QFakeXdgPortalFixture::isActive() const
{
    return d->active;
}

quint32 QFakeXdgPortalFixture::nodeId() const
{
    return d->nodeId.value;
}

q23::expected<void, QString> QFakeXdgPortalFixture::setScene(const SyntheticVideoScene &scene)
{
    if (!d->active)
        return q23::unexpected(u"the fixture is not running"_s);

    if (scene.frameSize.isValid() && scene.frameSize != d->frameSize) {
        if (auto result = startHelper(scene.frameSize); !result)
            return result;
    }

    QDBusInterface control(
            u"org.freedesktop.portal.Desktop"_s, u"/org/qtproject/qtmultimedia/FakeXdgPortal"_s,
            u"org.qtproject.qtmultimedia.FakeXdgPortal"_s, QDBusConnection::sessionBus());
    const QDBusMessage reply = control.call(u"SetScene"_s, scene.toVariantMap());
    if (reply.type() == QDBusMessage::ErrorMessage)
        return q23::unexpected(u"SetScene failed: "_s + reply.errorMessage());
    return {};
}

QT_END_NAMESPACE
