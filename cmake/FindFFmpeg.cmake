# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
#.rst:
# FindFFmpeg
# ----------
#
# Try to find the required ffmpeg components (default: AVFORMAT, AVUTIL, AVCODEC)
#
# Next variables can be used to hint FFmpeg libs search:
#
# ::
#
#   PC_<component>_LIBRARY_DIRS
#   PC_FFMPEG_LIBRARY_DIRS
#   PC_<component>_INCLUDE_DIRS
#   PC_FFMPEG_INCLUDE_DIRS
#
# Once done this will define
#
# ::
#
#   FFMPEG_FOUND         - System has the all required components.
#   FFMPEG_INCLUDE_DIRS  - Include directory necessary for using the required components headers.
#   FFMPEG_LIBRARIES     - Link these to use the required ffmpeg components.
#   FFMPEG_LIBRARY_DIRS  - Link directories
#   FFMPEG_DEFINITIONS   - Compiler switches required for using the required ffmpeg components.
#
# For each of the components it will additionally set.
#
# ::
#
#   AVCODEC
#   AVDEVICE
#   AVFORMAT
#   AVFILTER
#   AVUTIL
#   POSTPROC
#   SWSCALE
#
# the following variables will be defined
#
# ::
#
#   <component>_FOUND        - System has <component>
#   FFMPEG_<component>_FOUND - System has <component> (as checked by FHSPA)
#   <component>_INCLUDE_DIRS - Include directory necessary for using the <component> headers
#   <component>_LIBRARIES    - Link these to use <component>
#   <component>_LIBRARY_DIRS - Link directories
#   <component>_DEFINITIONS  - Compiler switches required for using <component>
#   <component>_VERSION      - The components version
#
# the following import targets is created
#
# ::
#
#   FFmpeg::FFmpeg - for all components
#   FFmpeg::<component> - where <component> in lower case (FFmpeg::avcodec) for each components
#
# Copyright (c) 2006, Matthias Kretz, <kretz@kde.org>
# Copyright (c) 2008, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2011, Michael Jansen, <kde@michael-jansen.biz>
# Copyright (c) 2017, Alexander Drozdov, <adrozdoff@gmail.com>
#

include(FindPackageHandleStandardArgs)

# The default components were taken from a survey over other FindFFMPEG.cmake files
if (NOT FFmpeg_FIND_COMPONENTS)
  set(FFmpeg_FIND_COMPONENTS AVCODEC AVFORMAT AVUTIL)
endif ()

#
### Macro: set_component_found
#
# Marks the given component as found if both *_LIBRARIES AND *_INCLUDE_DIRS is present.
#
macro(set_component_found _component )
  if (${_component}_LIBRARIES AND ${_component}_INCLUDE_DIRS)
    # message(STATUS "  - ${_component} found.")
    set(${_component}_FOUND TRUE)
    set(${CMAKE_FIND_PACKAGE_NAME}_${_component}_FOUND TRUE)
  else ()
    # message(STATUS "  - ${_component} not found.")
  endif ()
endmacro()

find_package(PkgConfig QUIET)
if (NOT PKG_CONFIG_FOUND AND NOT FFMPEG_DIR)
    set(FFMPEG_DIR "/usr/local")
endif()
#
### Macro: find_component
#
# Checks for the given component by invoking pkgconfig and then looking up the libraries and
# include directories.
#
macro(find_component _component _pkgconfig _library _header)

  # use pkg-config to get the directories and then use these values
  # in the FIND_PATH() and FIND_LIBRARY() calls
  if (PKG_CONFIG_FOUND AND NOT FFMPEG_DIR)
    pkg_check_modules(PC_${_component} ${_pkgconfig})
  endif ()

  if (FFMPEG_DIR OR FFMPEG_ROOT)
      set(__find_ffmpeg_backup_root_dir "${CMAKE_FIND_ROOT_PATH}")
  endif()

  if(FFMPEG_DIR)
       list(APPEND CMAKE_FIND_ROOT_PATH "${FFMPEG_DIR}")
  endif()

  if(FFMPEG_ROOT)
      list(APPEND CMAKE_FIND_ROOT_PATH "${FFMPEG_ROOT}")
  endif()

  find_path(${_component}_INCLUDE_DIRS ${_header}
    HINTS
      ${PC_${_component}_INCLUDEDIR}
      ${PC_${_component}_INCLUDE_DIRS}
      ${PC_FFMPEG_INCLUDE_DIRS}
    PATHS
      ${FFMPEG_DIR}
    PATH_SUFFIXES
      ffmpeg include
  )

  find_library(${_component}_LIBRARY NAMES ${PC_${_component}_LIBRARIES} ${_library}
    HINTS
      ${PC_${_component}_LIBDIR}
      ${PC_${_component}_LIBRARY_DIRS}
      ${PC_FFMPEG_LIBRARY_DIRS}
    PATHS
      ${FFMPEG_DIR}
    PATH_SUFFIXES
      lib
  )

  if(FFMPEG_DIR OR FFMPEG_ROOT)
    set(CMAKE_FIND_ROOT_PATH "${__find_ffmpeg_backup_root_dir}")
  endif()

  get_filename_component(${_component}_LIBRARY_DIR_FROM_FIND ${${_component}_LIBRARY} DIRECTORY)
  get_filename_component(${_component}_LIBRARY_FROM_FIND ${${_component}_LIBRARY} NAME)

  set(${_component}_DEFINITIONS  ${PC_${_component}_CFLAGS_OTHER}       CACHE STRING "The ${_component} CFLAGS.")
  set(${_component}_VERSION      ${PC_${_component}_VERSION}            CACHE STRING "The ${_component} version number.")
  set(${_component}_LIBRARY_DIRS ${${_component}_LIBRARY_DIR_FROM_FIND} CACHE STRING "The ${_component} library dirs.")
  set(${_component}_LIBRARIES    ${${_component}_LIBRARY_FROM_FIND}     CACHE STRING "The ${_component} libraries.")

#  message("Libs" ${FFMPEG_DIR} ${${_component}_LIBRARIES} ${${_component}_LIBRARY_DIRS})

#  message(STATUS "L0: ${${_component}_LIBRARIES}")
#  message(STATUS "L1: ${PC_${_component}_LIBRARIES}")
#  message(STATUS "L2: ${_library}")
#  message(STATUS "L3: ${${_component}_LIBRARY}")
#  message(STATUS "L4: ${${_component}_LIBRARY_DIRS}")

  set_component_found(${_component})
  mark_as_advanced(
    ${_component}_LIBRARY
    ${_component}_INCLUDE_DIRS
    ${_component}_LIBRARY_DIRS
    ${_component}_LIBRARIES
    ${_component}_DEFINITIONS
    ${_component}_VERSION)

endmacro()

# Clear the previously cached variables, because they are recomputed every time
# the Find script is included.
set(FFMPEG_INCLUDE_DIRS "")
set(FFMPEG_LIBRARIES "")
set(FFMPEG_DEFINITIONS "")
set(FFMPEG_LIBRARY_DIRS "")

# Check for all possible component.
find_component(AVCODEC    libavcodec    avcodec  libavcodec/avcodec.h)
find_component(AVFORMAT   libavformat   avformat libavformat/avformat.h)
find_component(AVDEVICE   libavdevice   avdevice libavdevice/avdevice.h)
find_component(AVUTIL     libavutil     avutil   libavutil/avutil.h)
find_component(AVFILTER   libavfilter   avfilter libavfilter/avfilter.h)
find_component(SWSCALE    libswscale    swscale  libswscale/swscale.h)
find_component(POSTPROC   libpostproc   postproc libpostproc/postprocess.h)
find_component(SWRESAMPLE libswresample swresample libswresample/swresample.h)

# Linking to private FFmpeg libraries is only needed if it was built statically
# Only one of the components needs to be tested
if(AVCODEC_LIBRARY AND ${AVCODEC_LIBRARY} MATCHES "\\${CMAKE_STATIC_LIBRARY_SUFFIX}$")
  set(FFMPEG_IS_STATIC TRUE CACHE INTERNAL "")
endif()

if (FFMPEG_IS_STATIC AND (ANDROID OR LINUX))
  set(ENABLE_DYNAMIC_RESOLVE_OPENSSL_SYMBOLS TRUE CACHE INTERNAL "")
endif()

set(ENABLE_DYNAMIC_RESOLVE_VAAPI_SYMBOLS ${LINUX} CACHE INTERNAL "")

function(__try_add_dynamic_resolve_dependency dep added)
  set(added TRUE PARENT_SCOPE)

  if(ENABLE_DYNAMIC_RESOLVE_OPENSSL_SYMBOLS AND
      (${dep} STREQUAL "ssl" OR ${dep} STREQUAL "crypto"))
    set(DYNAMIC_RESOLVE_OPENSSL_SYMBOLS TRUE CACHE INTERNAL "")
  elseif(ENABLE_DYNAMIC_RESOLVE_VAAPI_SYMBOLS AND ${dep} STREQUAL "va")
    set(DYNAMIC_RESOLVE_VAAPI_SYMBOLS TRUE CACHE INTERNAL "")
  elseif(ENABLE_DYNAMIC_RESOLVE_VAAPI_SYMBOLS AND ${dep} STREQUAL "va-drm")
    set(DYNAMIC_RESOLVE_VA_DRM_SYMBOLS TRUE CACHE INTERNAL "")
  elseif(ENABLE_DYNAMIC_RESOLVE_VAAPI_SYMBOLS AND ${dep} STREQUAL "va-x11")
    set(DYNAMIC_RESOLVE_VA_X11_SYMBOLS TRUE CACHE INTERNAL "")
  else()
    set(added FALSE PARENT_SCOPE)
  endif()
endfunction()

# Function parses package config file to find the static library dependencies
# and adds them to the target library.
function(__ffmpeg_internal_set_dependencies lib)
  set(PC_FILE ${FFMPEG_DIR}/lib/pkgconfig/lib${lib}.pc)
  if(EXISTS ${PC_FILE})
    file(READ ${PC_FILE} pcfile)

    set(prefix_l "(^| )\\-l")
    set(suffix_lib "\\.lib($| )")

    string(REGEX REPLACE ".*Libs:([^\n\r]+).*" "\\1" out "${pcfile}")
    string(REGEX MATCHALL "${prefix_l}[^ ]+" libs_dependency ${out})
    string(REGEX MATCHALL "[^ ]+${suffix_lib}" libs_dependency_lib ${out})

    string(REGEX REPLACE ".*Libs.private:([^\n\r]+).*" "\\1" out "${pcfile}")
    string(REGEX MATCHALL "${prefix_l}[^ ]+" libs_private_dependency ${out})
    string(REGEX MATCHALL "[^ ]+${suffix_lib}" libs_private_dependency_lib ${out})

    list(APPEND deps_no_suffix ${libs_dependency} ${libs_private_dependency})
    foreach(dependency ${deps_no_suffix})
      string(REGEX REPLACE ${prefix_l} "" dependency ${dependency})
      if(NOT ${lib} STREQUAL ${dependency})
        __try_add_dynamic_resolve_dependency(${dependency} added)
        if(NOT added)
          target_link_libraries(FFmpeg::${lib} INTERFACE ${dependency})
        endif()
      endif()
    endforeach()

    list(APPEND deps_lib_suffix ${libs_dependency_lib} ${libs_private_dependency_lib})
    foreach(dependency ${deps_lib_suffix})
      string(REGEX REPLACE ${suffix_lib} "" dependency ${dependency})
      target_link_libraries(FFmpeg::${lib} INTERFACE ${dependency})
    endforeach()
  endif()
endfunction()

# Check for cached results. If there are skip the costly part.
#if (NOT FFMPEG_LIBRARIES)

  # Check if the required components were found and add their stuff to the FFMPEG_* vars.
  foreach (_component ${FFmpeg_FIND_COMPONENTS})
    if (${_component}_FOUND)
      # message(STATUS "Libs: ${${_component}_LIBRARIES} | ${PC_${_component}_LIBRARIES}")

      # message(STATUS "Required component ${_component} present.")
      set(FFMPEG_LIBRARIES    ${FFMPEG_LIBRARIES}    ${${_component}_LIBRARY} ${${_component}_LIBRARIES})
      set(FFMPEG_DEFINITIONS  ${FFMPEG_DEFINITIONS}  ${${_component}_DEFINITIONS})

      list(APPEND FFMPEG_INCLUDE_DIRS ${${_component}_INCLUDE_DIRS})
      list(APPEND FFMPEG_LIBRARY_DIRS ${${_component}_LIBRARY_DIRS})

      string(TOLOWER ${_component} _lowerComponent)
      if (NOT TARGET FFmpeg::${_lowerComponent})
        add_library(FFmpeg::${_lowerComponent} INTERFACE IMPORTED)
        set_target_properties(FFmpeg::${_lowerComponent} PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${${_component}_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES ${${_component}_INCLUDE_DIRS}
            INTERFACE_LINK_LIBRARIES "${${_component}_LIBRARIES}"
            INTERFACE_LINK_DIRECTORIES "${${_component}_LIBRARY_DIRS}"
        )
        if(FFMPEG_IS_STATIC)
            __ffmpeg_internal_set_dependencies(${_lowerComponent})
        endif()
        target_link_libraries(FFmpeg::${_lowerComponent} INTERFACE "${${_component}_LIBRARY}")
        if (UNIX AND NOT APPLE)
          target_link_options(FFmpeg::${_lowerComponent} INTERFACE  "-Wl,--exclude-libs=lib${_lowerComponent}")
        endif ()
    endif()
    else()
      # message(STATUS "Required component ${_component} missing.")
    endif()
  endforeach ()

  # Build the include path with duplicates removed.
  if (FFMPEG_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIRS)
  endif ()

  # cache the vars.
  set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIRS} CACHE STRING "The FFmpeg include directories." FORCE)
  set(FFMPEG_LIBRARIES    ${FFMPEG_LIBRARIES}    CACHE STRING "The FFmpeg libraries." FORCE)
  set(FFMPEG_DEFINITIONS  ${FFMPEG_DEFINITIONS}  CACHE STRING "The FFmpeg cflags." FORCE)
  set(FFMPEG_LIBRARY_DIRS ${FFMPEG_LIBRARY_DIRS} CACHE STRING "The FFmpeg library dirs." FORCE)

  mark_as_advanced(FFMPEG_INCLUDE_DIRS
                   FFMPEG_LIBRARIES
                   FFMPEG_DEFINITIONS
                   FFMPEG_LIBRARY_DIRS)

#endif ()

if (NOT TARGET FFmpeg::FFmpeg)
  add_library(FFmpeg INTERFACE)
  set_target_properties(FFmpeg PROPERTIES
      INTERFACE_COMPILE_OPTIONS "${FFMPEG_DEFINITIONS}"
      INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}"
      INTERFACE_LINK_DIRECTORIES "${FFMPEG_LIBRARY_DIRS}")
  add_library(FFmpeg::FFmpeg ALIAS FFmpeg)
endif()

# Now set the noncached _FOUND vars for the components.
foreach (_component AVCODEC AVDEVICE AVFORMAT AVUTIL POSTPROCESS SWSCALE)
  set_component_found(${_component})
endforeach ()

# Compile the list of required vars
set(_FFmpeg_REQUIRED_VARS FFMPEG_LIBRARIES FFMPEG_INCLUDE_DIRS)
foreach (_component ${FFmpeg_FIND_COMPONENTS})
  list(APPEND _FFmpeg_REQUIRED_VARS ${_component}_LIBRARIES ${_component}_INCLUDE_DIRS)
endforeach ()

# Give a nice error message if some of the required vars are missing.
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS ${_FFmpeg_REQUIRED_VARS}
    HANDLE_COMPONENTS
)
