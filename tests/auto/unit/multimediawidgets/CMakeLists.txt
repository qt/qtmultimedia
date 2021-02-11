# Generated from multimediawidgets.pro.

add_subdirectory(qcamerawidgets)
add_subdirectory(qmediaplayerwidgets)
if(QT_FEATURE_private_tests)
    add_subdirectory(qgraphicsvideoitem)
    add_subdirectory(qpaintervideosurface)
#    add_subdirectory(qvideowidget)
endif()
