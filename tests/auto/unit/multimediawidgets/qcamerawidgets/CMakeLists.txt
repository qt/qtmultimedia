# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcamerawidgets Test:
#####################################################################

qt_internal_add_test(tst_qcamerawidgets
    SOURCES
        tst_qcamerawidgets.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
        Qt::MockMultimediaPlugin
)
