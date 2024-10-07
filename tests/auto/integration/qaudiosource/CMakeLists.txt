# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiosource Test:
#####################################################################

qt_internal_add_test(tst_qaudiosource
    SOURCES
        tst_qaudiosource.cpp
        ../../shared/mediabackendutils.h
    INCLUDE_DIRECTORIES
        ../../shared/
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
