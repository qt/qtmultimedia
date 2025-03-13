# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
qt_internal_add_module(MultimediaTestLibPrivate
    CONFIG_MODULE_NAME multimediatestlibprivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        audiogenerationutils_p.h
        capturesessionfixture_p.h capturesessionfixture.cpp
        framegenerator_p.h framegenerator.cpp
        mediabackendutils_p.h
        mediafileselector_p.h
        formatutils_p.h formatutils.cpp
        mediainfo_p.h
        multimedia_debug_support_p.h
        osdetection_p.h
        testvideosink_p.h
        qcolorutil_p.h qcolorutil.cpp
        qfileutil_p.h qfileutil.cpp
        qscopedenvironmentvariable_p.h
        qsequentialfileadaptor_p.h
        qsinewavevalidator_p.h qsinewavevalidator.cpp
        qmockiodevice_p.h
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
