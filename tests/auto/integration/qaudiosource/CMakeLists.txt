# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qaudiosource Test:
#####################################################################

qt_internal_add_test(tst_qaudiosource
    SOURCES
        tst_qaudiosource.cpp
    LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::MultimediaTestLibPrivate
)

if(APPLE)
    set_target_properties(tst_qaudiosource PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    )
endif()
