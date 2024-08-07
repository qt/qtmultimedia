# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudioringbuffer Test:
#####################################################################

qt_internal_add_test(tst_qaudioringbuffer
    SOURCES
        tst_qaudioringbuffer.cpp
    LIBRARIES
        Qt::MultimediaPrivate
)
