# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(audiodecoder)
add_subdirectory(mediadevices)
add_subdirectory(mediaformats)
add_subdirectory(media-metadata)
add_subdirectory(minimal-audio-recorder)
add_subdirectory(minimal-audio-callback-io)
add_subdirectory(minimal-player)
add_subdirectory(minimal-screen-recorder)
add_subdirectory(wasm)
add_subdirectory(mediaframeinputs)

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
    add_subdirectory(qml-screencapture-advanced)

    if(QT_FEATURE_gstreamer)
        add_subdirectory(qml-gstreamer-pipeline)
        add_subdirectory(qml-gstreamer-rtp)
    endif()
endif()
