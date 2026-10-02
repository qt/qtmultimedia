// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QFAKEXDGPORTALFIXTURE_P_H
#define QFAKEXDGPORTALFIXTURE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qexpected_p.h>

#include <private/qsyntheticvideoscene_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

/*!
    \internal

    Runs an unattended stand-in for xdg-desktop-portal, so that tests can exercise
    the portal-backed PipeWire code paths end to end without a human approving a
    consent dialog. Today it implements the ScreenCast interface; the intent is that
    every portal interface qtmultimedia talks to is faked here.

    It brings up, and tears down:
    \list
    \li a private pipewire daemon, with no session manager and no access to any
        real hardware;
    \li a helper process (this test binary, re-executed with
        QtFakeXdgPortal::HelperArgument) which owns
        \c org.freedesktop.portal.Desktop on a private D-Bus session bus and
        publishes a synthetic video source rendering a SyntheticVideoScene.
    \endlist

    Usage from a test's main():
    \code
    #define main testlib_main
    QTEST_MAIN(tst_Foo)
    #undef main

    int main(int argc, char *argv[])
    {
        if (QFakeXdgPortalFixture::isHelperProcess(argc, argv))
            return QFakeXdgPortalFixture::runHelperProcess(argc, argv);
        QFakeXdgPortalFixture::reexecUnderPrivateBusIfRequested(argc, argv);
        return testlib_main(argc, argv);
    }
    \endcode

    The re-exec must happen before QApplication is constructed: QApplication opens
    the session bus, and QDBusConnection caches it for the lifetime of the process.
*/
class QFakeXdgPortalFixture
{
public:
    QFakeXdgPortalFixture();
    ~QFakeXdgPortalFixture();

    Q_DISABLE_COPY_MOVE(QFakeXdgPortalFixture)

    //! True when this process was started as the fake portal helper.
    static bool isHelperProcess(int argc, char **argv);
    static int runHelperProcess(int argc, char **argv);

    /*!
        Whether the fixture should be used at all. Controlled by
        QT_MULTIMEDIA_TEST_XDG_PORTAL_FIXTURE: "fake" forces it on, "none" forces it
        off, and the default "auto" enables it on Linux for non-X11 sessions, where
        the real portal would otherwise block on a consent dialog.
    */
    static bool isRequested();

    //! Whether the external pieces (pipewire, dbus-run-session) are present.
    static bool prerequisitesAvailable(QString *unavailableReason = nullptr);

    /*!
        Re-executes the test binary under dbus-run-session when isRequested() and the
        prerequisites hold, so the whole test runs against a private session bus.
        Returns normally when no re-exec is needed or possible.
    */
    static void reexecUnderPrivateBusIfRequested(int argc, char **argv);

    //! True if the current process is already running on the fixture's private bus.
    static bool isRunningOnPrivateBus();

    q23::expected<void, QString> start(const SyntheticVideoScene &initialScene);
    void stop();
    [[nodiscard]] bool isActive() const;

    //! The node id the fake portal reports from Start(). Mostly useful for diagnostics.
    [[nodiscard]] quint32 nodeId() const;

    //! Replaces the content published by the synthetic source.
    q23::expected<void, QString> setScene(const SyntheticVideoScene &scene);

    //! Revokes the portal session mid-stream, as a compositor would.
    q23::expected<void, QString> closeSessions();

private:
    q23::expected<void, QString> startHelper(QSize frameSize);
    void stopHelper();

    class Private;
    std::unique_ptr<Private> d;
};

QT_END_NAMESPACE

#endif // QFAKEXDGPORTALFIXTURE_P_H
