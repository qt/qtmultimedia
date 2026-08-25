// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QFAKEXDGPORTAL_P_H
#define QFAKEXDGPORTAL_P_H

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

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    \b{Overview.} This fixture stands in for xdg-desktop-portal so that tests can
    exercise portal-backed PipeWire code paths without a human present. The real
    stack looks like:

    \code
    client (e.g. QPipeWireCaptureHelper)
        -> D-Bus session bus
        -> xdg-desktop-portal (interactive consent dialog)
        -> compositor's PipeWire node
    \endcode

    QFakeXdgPortalFixture (qfakexdgportalfixture_p.h) replaces the middle two: it
    starts a private, hardware-free \c pipewire daemon, then re-execs the test
    binary itself as a helper process on a private D-Bus session bus with no
    service activation, so the real xdg-desktop-portal can never be reached. That
    helper process is what this namespace implements. It owns
    \c org.freedesktop.portal.Desktop and answers portal calls immediately with
    canned, always-successful responses (FakePortalObject and its per-interface
    QDBusAbstractAdaptor subclasses in qfakexdgportal.cpp), backed by a
    QSyntheticPipeWireVideoSource (qsyntheticpipewirevideosource_p.h) that publishes
    a synthetic, painter-generated frame as if it came from a real screen or
    camera. A private org.qtproject.qtmultimedia.FakeXdgPortal D-Bus interface lets
    the fixture drive the helper from the test process (change the published
    scene, force a specific response code, and so on).
*/
namespace QtFakeXdgPortal {

inline constexpr const char *HelperArgument = "--qt-fake-xdg-portal";

int runPortalHelper(int argc, char **argv);

} // namespace QtFakeXdgPortal

QT_END_NAMESPACE

#endif // QFAKEXDGPORTAL_P_H
