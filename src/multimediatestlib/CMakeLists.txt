# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
qt_internal_add_module(MultimediaTestLibPrivate
    CONFIG_MODULE_NAME multimediatestlibprivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        mediabackendutils_p.h
        qscopedenvironmentvariable_p.h
        qsequentialfileadaptor_p.h
        qsinewavevalidator_p.h qsinewavevalidator.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Multimedia
        Qt::MultimediaPrivate
    NO_GENERATE_CPP_EXPORTS
    QT_LICENSE_ID QT_COMMERCIAL_OR_GPL3
)
