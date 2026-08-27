# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpipewire_videoformat_support Test:
#####################################################################

if(NOT QT_FEATURE_pipewire)
    return()
endif()

qt_find_package(PipeWire PROVIDED_TARGETS PipeWire::PipeWire MODULE_NAME multimedia QMAKE_LIB pipewire)

qt_internal_multimedia_add_test(tst_qpipewire_videoformat_support
    SOURCES
        tst_qpipewire_videoformat_support.cpp
    LIBRARIES
        Qt::MultimediaPrivate
        PipeWire::PipeWire
)
