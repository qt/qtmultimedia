# Generated from audiodecoder.pro.

cmake_minimum_required(VERSION 3.16)
project(audiodecoder LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/audiodecoder")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Multimedia)
if(ANDROID)
    find_package(Qt6 COMPONENTS Widgets)
endif()

qt_add_executable(audiodecoder
    audiodecoder.cpp audiodecoder.h
    main.cpp
)
set_target_properties(audiodecoder PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(audiodecoder PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
)

if(ANDROID)
    target_link_libraries(audiodecoder PUBLIC Qt::Widgets)
endif()

install(TARGETS audiodecoder
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
