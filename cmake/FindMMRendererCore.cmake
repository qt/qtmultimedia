find_library(MMRendererCore_LIBRARY NAMES mmrndcore)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MMRendererCore DEFAULT_MSG MMRendererCore_LIBRARY)
if(MMRendererCore_FOUND AND NOT TARGET MMRendererCore::MMRendererCore)
    add_library(MMRendererCore::MMRendererCore INTERFACE IMPORTED)
    target_link_libraries(MMRendererCore::MMRendererCore
                        INTERFACE "${MMRendererCore_LIBRARY}")
endif()
mark_as_advanced(MMRendererCore_LIBRARY)
