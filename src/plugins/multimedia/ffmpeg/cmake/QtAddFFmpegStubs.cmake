# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Utilities

function(qt_internal_multimedia_find_ffmpeg_stubs)
    foreach (stub ${FFMPEG_STUBS})
        if (${stub} MATCHES ${vaapi_regex})
            set(ffmpeg_has_vaapi TRUE PARENT_SCOPE)
        elseif (${stub} MATCHES ${openssl_regex})
            set(ffmpeg_has_openssl TRUE PARENT_SCOPE)
        else()
            set(unknown_ffmpeg_stubs
                ${unknown_ffmpeg_stubs} ${stub} PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

function(qt_internal_multimedia_check_ffmpeg_stubs_configuration)
    if (NOT LINUX AND NOT ANDROID)
        message(FATAL_ERROR "Currently, stubs are supported on Linux and Android")
    endif()

    if (unknown_ffmpeg_stubs)
        message(FATAL_ERROR "Unknown ffmpeg stubs: ${unknown_ffmpeg_stubs}")
    endif()

    if (ffmpeg_has_vaapi AND NOT QT_FEATURE_vaapi)
        message(FATAL_ERROR
               "QT_FEATURE_vaapi is OFF but FFmpeg includes VAAPI.")
    elseif (NOT ffmpeg_has_vaapi AND QT_FEATURE_vaapi)
        message(WARNING
               "QT_FEATURE_vaapi is ON "
               "but FFmpeg includes VAAPI and dynamic symbols resolve is enabled.")
    elseif(ffmpeg_has_vaapi AND NOT VAAPI_SUFFIX)
        message(FATAL_ERROR "Cannot find VAAPI_SUFFIX, fix FindVAAPI.cmake")
    elseif (ffmpeg_has_vaapi AND "${VAAPI_SUFFIX}" MATCHES "^1\\.32.*")
        # drop the ancient vaapi version to avoid ABI problems
        message(FATAL_ERROR "VAAPI ${VAAPI_SUFFIX} is not supported")
    endif()

    if (ffmpeg_has_openssl AND NOT QT_FEATURE_openssl)
        message(FATAL_ERROR
               "QT_FEATURE_openssl is OFF but FFmpeg includes OpenSSL.")
    endif()
endfunction()

macro(qt_internal_multimedia_find_vaapi_soversion)
    string(REGEX MATCH "^[0-9]+" va_soversion "${VAAPI_SUFFIX}")

    set(va-drm_soversion "${va_soversion}")
    set(va-x11_soversion "${va_soversion}")
endmacro()

macro(qt_internal_multimedia_find_openssl_soversion)
    # Update OpenSSL variables since OPENSSL_SSL_LIBRARY is not propagated to this place in some cases.
    qt_find_package(OpenSSL)

    if (NOT OPENSSL_INCLUDE_DIR AND OPENSSL_ROOT_DIR)
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    endif()

    if (LINUX)
        if (NOT OPENSSL_SSL_LIBRARY)
            message(FATAL_ERROR "OPENSSL_SSL_LIBRARY is not found")
        endif()

        get_filename_component(ssl_lib_realpath "${OPENSSL_SSL_LIBRARY}" REALPATH)
        string(REGEX MATCH "[0-9]+(\\.[0-9]+)*$" ssl_soversion "${ssl_lib_realpath}")
        string(REGEX REPLACE "^3(\\..*|$)" "3" ssl_soversion "${ssl_soversion}")
    endif()

    #TODO: enhance finding openssl version and throw an error if it's not found.

    set(crypto_soversion "${ssl_soversion}")
endmacro()

function(qt_internal_multimedia_set_stub_include_directories stub target)
    qt_internal_extend_target(${target}
        CONDITION ${stub} MATCHES "${openssl_regex}"
        INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")

    qt_internal_extend_target(${target}
        CONDITION ${stub} MATCHES "${vaapi_regex}"
        INCLUDE_DIRECTORIES "${VAAPI_INCLUDE_DIR}")
endfunction()

function(qt_internal_multimedia_define_stub_needed_version stub target)
    string(TOUPPER ${stub} prefix)
    string(REPLACE "-" "_" prefix ${prefix})

    target_compile_definitions(${target} PRIVATE
        "${prefix}_NEEDED_SOVERSION=\"${${stub}_soversion}\"")
endfunction()

function(qt_internal_multimedia_add_private_stub_to_plugin stub)
    qt_internal_multimedia_set_stub_include_directories(${stub} QFFmpegMediaPlugin)
    qt_internal_multimedia_define_stub_needed_version(${stub} QFFmpegMediaPlugin)
    qt_internal_extend_target(QFFmpegMediaPlugin SOURCES "symbolstubs/qffmpegsymbols-${stub}.cpp")
endfunction()

# Main function

set(vaapi_regex "^(va|va-drm|va-x11)$")
set(openssl_regex "^(ssl|crypto)$")

function(qt_internal_multimedia_add_ffmpeg_stubs)
    qt_internal_multimedia_find_ffmpeg_stubs()
    qt_internal_multimedia_check_ffmpeg_stubs_configuration()

    if (ffmpeg_has_openssl)
        qt_internal_multimedia_find_vaapi_soversion()
    endif()

    if (ffmpeg_has_openssl)
        qt_internal_multimedia_find_openssl_soversion()
    endif()

    foreach (stub ${FFMPEG_STUBS})
        if (FFMPEG_SHARED_LIBRARIES)
            # to be implemented
        else()
            qt_internal_multimedia_add_private_stub_to_plugin("${stub}")
        endif()
    endforeach()
endfunction()
