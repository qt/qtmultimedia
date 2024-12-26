# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_multimedia_static_linking
    SOURCES
        tst_multimedia_static_linking.cpp
    LIBRARIES
        Qt::Multimedia
)

foreach(plugin Android Darwin Gstreamer Qnx FFMpeg Wasm Windows)
    if (TARGET Qt::Q${plugin}MediaPlugin)
        target_link_libraries(tst_multimedia_static_linking PUBLIC Qt::Q${plugin}MediaPlugin)
    endif()
endforeach()
