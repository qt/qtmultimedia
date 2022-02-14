cmake_minimum_required(VERSION 3.16)
project(audiosource LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/audiosource")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Widgets)

qt_add_executable(audiosource
    audiosource.cpp audiosource.h
    main.cpp
)

set_target_properties(audiosource PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(audiosource PUBLIC
    ../shared
)

target_link_libraries(audiosource PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Widgets
)

install(TARGETS audiosource
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
