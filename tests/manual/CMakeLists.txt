# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(audiodecoder)
add_subdirectory(devices)
add_subdirectory(minimal-player)
add_subdirectory(wasm)

if(TARGET Qt::Quick)
    add_subdirectory(qml-minimal-camera)
    add_subdirectory(qml-minimal-player)

    if (QT_FEATURE_gstreamer)
        add_subdirectory(qml-gstreamer-rtp)
    endif()
endif()
