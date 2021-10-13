# Generated from video.pro.

add_subdirectory(qmlvideo)
if(TARGET Qt::QuickControls2)
    if(TARGET Qt::Svg)
        add_subdirectory(mediaplayer)
    endif()
    add_subdirectory(recorder)
endif()
