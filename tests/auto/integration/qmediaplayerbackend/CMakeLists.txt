# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmediaplayerbackend.pro.

#####################################################################
## tst_qmediaplayerbackend Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND testdata_resource_files ${test_data_glob})

qt_internal_add_test(tst_qmediaplayerbackend
    SOURCES
        ../shared/mediafileselector.h
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
    TESTDATA ${testdata_resource_files}
)

qt_internal_add_resource(tst_qmediaplayerbackend "testdata"
    PREFIX
        "/"
    FILES
        ${testdata_resource_files}
        "LazyLoad.qml"
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmediaplayerbackend CONDITION boot2qt
    DEFINES
        SKIP_OGV_TEST
)
