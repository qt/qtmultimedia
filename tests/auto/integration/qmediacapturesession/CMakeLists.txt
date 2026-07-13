# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcamerabackend Test:
#####################################################################

qt_internal_add_test(tst_qmediacapturesession
    SOURCES
        tst_qmediacapturesession.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaWidgets
        Qt::MultimediaTestLibPrivate
)

if(APPLE)
    set_target_properties(tst_qmediacapturesession PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    )
endif()

if(OHOS)
    # The stress_test_setup_and_teardown_* subtests can run for tens of seconds
    # without emitting any progress, which trips the 60 s no-progress watchdog
    # in harmonyostestrunner. Disable that watchdog for this test; the overall
    # test timeout still bounds runtime.
    set_property(TEST tst_qmediacapturesession APPEND PROPERTY
        ENVIRONMENT "HARMONYOS_NO_PROGRESS_TIMEOUT=0")
endif()

if(IOS)
    qt_add_ios_ffmpeg_libraries(tst_qmediacapturesession)
endif()
