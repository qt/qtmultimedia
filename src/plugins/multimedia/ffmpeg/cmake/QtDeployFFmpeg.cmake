# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_multimedia_set_ffmpeg_link_directory directory)
    foreach (lib ${ffmpeg_libs})
        set_target_properties(${lib} PROPERTIES INTERFACE_LINK_DIRECTORIES ${directory})
    endforeach()
endfunction()

function(qt_internal_multimedia_copy_or_install_ffmpeg)
    if (WIN32)
        set(install_dir ${INSTALL_BINDIR})
    elseif (UIKIT)
        set(install_dir ${INSTALL_LIBDIR}/ffmpeg)
    else()
        set(install_dir ${INSTALL_LIBDIR})
    endif()

    if (UIKIT)
        set(ffmpeg_frameworks "")
        foreach(lib_path ${FFMPEG_SHARED_LIBRARIES})
            get_filename_component(path ${lib_path} DIRECTORY)
            get_filename_component(lib_name_we ${lib_path} NAME_WE)
            if (EXISTS "${path}/../framework/${lib_name_we}.xcframework")
                list(APPEND ffmpeg_frameworks "${path}/../framework/${lib_name_we}.xcframework")
            else()
                message(WARNING "${path}/../framework/${lib_name_we}.xcframework does not exist")
            endif()
        endforeach()
        list(REMOVE_DUPLICATES ffmpeg_frameworks)
        # Fail the build if we failed to find any of the FFmpeg frameworks we wanted to deploy.
        # This can happen if there's an incompatibility between CI ffmpeg-install script and
        # this deployment script.
        if (NOT ffmpeg_frameworks)
            message(FATAL_ERROR "Attempted to install iOS FFmpeg Frameworks but none were found.")
        endif()
    endif()

    if (QT_WILL_INSTALL)
        if (UIKIT)
            qt_install(DIRECTORY "${ffmpeg_frameworks}" DESTINATION ${install_dir})
        else()
            qt_install(FILES "${FFMPEG_SHARED_LIBRARIES}" DESTINATION ${install_dir})
        endif()
    else()
        # elseif(NOT WIN32) actually we can just drop the coping for unix platforms
        #                   However, it makes sense to copy anyway for consistency:
        #                   in order to have the same configuration for developer builds.

        set(ffmpeg_output_dir "${QT_BUILD_DIR}/${install_dir}")
        file(MAKE_DIRECTORY ${ffmpeg_output_dir})

        if (UIKIT)
            foreach(lib_path ${ffmpeg_frameworks})
                get_filename_component(lib_name ${lib_path} NAME)
                if(NOT EXISTS "${ffmpeg_output_dir}/${lib_name}")
                    file(COPY ${lib_path} DESTINATION ${ffmpeg_output_dir})
                endif()
            endforeach()
        else()
            foreach(lib_path ${FFMPEG_SHARED_LIBRARIES})
                get_filename_component(lib_name ${lib_path} NAME)
                if(NOT EXISTS "${ffmpeg_output_dir}/${lib_name}")
                    file(COPY ${lib_path} DESTINATION ${ffmpeg_output_dir})
                endif()
            endforeach()
        endif()

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
