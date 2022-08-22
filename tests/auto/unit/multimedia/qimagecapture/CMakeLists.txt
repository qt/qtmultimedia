# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qimagecapture.pro.

#####################################################################
## tst_qimagecapture Test:
#####################################################################

qt_internal_add_test(tst_qimagecapture
    SOURCES
        tst_qimagecapture.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)
