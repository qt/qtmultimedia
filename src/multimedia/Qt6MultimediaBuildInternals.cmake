# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


function(qt_internal_multimedia_get_ffmpeg_stub_libraries out_var)
    set(result "")

    set(stub_prefix "${QT_CMAKE_EXPORT_NAMESPACE}FFmpegStub-")
    string(TOUPPER "${QT_CMAKE_EXPORT_NAMESPACE}" install_paths_prefix)

    set(searched_dirs "")
    if(${install_paths_prefix}_INSTALL_PREFIX AND ${install_paths_prefix}_INSTALL_LIBS)
        list(APPEND searched_dirs
            "${${install_paths_prefix}_INSTALL_PREFIX}/${${install_paths_prefix}_INSTALL_LIBS}")
    endif()

    if(CMAKE_CURRENT_FUNCTION_LIST_DIR)
        get_filename_component(config_libdir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
        if(NOT config_libdir IN_LIST searched_dirs)
            list(APPEND searched_dirs "${config_libdir}")
        endif()
    endif()

    if(QT_FEATURE_ffmpeg_stubs)
        foreach(stub IN ITEMS ssl crypto va va-drm va-x11)
            if(TARGET ${stub_prefix}${stub})
                list(APPEND result ${stub_prefix}${stub})
            endif()
        endforeach()

        if(NOT result)
            foreach(dir IN LISTS searched_dirs)
                file(GLOB stub_files "${dir}/lib${stub_prefix}*${CMAKE_SHARED_LIBRARY_SUFFIX}*")

                foreach(stub_file IN LISTS stub_files)
                    get_filename_component(stub_file "${stub_file}" REALPATH)
                    if(NOT stub_file IN_LIST result)
                        list(APPEND result "${stub_file}")
                    endif()
                endforeach()
            endforeach()
        endif()
    endif()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

function(qt_internal_multimedia_link_ffmpeg_stubs target)
    qt_internal_multimedia_get_ffmpeg_stub_libraries(stub_libraries)

    if(stub_libraries)
        target_link_options(${target} PRIVATE "LINKER:--no-as-needed")
        target_link_libraries(${target} PRIVATE ${stub_libraries})
    elseif(QT_FEATURE_ffmpeg_stubs AND BUILD_SHARED_LIBS)
        message(FATAL_ERROR
            "FFmpeg is used with symbol stubs, but no stub library was found for "
            "target ${target}. Linking it would fail with undefined references "
            "into OpenSSL or VAAPI. Reconfigure with "
            "-DQT_MULTIMEDIA_DEBUG_FFMPEG_STUBS=ON for details.")
    endif()
endfunction()

function(qt_internal_multimedia_add_test name)
    set(options
        MACOS_BUNDLE          # Add a macOS bundle target, requires Info.plist
        NEEDS_MEDIA_BACKEND_PLUGINS # Add dependency to qt_plugins_qtmultimedia target
                              # IOS: Add ffmpeg libraries to the target
        LINK_FFMPEG_STUBS     # Required for executables linking Qt::FFmpegMediaPluginImplPrivate
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

    if(args_LINK_FFMPEG_STUBS)
        qt_internal_multimedia_link_ffmpeg_stubs(${name})
    endif()
endfunction()
