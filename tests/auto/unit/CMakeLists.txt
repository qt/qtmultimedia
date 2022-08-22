# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from unit.pro.

add_subdirectory(mockbackend)
add_subdirectory(multimedia)
if(TARGET Qt::Widgets)
    add_subdirectory(multimediawidgets)
endif()
