# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(CheckCXXSourceCompiles)

check_cxx_source_compiles("
    #include <ranges>
    int main() { return 0; }
" COMPILER_SUPPORTS_STD_RANGES)

add_subdirectory(audiodecoder)
add_subdirectory(media-metadata)
add_subdirectory(minimal-audio-recorder)
# minimal-audio-callback-io relies on <ranges>. At the time of writing,
# some targets in CI run compilers that do not support this.
if (COMPILER_SUPPORTS_STD_RANGES)
    add_subdirectory(minimal-audio-callback-io)
endif()
add_subdirectory(minimal-player)
add_subdirectory(minimal-screen-recorder)
add_subdirectory(wasm)
add_subdirectory(mediaframeinputs)

if (NOT ANDROID AND NOT IOS)
    add_subdirectory(mediadevices)
    add_subdirectory(mediaformats)
endif()

if(QT_FEATURE_gstreamer)
    add_subdirectory(gstreamer-custom-camera)
    add_subdirectory(gstreamer-custom-camera-rtp)
endif()

if(TARGET Qt::Quick)
    add_subdirectory(qml-minimal-audio-recorder)
    add_subdirectory(qml-minimal-camera)
    add_subdirectory(qml-minimal-player)
    add_subdirectory(qml-minimal-sound-effect)
    add_subdirectory(qml-multiple-players)
    add_subdirectory(qml-camera-advanced)
    add_subdirectory(qml-screencapture-advanced)

    if(QT_FEATURE_gstreamer)
        add_subdirectory(qml-gstreamer-pipeline)
        add_subdirectory(qml-gstreamer-rtp)
    endif()
endif()
