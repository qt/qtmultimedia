# Generated from multimedia.pro.

add_subdirectory(audiodecoder)
if(TARGET Qt::Widgets)
    add_subdirectory(spectrum)
    add_subdirectory(audiorecorder)
    add_subdirectory(audiodevices)
    add_subdirectory(audioinput)
    add_subdirectory(audiooutput)
endif()
if(TARGET Qt::Quick)
    add_subdirectory(declarative-camera)
    add_subdirectory(video)
endif()
