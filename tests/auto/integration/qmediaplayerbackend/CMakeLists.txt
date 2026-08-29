# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmediaplayerbackend LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_multimedia_add_test(tst_qmediaplayerbackend
    NEEDS_MEDIA_BACKEND_PLUGINS
    SOURCES
        mediaplayerstate.h
        fake.h
        fixture.h
        server.h
        tst_qmediaplayerbackend.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
        Qt::MultimediaTestLibPrivate
    BUILTIN_TESTDATA
    TESTDATA
        "LazyLoad.qml"
        testdata/_test.wav
        testdata/15s.mkv
        testdata/3colors_with_sound_1s.mp4
        testdata/BigBuckBunny.mp4
        testdata/audio_video_with_jpg_thumbnail.mp4
        testdata/audio_video_with_png_thumbnail.mp4
        testdata/busAv1.webm
        testdata/busMpeg4.mp4
        testdata/color_matrix_180_deg_clockwise.mp4
        testdata/color_matrix_180_deg_clockwise_mirrored.mp4
        testdata/color_matrix_270_deg_clockwise.mp4
        testdata/color_matrix_270_deg_clockwise_mirrored.mp4
        testdata/color_matrix_90_deg_clockwise.mp4
        testdata/color_matrix_90_deg_clockwise_mirrored.mp4
        testdata/color_matrix_mirrored.mp4
        testdata/color_matrix.mp4
        testdata/colors.mp4
        testdata/colors.ogv
        testdata/corrupt_end.ogg
        testdata/duration_issues.webm
        testdata/h264_avc1_yuv420p10le_tv_bt2020.mov
        testdata/invalid_media.m3u
        testdata/invalid_media2.m3u
        testdata/multitrack-subtitle-start-at-zero.mkv
        testdata/multitrack.mkv
        testdata/nested1.m3u
        testdata/nested2.m3u
        testdata/nokia-tune.mkv
        testdata/nokia-tune.mp3
        testdata/one_red_frame.mp4
        testdata/par_2_3.mp4
        testdata/par_3_2.mp4
        testdata/recursive.m3u
        testdata/recursive_master.m3u
        testdata/sample.m3u
        testdata/subtitletest.mkv
        testdata/test.wav
        testdata/with_creation_time.mp4
        testdata/with_date.mp4
        testdata/with_date_and_creation_time.mp4
        testdata/with_qtdate_and_creation_time.mov
)
