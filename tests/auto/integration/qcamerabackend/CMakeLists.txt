# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcamerabackend Test:
#####################################################################

qt_internal_add_test(tst_qcamerabackend
    SOURCES
        tst_qcamerabackend.cpp
    LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Widgets
        Qt::MultimediaTestLibPrivate
)

# special case begin
if(APPLE)
    set_property(TARGET tst_qcamerabackend PROPERTY MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist")
    set_property(TARGET tst_qcamerabackend PROPERTY PROPERTY MACOSX_BUNDLE TRUE)
endif()
# special case end
