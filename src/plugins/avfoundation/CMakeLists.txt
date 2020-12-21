# Generated from avfoundation.pro.

add_subdirectory(mediaplayer)
if(NOT TVOS)
    add_subdirectory(camera)
endif()
