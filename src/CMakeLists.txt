# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


qt_internal_find_apple_system_framework(FWAVFoundation AVFoundation)
qt_internal_find_apple_system_framework(FWAppKit AppKit)
qt_internal_find_apple_system_framework(FWAudioToolbox AudioToolbox)
qt_internal_find_apple_system_framework(FWAudioUnit AudioUnit)
qt_internal_find_apple_system_framework(FWCoreAudio CoreAudio)
qt_internal_find_apple_system_framework(FWCoreMedia CoreMedia)
qt_internal_find_apple_system_framework(FWCoreVideo CoreVideo)
qt_internal_find_apple_system_framework(FWSecurity Security)
qt_internal_find_apple_system_framework(FWVideoToolbox VideoToolbox)


add_subdirectory(3rdparty/tlsf)
add_subdirectory(multimedia)
if(ANDROID)
    add_subdirectory(android)
endif()
if(QT_FEATURE_spatialaudio)
    add_subdirectory(resonance-audio)
    add_subdirectory(spatialaudio)
endif()

# special case begin
if(TARGET Qt::Quick)
    add_subdirectory(multimediaquick)
endif()
if(TARGET Qt::Quick3D AND QT_FEATURE_spatialaudio)
    add_subdirectory(spatialaudioquick3d)
endif()

if(TARGET Qt::Widgets)
    add_subdirectory(multimediawidgets)
endif()

add_subdirectory(plugins)

if(QT_FEATURE_testlib AND TARGET Qt::Test)
    add_subdirectory(multimediatestlib)
endif()
