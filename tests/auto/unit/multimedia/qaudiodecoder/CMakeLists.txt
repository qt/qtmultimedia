# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiodecoder Test:
#####################################################################

qt_internal_add_test(tst_qaudiodecoder
    SOURCES
        tst_qaudiodecoder.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
)
