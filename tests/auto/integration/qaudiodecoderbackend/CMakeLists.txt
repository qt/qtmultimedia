# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qaudiodecoderbackend LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qaudiodecoderbackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        tst_qaudiodecoderbackend.cpp
    LIBRARIES
        Qt::Core
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA
        testdata/nokia-tune.mp3
        testdata/test-24bit.wav
        testdata/test-corrupted.wav
        testdata/test-float32.wav
        testdata/test-no-audio-track.mp4
        testdata/test-unsupported.avi
        testdata/test.aac
        testdata/test.flac
        testdata/test.ogg
        testdata/test.wav
)

qt_internal_multimedia_add_test(tst_qaudiodecoderbackend_drwav
    SOURCES
        tst_qaudiodecoderbackend.cpp
    LIBRARIES
        Qt::Core
        Qt::Multimedia
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
    TESTDATA
        testdata/nokia-tune.mp3
        testdata/test-24bit.wav
        testdata/test-corrupted.wav
        testdata/test-float32.wav
        testdata/test-no-audio-track.mp4
        testdata/test-unsupported.avi
        testdata/test.aac
        testdata/test.flac
        testdata/test.ogg
        testdata/test.wav
)

target_compile_definitions(tst_qaudiodecoderbackend_drwav PRIVATE QT_TEST_DRWAV_AUDIO_DECODER=1)

foreach(target tst_qaudiodecoderbackend tst_qaudiodecoderbackend_drwav)
    qt_internal_add_resource(${target} "qrc_testdata"
        PREFIX
            "/"
        FILES
            "testdata/test.wav"
    )
endforeach()
