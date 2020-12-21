# Generated from qmlvideo.pro.

cmake_minimum_required(VERSION 3.14)
project(qmlvideo LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/qmlvideo")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Multimedia)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(qmlvideo
    ../snippets/frequencymonitor/frequencymonitor.cpp ../snippets/frequencymonitor/frequencymonitor.h
    ../snippets/frequencymonitor/frequencymonitordeclarative.cpp
    ../snippets/performancemonitor/performancemonitor.cpp ../snippets/performancemonitor/performancemonitor.h
    ../snippets/performancemonitor/performancemonitordeclarative.cpp ../snippets/performancemonitor/performancemonitordeclarative.h
    main.cpp
    trace.h
)
set_target_properties(qmlvideo PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(qmlvideo PUBLIC
    ../snippets/frequencymonitor
    ../snippets/performancemonitor
)

target_compile_definitions(qmlvideo PUBLIC
    FREQUENCYMONITOR_SUPPORT
    PERFORMANCEMONITOR_SUPPORT
)

target_link_libraries(qmlvideo PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Qml
    Qt::Quick
)


# Resources:
set_source_files_properties("images/folder.png"
    PROPERTIES QT_RESOURCE_ALIAS "folder.png"
)
set_source_files_properties("images/leaves.jpg"
    PROPERTIES QT_RESOURCE_ALIAS "leaves.jpg"
)
set_source_files_properties("images/up.png"
    PROPERTIES QT_RESOURCE_ALIAS "up.png"
)
set(qmlvideo_resource_files
    "images/folder.png"
    "images/leaves.jpg"
    "images/up.png"
    "qml/qmlvideo/Button.qml"
    "qml/qmlvideo/CameraBasic.qml"
    "qml/qmlvideo/CameraDrag.qml"
    "qml/qmlvideo/CameraDummy.qml"
    "qml/qmlvideo/CameraFullScreen.qml"
    "qml/qmlvideo/CameraFullScreenInverted.qml"
    "qml/qmlvideo/CameraItem.qml"
    "qml/qmlvideo/CameraMove.qml"
    "qml/qmlvideo/CameraOverlay.qml"
    "qml/qmlvideo/CameraResize.qml"
    "qml/qmlvideo/CameraRotate.qml"
    "qml/qmlvideo/CameraSpin.qml"
    "qml/qmlvideo/Content.qml"
    "qml/qmlvideo/ErrorDialog.qml"
    "qml/qmlvideo/FileBrowser.qml"
    "qml/qmlvideo/Scene.qml"
    "qml/qmlvideo/SceneBasic.qml"
    "qml/qmlvideo/SceneDrag.qml"
    "qml/qmlvideo/SceneFullScreen.qml"
    "qml/qmlvideo/SceneFullScreenInverted.qml"
    "qml/qmlvideo/SceneMove.qml"
    "qml/qmlvideo/SceneMulti.qml"
    "qml/qmlvideo/SceneOverlay.qml"
    "qml/qmlvideo/SceneResize.qml"
    "qml/qmlvideo/SceneRotate.qml"
    "qml/qmlvideo/SceneSelectionPanel.qml"
    "qml/qmlvideo/SceneSpin.qml"
    "qml/qmlvideo/SeekControl.qml"
    "qml/qmlvideo/VideoBasic.qml"
    "qml/qmlvideo/VideoDrag.qml"
    "qml/qmlvideo/VideoDummy.qml"
    "qml/qmlvideo/VideoFillMode.qml"
    "qml/qmlvideo/VideoFullScreen.qml"
    "qml/qmlvideo/VideoFullScreenInverted.qml"
    "qml/qmlvideo/VideoItem.qml"
    "qml/qmlvideo/VideoMetadata.qml"
    "qml/qmlvideo/VideoMove.qml"
    "qml/qmlvideo/VideoOverlay.qml"
    "qml/qmlvideo/VideoPlaybackRate.qml"
    "qml/qmlvideo/VideoResize.qml"
    "qml/qmlvideo/VideoRotate.qml"
    "qml/qmlvideo/VideoSeek.qml"
    "qml/qmlvideo/VideoSpin.qml"
    "qml/qmlvideo/main.qml"
)

qt6_add_resources(qmlvideo "qmlvideo"
    PREFIX
        "/"
    FILES
        ${qmlvideo_resource_files}
)
set_source_files_properties("../snippets/frequencymonitor/qml/frequencymonitor/FrequencyItem.qml"
    PROPERTIES QT_RESOURCE_ALIAS "qml/frequencymonitor/FrequencyItem.qml"
)
set(frequencymonitordeclarative_resource_files
    "qml/frequencymonitor/FrequencyItem.qml"
)

qt6_add_resources(qmlvideo "frequencymonitordeclarative"
    PREFIX
        "/"
    BASE
        "../snippets/frequencymonitor"
    FILES
        ${frequencymonitordeclarative_resource_files}
)
set_source_files_properties("../snippets/performancemonitor/qml/performancemonitor/PerformanceItem.qml"
    PROPERTIES QT_RESOURCE_ALIAS "qml/performancemonitor/PerformanceItem.qml"
)
set(performancemonitordeclarative_resource_files
    "qml/performancemonitor/PerformanceItem.qml"
)

qt6_add_resources(qmlvideo "performancemonitordeclarative"
    PREFIX
        "/"
    BASE
        "../snippets/performancemonitor"
    FILES
        ${performancemonitordeclarative_resource_files}
)

if(ANDROID AND TARGET Qt::AndroidExtras)
    target_compile_definitions(qmlvideo PUBLIC
        REQUEST_PERMISSIONS_ON_ANDROID
    )

    target_link_libraries(qmlvideo PUBLIC
        Qt::AndroidExtras
    )
endif()

install(TARGETS qmlvideo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
