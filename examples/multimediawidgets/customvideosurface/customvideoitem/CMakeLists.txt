# Generated from customvideoitem.pro.

cmake_minimum_required(VERSION 3.14)
project(customvideoitem LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimediawidgets/customvideosurface/customvideoitem")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Multimedia)
find_package(Qt6 COMPONENTS MultimediaWidgets)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(customvideoitem
    main.cpp
    videoitem.cpp videoitem.h
    videoplayer.cpp videoplayer.h
)
set_target_properties(customvideoitem PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(customvideoitem PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::MultimediaWidgets
    Qt::Widgets
)

if(TARGET Qt::OpenGL)
    target_link_libraries(customvideoitem PUBLIC
        Qt::OpenGL
    )
endif()

install(TARGETS customvideoitem
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
