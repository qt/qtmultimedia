# Generated from src.pro.

add_subdirectory(multimedia)
if(ANDROID)
    add_subdirectory(android)
endif()

# special case begin
if(TARGET Qt::Quick)
    add_subdirectory(multimediaquick)
    add_subdirectory(imports)
endif()

if(TARGET Qt::Widgets)
    add_subdirectory(multimediawidgets)
endif()

add_subdirectory(plugins)
#special case end
