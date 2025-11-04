# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_multimedia_set_ffmpeg_link_directory directory)
    foreach (lib ${ffmpeg_libs})
        set_target_properties(${lib} PROPERTIES INTERFACE_LINK_DIRECTORIES ${directory})
    endforeach()
endfunction()

function(qt_internal_multimedia_validate_ffmpeg_shared_libs libs)
    if (NOT libs)
        message(FATAL_ERROR "Attempted to install FFmpeg shared libraries but the list is empty")
    endif()

    foreach(lib_path IN LISTS libs)
        if (NOT EXISTS "${lib_path}")
            message(FATAL_ERROR "FFmpeg shared library file '${lib_path}' does not exist")
        endif()
    endforeach()
endfunction()

function(qt_internal_multimedia_copy_or_install_ffmpeg)
    qt_internal_multimedia_validate_ffmpeg_shared_libs(${FFMPEG_SHARED_LIBRARIES})

    if (WIN32)
        set(install_dir ${INSTALL_BINDIR})
    elseif (UIKIT)
        set(install_dir ${INSTALL_LIBDIR}/ffmpeg)
    else()
        set(install_dir ${INSTALL_LIBDIR})
    endif()

    if (QT_WILL_INSTALL)
        if (UIKIT)
            # iOS uses .xcframework files for dynamic linking. These
            # count as directories by the qt_install function.
            qt_install(DIRECTORY "${FFMPEG_SHARED_LIBRARIES}" DESTINATION ${install_dir})
        else()
            qt_install(FILES "${FFMPEG_SHARED_LIBRARIES}" DESTINATION ${install_dir})
        endif()
    else()
        # elseif(NOT WIN32) actually we can just drop the coping for unix platforms
        #                   However, it makes sense to copy anyway for consistency:
        #                   in order to have the same configuration for developer builds.

        set(ffmpeg_output_dir "${QT_BUILD_DIR}/${install_dir}")
        file(MAKE_DIRECTORY ${ffmpeg_output_dir})

        foreach(lib_path ${FFMPEG_SHARED_LIBRARIES})
            get_filename_component(lib_name ${lib_path} NAME)
            if(NOT EXISTS "${ffmpeg_output_dir}/${lib_name}")
                file(COPY ${lib_path} DESTINATION ${ffmpeg_output_dir})
            endif()
        endforeach()

        # On Windows, shared linking goes through 'integration' static libs,
        # otherwise we should link the directory with copied libs
        # On iOS we are using frameworks, not shared libraries.
        if (NOT WIN32 AND NOT UIKIT)
            qt_internal_multimedia_set_ffmpeg_link_directory(${ffmpeg_output_dir})
        endif()
    endif()

    # Should we set the compile definition for the plugin or for the QtMM module?
    # target_compile_definitions(QFFmpegMediaPlugin PRIVATE FFMPEG_DEPLOY_FOLDER="${FFMPEG_DEPLOY_FOLDER}")
endfunction()
