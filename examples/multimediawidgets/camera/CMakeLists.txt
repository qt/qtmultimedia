cmake_minimum_required(VERSION 3.16)
project(camera LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimediawidgets/camera")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia MultimediaWidgets Widgets)

set(camera_form "")
set(videosettings_form "")
if(ANDROID OR IOS)
    set(camera_form camera_mobile.ui)
    set(videosettings_form videosettings_mobile.ui)
else()
    set(camera_form camera.ui)
    set(videosettings_form videosettings.ui)
endif()

qt_add_executable(camera
    MANUAL_FINALIZATION
    camera.cpp camera.h ${camera_form}
    imagesettings.cpp imagesettings.h imagesettings.ui
    main.cpp
    videosettings.cpp videosettings.h ${videosettings_form}
    metadatadialog.cpp metadatadialog.h
)

set_target_properties(camera PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(APPLE AND NOT IOS)
    set_target_properties(camera PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
    )
elseif(IOS)
    set_target_properties(camera PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in"
    )
endif()

set_property(TARGET camera APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
             ${CMAKE_CURRENT_SOURCE_DIR}/android)

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

qt_finalize_executable(camera)

install(TARGETS camera
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
