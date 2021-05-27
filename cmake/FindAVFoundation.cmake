find_library(AVFoundation_LIBRARY NAMES AVFoundation)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AVFoundation DEFAULT_MSG AVFoundation_LIBRARY)

if(AVFoundation_FOUND AND NOT TARGET AVFoundation::AVFoundation)
    add_library(AVFoundation::AVFoundation INTERFACE IMPORTED)
    set_target_properties(AVFoundation::AVFoundation PROPERTIES
                          INTERFACE_LINK_LIBRARIES "${AVFoundation_LIBRARY}")
endif()

mark_as_advanced(AVFoundation_LIBRARY)
