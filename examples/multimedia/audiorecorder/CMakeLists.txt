# Generated from audiorecorder.pro.

cmake_minimum_required(VERSION 3.16)
project(audiorecorder LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/audiorecorder")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Multimedia)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(audiorecorder
    audiolevel.cpp audiolevel.h
    audiorecorder.cpp audiorecorder.h audiorecorder.ui
    main.cpp
)
set_target_properties(audiorecorder PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
        "${CMAKE_CURRENT_LIST_DIR}/audiorecorder.entitlements"
)
# special case begin
target_include_directories(audiorecorder PUBLIC
    ../shared
)
# special case end
target_link_libraries(audiorecorder PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Widgets
)

if(WIN32)
    target_include_directories(audiorecorder PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

install(TARGETS audiorecorder
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

if(APPLE AND NOT IOS)
    if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
        add_custom_command(TARGET audiorecorder
        POST_BUILD
        COMMAND codesign --sign - --entitlements ${CMAKE_CURRENT_SOURCE_DIR}/audiorecorder.entitlements ${CMAKE_CURRENT_BINARY_DIR}/audiorecorder.app
    )
    endif()
endif()
