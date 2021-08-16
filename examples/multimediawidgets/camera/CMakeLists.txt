# Generated from camera.pro.

cmake_minimum_required(VERSION 3.16)
project(camera LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimediawidgets/camera")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Multimedia)
find_package(Qt6 COMPONENTS MultimediaWidgets)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(camera
    camera.cpp camera.h camera.ui
    imagesettings.cpp imagesettings.h imagesettings.ui
    main.cpp
    videosettings.cpp videosettings.h videosettings.ui
    metadatadialog.cpp metadatadialog.h
)
set_target_properties(camera PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)
target_include_directories(camera PUBLIC
    ../../multimedia/shared
)
target_link_libraries(camera PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::MultimediaWidgets
    Qt::Widgets
)


# Resources:
set(camera_resource_files
    "images/shutter.svg"
)

qt6_add_resources(camera "camera"
    PREFIX
        "/"
    FILES
        ${camera_resource_files}
)

install(TARGETS camera
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
