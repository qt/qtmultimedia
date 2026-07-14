# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiodecoderbackend Test:
#####################################################################

qt_internal_add_test(tst_qaudiodecoderbackend
    SOURCES
        tst_qaudiodecoderbackend.cpp
    LIBRARIES
        Qt::Core
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA
        testdata/test.wav
        testdata/test-corrupted.wav
        testdata/test-no-audio-track.mp4
        testdata/test-unsupported.avi
)

qt_internal_add_resource(tst_qaudiodecoderbackend "qrc_testdata"
    PREFIX
        "/"
    FILES
        "testdata/test.wav"
)

if(IOS)
    qt_add_ios_ffmpeg_libraries(tst_qaudiodecoderbackend)
endif()
