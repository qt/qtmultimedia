# FindGStreamer
# ---------
#
# Locate the gstreamer-1.0 library and some of its plugins.
# Defines the following imported target:
#
#   ``GStreamer::GStreamer``
#       If the gstreamer-1.0 library is available and target GStreamer::Base,
#       GStreamer::Audio, GStreamer::Video, GStreamer::Pbutils and
#       GStreamer::Allocators exist
#
#  If target GStreamer::GStreamer exists, the following targets may be defined:
#
#   ``GStreamer::App``
#       If the gstapp-1.0 library is available and target GStreamer::GStreamer exists
#   ``GStreamer::Photography``
#       If the gstphotography-1.0 library is available and target GStreamer::GStreamer exists
#   ``GStreamer::Gl``
#       If the gstgl-1.0 library is available and target GStreamer::GStreamer exists
#

include(CMakeFindDependencyMacro)
find_dependency(GObject)

find_package(PkgConfig QUIET)
function(find_gstreamer_component component prefix header library)
    if(NOT TARGET GStreamer::${component})
        string(TOUPPER ${component} upper)
        pkg_check_modules(PC_GSTREAMER_${upper} ${prefix} IMPORTED_TARGET)
        if(TARGET PkgConfig::PC_GSTREAMER_${upper})
            add_library(GStreamer::${component} INTERFACE IMPORTED)
            target_link_libraries(GStreamer::${component} INTERFACE PkgConfig::PC_GSTREAMER_${upper})
        else()
            find_path(GStreamer_${component}_INCLUDE_DIR
                NAMES ${header}
                PATH_SUFFIXES gstreamer-1.0
            )
            find_library(GStreamer_${component}_LIBRARY
                NAMES ${library}
            )
            if(${component} STREQUAL "Gl")
                # search the gstglconfig.h include dir under the same root where the library is found
                get_filename_component(gstglLibDir "${GStreamer_Gl_LIBRARY}" PATH)
                find_path(GStreamer_GlConfig_INCLUDE_DIR
                    NAMES gst/gl/gstglconfig.h
                    PATH_SUFFIXES gstreamer-1.0/include
                    HINTS ${PC_GSTREAMER_GL_INCLUDE_DIRS} ${PC_GSTREAMER_GL_INCLUDEDIR} "${gstglLibDir}"
                )
                if(GStreamer_GlConfig_INCLUDE_DIR)
                    list(APPEND GStreamer_Gl_INCLUDE_DIR "${GStreamer_GlConfig_INCLUDE_DIR}")
                    list(REMOVE_DUPLICATES GStreamer_Gl_INCLUDE_DIR)
                endif()
            endif()
            if(GStreamer_${component}_LIBRARY AND GStreamer_${component}_INCLUDE_DIR)
                add_library(GStreamer::${component} INTERFACE IMPORTED)
                target_include_directories(GStreamer::${component} INTERFACE ${GStreamer_${component}_INCLUDE_DIR})
                target_link_libraries(GStreamer::${component} INTERFACE ${GStreamer_${component}_LIBRARY})
            endif()
            mark_as_advanced(GStreamer_${component}_INCLUDE_DIR GStreamer_${component}_LIBRARY)
        endif()
    endif()

    if(TARGET GStreamer::${component})
        set(GStreamer_${component}_FOUND TRUE PARENT_SCOPE)
    endif()
endfunction()

# GStreamer required dependencies
find_gstreamer_component(Core gstreamer-1.0 gst/gst.h gstreamer-1.0)
find_gstreamer_component(Base gstreamer-base-1.0 gst/gst.h gstbase-1.0)
find_gstreamer_component(Audio gstreamer-audio-1.0 gst/audio/audio.h gstaudio-1.0)
find_gstreamer_component(Video gstreamer-video-1.0 gst/video/video.h gstvideo-1.0)
find_gstreamer_component(Pbutils gstreamer-pbutils-1.0 gst/pbutils/pbutils.h gstpbutils-1.0)
find_gstreamer_component(Allocators gstreamer-allocators-1.0 gst/allocators/allocators.h gstallocators-1.0)

if(TARGET GStreamer::Core)
    target_link_libraries(GStreamer::Core INTERFACE GObject::GObject)
endif()
if(TARGET GStreamer::Base AND TARGET GStreamer::Core)
    target_link_libraries(GStreamer::Base INTERFACE GStreamer::Core)
endif()
if(TARGET GStreamer::Audio AND TARGET GStreamer::Base)
    target_link_libraries(GStreamer::Audio INTERFACE GStreamer::Base)
endif()
if(TARGET GStreamer::Video AND TARGET GStreamer::Base)
    target_link_libraries(GStreamer::Video INTERFACE GStreamer::Base)
endif()
if(TARGET GStreamer::Pbutils AND TARGET GStreamer::Audio AND TARGET GStreamer::Video)
    target_link_libraries(GStreamer::Pbutils INTERFACE GStreamer::Audio GStreamer::Video)
endif()
if(TARGET GStreamer::Allocators AND TARGET GStreamer::Core)
    target_link_libraries(GStreamer::Allocators INTERFACE GStreamer::Core)
endif()

# GStreamer optional components
foreach(component ${GStreamer_FIND_COMPONENTS})
    if (${component} STREQUAL "App")
        find_gstreamer_component(App gstreamer-app-1.0 gst/app/gstappsink.h gstapp-1.0)
        if(TARGET GStreamer::App AND TARGET GStreamer::Base)
            target_link_libraries(GStreamer::App INTERFACE GStreamer::Base)
        endif()
    elseif (${component} STREQUAL "Photography")
        find_gstreamer_component(Photography gstreamer-photography-1.0 gst/interfaces/photography.h gstphotography-1.0)
        if(TARGET GStreamer::Photography AND TARGET GStreamer::Core)
            target_link_libraries(GStreamer::Photography INTERFACE GStreamer::Core)
        endif()
    elseif (${component} STREQUAL "Gl")
        find_gstreamer_component(Gl gstreamer-gl-1.0 gst/gl/gl.h gstgl-1.0)
        if(TARGET GStreamer::Gl AND TARGET GStreamer::Video AND TARGET GStreamer::Allocators)
            target_link_libraries(GStreamer::Gl INTERFACE GStreamer::Video GStreamer::Allocators)
        endif()
    else()
        message(WARNING "FindGStreamer.cmake: Invalid Gstreamer component \"${component}\" requested")
    endif()
endforeach()

# Create target GStreamer::GStreamer
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamer
                                REQUIRED_VARS
                                GStreamer_Core_FOUND
                                GStreamer_Base_FOUND
                                GStreamer_Audio_FOUND
                                GStreamer_Video_FOUND
                                GStreamer_Pbutils_FOUND
                                GStreamer_Allocators_FOUND
                                HANDLE_COMPONENTS
)

if(GStreamer_FOUND AND NOT TARGET GStreamer::GStreamer)
    add_library(GStreamer::GStreamer INTERFACE IMPORTED)
    target_link_libraries(GStreamer::GStreamer INTERFACE
                            GStreamer::Core
                            GStreamer::Base
                            GStreamer::Audio
                            GStreamer::Video
                            GStreamer::Pbutils
                            GStreamer::Allocators
    )
endif()

if(TARGET PkgConfig::PC_GSTREAMER_GL)
    get_target_property(_qt_incs PkgConfig::PC_GSTREAMER_GL INTERFACE_INCLUDE_DIRECTORIES)
    set(__qt_fixed_incs)
    foreach(path IN LISTS _qt_incs)
        if(IS_DIRECTORY "${path}")
            list(APPEND __qt_fixed_incs "${path}")
        endif()
    endforeach()
    set_property(TARGET PkgConfig::PC_GSTREAMER_GL PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${__qt_fixed_incs}")
endif()
