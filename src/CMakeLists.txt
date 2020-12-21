# Generated from src.pro.

add_subdirectory(multimedia)

# special case begin
if(TARGET Qt::Quick)
    add_subdirectory(qtmultimediaquicktools)
    add_subdirectory(imports)
endif()

if(TARGET Qt::Widgets)
    add_subdirectory(multimediawidgets)
endif()

if(QT_FEATURE_gstreamer)
    add_subdirectory(gsttools)
endif()

add_subdirectory(plugins)
#special case end
