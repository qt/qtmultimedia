# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from jar.pro.

set(java_sources
    src/org/qtproject/qt/android/multimedia/QtAndroidMediaPlayer.java
    src/org/qtproject/qt/android/multimedia/QtCameraListener.java
    src/org/qtproject/qt/android/multimedia/QtCamera2.java
    src/org/qtproject/qt/android/multimedia/QtExifDataHandler.java
    src/org/qtproject/qt/android/multimedia/QtAudioDeviceManager.java
    src/org/qtproject/qt/android/multimedia/QtVideoDeviceManager.java
    src/org/qtproject/qt/android/multimedia/QtMediaRecorderListener.java
    src/org/qtproject/qt/android/multimedia/QtMultimediaUtils.java
    src/org/qtproject/qt/android/multimedia/QtSurfaceHolderCallback.java
    src/org/qtproject/qt/android/multimedia/QtSurfaceTextureHolder.java
    src/org/qtproject/qt/android/multimedia/QtSurfaceTextureListener.java
)

qt_internal_add_jar(Qt${QtMultimedia_VERSION_MAJOR}AndroidMultimedia
    INCLUDE_JARS ${QT_ANDROID_JAR}
    SOURCES ${java_sources}
    OUTPUT_DIR "${QT_BUILD_DIR}/jar"
)

qt_path_join(destination ${INSTALL_DATADIR} "jar")

install_jar(Qt${QtMultimedia_VERSION_MAJOR}AndroidMultimedia
    DESTINATION ${destination}
    COMPONENT Devel
)

