# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qcamerawidgets.pro.

#####################################################################
## tst_qcamerawidgets Test:
#####################################################################

qt_internal_add_test(tst_qcamerawidgets
    SOURCES
        tst_qcamerawidgets.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgetsPrivate
        Qt::Widgets
        QtMultimediaMockBackend
)
