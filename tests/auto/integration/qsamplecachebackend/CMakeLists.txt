# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qsamplecachebackend Test:
#####################################################################

qt_internal_multimedia_add_test(tst_qsamplecachebackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qsamplecachebackend.cpp
    LIBRARIES
        Qt::Core
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA
        testdata/nokia-tune.mp3
        testdata/test.wav
)
