# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsamplecache Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qsamplecache
    SOURCES
        tst_qsamplecache.cpp
    LIBRARIES
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MockMultimediaPlugin
    TESTDATA
        testdata/corrupted.wav
        testdata/drwav_heap-buffer-overflow.wav
        testdata/drwav_integer-underflow.wav
        testdata/nokia-tune.mp3
        testdata/test.wav
        testdata/test2.wav
)
