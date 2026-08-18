# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_multimedia_add_test name)
    set(options
        MACOS_BUNDLE          # Add a macOS bundle target, requires Info.plist
        NEEDS_MEDIA_BACKEND_PLUGINS # Add dependency to qt_plugins_qtmultimedia target
                              # IOS: Add ffmpeg libraries to the target
    )
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 1 args "${options}" "${oneValueArgs}" "${multiValueArgs}")

    qt_internal_add_test(${name} ${args_UNPARSED_ARGUMENTS})

    if(NOT TARGET ${name})
        return()
    endif()

    if(args_MACOS_BUNDLE AND APPLE)
        set_target_properties(${name} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        )
    endif()

    if(args_NEEDS_MEDIA_BACKEND_PLUGINS)
        if(IOS)
            qt_add_ios_ffmpeg_libraries(${name})
        endif()

        if(TARGET qt_plugins_qtmultimedia)
            add_dependencies(${name} qt_plugins_qtmultimedia)
        endif()
    endif()
endfunction()
