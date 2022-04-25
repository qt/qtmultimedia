cmake_minimum_required(VERSION 3.16)
project(audiodecoder LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/spatialaudio")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Widgets)

qt_add_executable(spatialaudio
    main.cpp
)

set_target_properties(spatialaudio PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(spatialaudio PUBLIC
    Qt::Core
    Qt::Multimedia
    Qt::Widgets
)

install(TARGETS spatialaudio
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
