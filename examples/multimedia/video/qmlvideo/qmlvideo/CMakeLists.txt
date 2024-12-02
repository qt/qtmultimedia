# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "This module is part of the 'qmlvideo' example, and should not be built independently.")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/qmlvideo/qmlvideo")

qt_add_qml_module(qmlvideo
    URI qmlvideo
    VERSION 1.0
    SOURCES
        videosingleton.cpp
        videosingleton.h
        qmlvideo_global.h
    QML_FILES
        "CameraBasic.qml"
        "CameraDrag.qml"
        "CameraDummy.qml"
        "CameraFullScreen.qml"
        "CameraFullScreenInverted.qml"
        "CameraItem.qml"
        "CameraMove.qml"
        "CameraOverlay.qml"
        "CameraResize.qml"
        "CameraRotate.qml"
        "CameraSpin.qml"
        "Content.qml"
        "ErrorDialog.qml"
        "Main.qml"
        "Scene.qml"
        "SceneBasic.qml"
        "SceneDrag.qml"
        "SceneFullScreen.qml"
        "SceneFullScreenInverted.qml"
        "SceneMove.qml"
        "SceneMulti.qml"
        "SceneOverlay.qml"
        "SceneResize.qml"
        "SceneRotate.qml"
        "SceneSelectionPanel.qml"
        "SceneSpin.qml"
        "SeekControl.qml"
        "VideoBasic.qml"
        "VideoDrag.qml"
        "VideoDummy.qml"
        "VideoFillMode.qml"
        "VideoFullScreen.qml"
        "VideoFullScreenInverted.qml"
        "VideoItem.qml"
        "VideoMetadata.qml"
        "VideoMove.qml"
        "VideoOverlay.qml"
        "VideoPlaybackRate.qml"
        "VideoResize.qml"
        "VideoRotate.qml"
        "VideoSeek.qml"
        "VideoSpin.qml"
    RESOURCES
        "images/folder.png"
        "images/leaves.jpg"
        "images/up.png"
)

target_compile_definitions(qmlvideo PRIVATE QMLVIDEO_LIB)

target_link_libraries(qmlvideo PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

install(TARGETS qmlvideo
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${INSTALL_EXAMPLEDIR}"
)
