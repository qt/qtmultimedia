# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiobuffer Test:
#####################################################################

qt_internal_add_test(tst_qaudiobuffer
    SOURCES
        tst_qaudiobuffer.cpp
    LIBRARIES
        Qt::Multimedia Qt::MultimediaPrivate
)
