# Generated from audiooutput.pro.

cmake_minimum_required(VERSION 3.16)
project(audiooutput LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/audiooutput")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Multimedia)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(audiooutput
    audiooutput.cpp audiooutput.h
    main.cpp
)
set_target_properties(audiooutput PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(audiooutput PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Widgets
)

install(TARGETS audiooutput
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
