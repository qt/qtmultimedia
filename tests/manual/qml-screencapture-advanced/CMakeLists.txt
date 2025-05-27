cmake_minimum_required(VERSION 3.16)

project(qml-screencapture-advanced VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/qml-screencapture-advanced")

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Multimedia Qml Quick)
endif()

qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(qml-screencapture-advanced
    WIN32
    MACOSX_BUNDLE
    main.cpp)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(qml-screencapture-advanced
    URI QmlScreenCaptureAdvanced
    QML_FILES
        Main.qml
        WindowCapturePanel.qml
        ScreenCapturePanel.qml
)

qt_add_ios_ffmpeg_libraries(qml-screencapture-advanced)

target_link_libraries(qml-screencapture-advanced PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Qml
    Qt::Quick)

set_target_properties(qml-screencapture-advanced PROPERTIES
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android)

install(TARGETS qml-screencapture-advanced
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
