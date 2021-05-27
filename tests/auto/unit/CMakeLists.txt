# Generated from unit.pro.

add_subdirectory(mockbackend)
add_subdirectory(multimedia)
if(TARGET Qt::Widgets)
    add_subdirectory(multimediawidgets)
endif()
