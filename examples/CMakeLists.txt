# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

# Pass 3rd party dependency locations to examples as external projects.
if(NOT FFMPEG_DIR AND NOT "$ENV{FFMPEG_DIR}" STREQUAL "")
    set(FFMPEG_DIR "$ENV{FFMPEG_DIR}")
endif()

set(QT_EXAMPLE_CMAKE_VARS_TO_PASS
    FFMPEG_DIR:STRING
)

qt_examples_build_begin(EXTERNAL_BUILD)

add_subdirectory(multimedia)
if(QT_FEATURE_spatialaudio)
    add_subdirectory(spatialaudio)
endif()

qt_examples_build_end()
