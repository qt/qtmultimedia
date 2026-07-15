# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmediaplaylist.pro.

#####################################################################
## tst_qmediaplaylist Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qmediaplaylist
    SOURCES
        tst_qmediaplaylist.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
    TESTDATA
        testdata/empty.pls
        testdata/test.m3u
        testdata/test.pls
        testdata/testfile
        testdata/testfile2#suffix
        testdata/totem-pl-example.pls
)
