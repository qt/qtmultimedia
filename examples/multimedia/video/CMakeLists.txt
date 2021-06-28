# Generated from video.pro.

add_subdirectory(qmlvideo)
add_subdirectory(qmlvideofx)
if(TARGET Qt::QuickControls2)
    add_subdirectory(mediaplayer)
    add_subdirectory(recorder)
endif()
if(ANDROID)
    add_subdirectory(android)
endif()
