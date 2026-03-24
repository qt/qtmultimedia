# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function is in Technology Preview.
if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_add_ios_ffmpeg_libraries target)
        if(QT_DEFAULT_MAJOR_VERSION EQUAL 6)
            qt6_add_ios_ffmpeg_libraries(${target})
        else()
            message(FATAL_ERROR "qt_add_ios_ffmpeg_libraries() is only available in Qt 6.")
        endif()
    endfunction()
endif()

function(qt6_add_ios_ffmpeg_libraries target)
    if(NOT IOS)
        return()
    endif()

    if(NOT TARGET ${target})
        message(FATAL_ERROR "'${target}' is not a target")
    endif()

    if(CMAKE_VERSION VERSION_LESS 3.28)
        message(FATAL_ERROR "qt_add_ios_ffmpeg_libraries() requires CMake version 3.28 or later.")
    endif()

    # TODO: These values should be pulled from somewhere in Qt Multimedia config.
    set(wanted_ffmpeg_components avcodec avformat avutil swresample swscale)
    set(ffmpeg_frameworks "")
    set(missing_frameworks "")
    foreach(component ${wanted_ffmpeg_components})
        # Find the corresponding shared library that was installed
        set(path "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}/ffmpeg/lib${component}.xcframework")
        list(APPEND ffmpeg_frameworks "${path}")
        if(NOT EXISTS "${path}")
            list(APPEND missing_frameworks ${path})
        endif()
    endforeach()
    if(missing_frameworks)
        message(FATAL_ERROR "CMake script links against deployed FFmpeg libraries, but the following files were missing: ${missing_frameworks}")
    endif()
    if(NOT ffmpeg_frameworks)
        message(FATAL_ERROR "CMake script links against deployed FFmpeg libraries, but none were found")
    endif()

    get_target_property(target_type ${target} TYPE)
    if(${target_type} STREQUAL "EXECUTABLE")
        set_property(TARGET ${target} APPEND PROPERTY XCODE_EMBED_FRAMEWORKS "${ffmpeg_frameworks}")
        set_property(TARGET ${target}
            APPEND PROPERTY XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks")

        if(NOT QT_NO_FFMPEG_XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY)
            set_property(TARGET ${target} PROPERTY XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
        endif()

        target_link_libraries(${target} PRIVATE ${ffmpeg_frameworks})
    else()
        message(
            FATAL_ERROR
            "CMake function qt_add_ios_ffmpeg_libraries should only be used on executable targets")
    endif()

endfunction()
