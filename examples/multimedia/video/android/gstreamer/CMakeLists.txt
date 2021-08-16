# Generated from gstreamer.pro.

cmake_minimum_required(VERSION 3.16)
project(gstreamer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/android/gstreamer")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Multimedia)
find_package(Qt6 COMPONENTS MultimediaWidgets)

qt_add_executable(gstreamer
    main.cpp
)
set_target_properties(gstreamer PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(gstreamer PUBLIC
    $(GSTREAMER_ROOT_ANDROID)/armv7/include
    $(GSTREAMER_ROOT_ANDROID)/armv7/include/glib-2.0
    $(GSTREAMER_ROOT_ANDROID)/armv7/include/gstreamer-1.0
    $(GSTREAMER_ROOT_ANDROID)/armv7/lib/glib-2.0/include
)

target_link_libraries(gstreamer PUBLIC
    # Remove: L$(GSTREAMER_ROOT_ANDROID)/armv7/lib/
    # Remove: L$(GSTREAMER_ROOT_ANDROID)/armv7/lib/gstreamer-1.0
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::MultimediaWidgetsPrivate
    Qt::Quick
    avcodec
    avfilter
    avformat
    avutil
    bz2
    gio-2.0
    gstapp
    gstaudioconvert
    gstaudioparsers
    gstautodetect
    gstavi
    gstcoreelements
    gstgio
    gstisomp4
    gstivorbisdec
    gstlibav
    gstmpg123
    gstnet-1.0
    gstogg
    gstplayback
    gstriff-1.0
    gstrtp-1.0
    gstsoup
    gstsubparse
    gsttcp
    gsttheora
    gsttypefindfunctions
    gstvideoconvert
    gstvideotestsrc
    gstvorbis
    gstwavenc
    gstwavpack
    gstwavparse
    gstx264
    mpg123
    ogg
    soup-2.4
    swresample
    theora
    vorbis
    vorbisenc
    vorbisfile
    x264
)


# Resources:
set(qml_resource_files
    "main.qml"
)

qt6_add_resources(gstreamer "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

install(TARGETS gstreamer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
