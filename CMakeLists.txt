# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qtmultimedia.pro.

cmake_minimum_required(VERSION 3.16)

include(.cmake.conf)
project(QtMultimedia
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Multimedia Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals)

# This should be called as early as possible, just after find_package(BuildInternals) where it is
# defined.
qt_internal_project_setup()

find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED Core)
find_package(Qt6 ${PROJECT_VERSION} QUIET CONFIG
    OPTIONAL_COMPONENTS
        Gui
        Network
        Svg
        Widgets
        Quick
        Qml
        Test
        QuickTest
        QuickControls2
        Quick3D
        OpenGLPrivate
        Concurrent)

if(NOT TARGET Qt::Gui)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Gui\" is not met.")
    return()
endif()

if(QNX AND CMAKE_SYSTEM_VERSION GREATER_EQUAL 800)
    message(NOTICE "Skipping the build as multimedia is not supported on QNX8.0.")
    return()
endif()

if(NOT QT_FEATURE_thread)
    message(NOTICE "Skipping the build of QtMultimedia as it is not supported without threads")
    return()
endif()

include(src/multimedia/Qt6MultimediaMacros.cmake)

set(QT_BUILD_EXTRA_IDE_FILE_PATTERNS
    src/multimedia/doc/QtMultimediaDoc
    src/multimedia/doc/qtmultimedia.qdocconf
    src/multimedia/doc/snippets/CMakeLists.txt
    src/multimedia/doc/snippets/multimedia-snippets/audio.cpp
    src/multimedia/doc/snippets/multimedia-snippets/camera.cpp
    src/multimedia/doc/snippets/multimedia-snippets/devices.cpp
    src/multimedia/doc/snippets/multimedia-snippets/images/qt-logo.png
    src/multimedia/doc/snippets/multimedia-snippets/media.cpp
    src/multimedia/doc/snippets/multimedia-snippets/multiple-videooutputs.qml
    src/multimedia/doc/snippets/multimedia-snippets/qsound.cpp
    src/multimedia/doc/snippets/multimedia-snippets/qtvideosink.qml
    src/multimedia/doc/snippets/multimedia-snippets/soundeffect.qml
    src/multimedia/doc/snippets/multimedia-snippets/video.cpp
    src/multimedia/doc/src/audiooverview.qdoc
    src/multimedia/doc/src/backend-notes-apple.qdoc
    src/multimedia/doc/src/cameraoverview.qdoc
    src/multimedia/doc/src/classic.css
    src/multimedia/doc/src/multimedia-overview.qdoc
    src/multimedia/doc/src/platform-notes-apple.qdoc
    src/multimedia/doc/src/platform-notes-wasm.qdoc
    src/multimedia/doc/src/platform-notes-wayland.qdoc
    src/multimedia/doc/src/qm-external-pages.qdoc
    src/multimedia/doc/src/qt6-changes.qdoc
    src/multimedia/doc/src/qtmultimedia-building-from-source.qdoc
    src/multimedia/doc/src/qtmultimedia-cpp.qdoc
    src/multimedia/doc/src/qtmultimedia-examples.qdoc
    src/multimedia/doc/src/qtmultimedia-index.qdoc
    src/multimedia/doc/src/qtmultimedia-qml-types.qdoc
    src/multimedia/doc/src/videooverview.qdoc
    src/multimedia/doc/src/advanced-ffmpeg-configuration.qdoc
    src/multimedia/doc/src/images/Zoom.gif
    src/multimedia/doc/src/images/annotatedurl.png
    src/multimedia/doc/src/images/camera_correctionAngle_90.png
    src/multimedia/doc/src/images/codeless.png
    src/multimedia/doc/src/images/how-focus-works.gif
    src/multimedia/doc/src/images/image_processing.png
    src/multimedia/doc/src/images/noun_Media_166644.svg
    src/multimedia/doc/src/images/qS1FmgPVL.jpg
    src/multimedia/doc/src/images/qmlcamera-menu.png
    src/multimedia/doc/src/images/radio-example.png
    src/multimedia/doc/src/images/slideshow-img1.png
    src/multimedia/doc/src/images/sound-wave-small.jpg
    src/multimedia/doc/src/images/video-graphics-memory.png

)

qt_build_repo()
