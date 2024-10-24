# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function is in Technical Preview.
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

    file (GLOB ffmpeg_dylibs "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_LIBS}/ffmpeg/*.*.dylib")
    if(NOT ffmpeg_dylibs)
        return()
    endif()
    set_property(TARGET ${target} APPEND PROPERTY XCODE_EMBED_FRAMEWORKS "${ffmpeg_dylibs}")

    set_property(TARGET ${target} APPEND PROPERTY
                 XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/")

    if(NOT QT_NO_FFMPEG_XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY)
        set_property(TARGET ${target} PROPERTY XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
    endif()

    target_link_libraries(${target} PRIVATE ${ffmpeg_dylibs})
endfunction()
