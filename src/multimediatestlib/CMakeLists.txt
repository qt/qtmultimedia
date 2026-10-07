# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
qt_internal_add_module(MultimediaTestLibPrivate
    CONFIG_MODULE_NAME multimediatestlibprivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        audiogenerationutils_p.h audiogenerationutils.cpp
        capturesessionfixture_p.h capturesessionfixture.cpp
        framegenerator_p.h framegenerator.cpp
        framegrabber_p.h framegrabber.cpp
        mediabackendutils_p.h
        qintegrationtestbase_p.h qintegrationtestbase.cpp
        mediafileselector_p.h
        formatutils_p.h formatutils.cpp
        mediainfo_p.h
        multimedia_debug_support_p.h
        osdetection_p.h
        rhi_support_p.h rhi_support.cpp
        testvideosink_p.h testvideosink.cpp
        surfacecapturetestutils_p.h surfacecapturetestutils.cpp
        qcolorutil_p.h qcolorutil.cpp
        qfileutil_p.h qfileutil.cpp
        qvideoframetestutils_p.h qvideoframetestutils.cpp
        qscopedenvironmentvariable_p.h
        qsyntheticvideoscene_p.h qsyntheticvideoscene.cpp
        qsequentialfileadaptor_p.h
        qsinewavevalidator_p.h qsinewavevalidator.cpp
        qmockiodevice_p.h
        qmessagespy_p.h
        qmockresolvers_p.h
    SYSTEM_INCLUDE_DIRECTORIES
        ../3rdparty/dr_libs
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::Test
    NO_GENERATE_CPP_EXPORTS
    QT_LICENSE_ID QT_COMMERCIAL_OR_GPL3
)

qt_internal_extend_target(MultimediaTestLibPrivate CONDITION LINUX
    SOURCES
        udmabuftestutils_p.h udmabuftestutils.cpp
)

qt_internal_extend_target(MultimediaTestLibPrivate CONDITION WIN32
    SOURCES
        windows/qwindowsd3d11testdevicecontext_p.h windows/qwindowsd3d11testdevicecontext.cpp
    LIBRARIES
        d3d11
)

# TODO: widen to "OR QT_FEATURE_pipewire_camera" once that feature lands, since
# the fixture also fakes org.freedesktop.portal.Camera.
qt_internal_extend_target(MultimediaTestLibPrivate
    CONDITION
        QT_FEATURE_pipewire_screencapture
    SOURCES
        qfakexdgportalfixture_p.h qfakexdgportalfixture.cpp
        qfakexdgportal_p.h qfakexdgportal.cpp
        qsyntheticpipewirevideosource_p.h qsyntheticpipewirevideosource.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
        PipeWire::PipeWire
)
