# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qscreencapturebackend Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qscreencapturebackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qscreencapturebackend.cpp
    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::Gui
        Qt::Widgets
        Qt::MultimediaTestLibPrivate
)

if(ANDROID)
    set_property(TARGET tst_qscreencapturebackend
        PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
endif()
