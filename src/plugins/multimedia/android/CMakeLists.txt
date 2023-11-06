# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_plugin(QAndroidMediaPlugin
    OUTPUT_NAME androidmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        common/qandroidaudiooutput_p.h
        common/qandroidaudioinput.cpp common/qandroidaudioinput_p.h
        audio/qandroidaudiodecoder.cpp audio/qandroidaudiodecoder_p.h
        common/qandroidglobal_p.h
        common/qandroidmultimediautils.cpp common/qandroidmultimediautils_p.h
        common/qandroidvideosink.cpp common/qandroidvideosink_p.h
        common/qandroidvideooutput.cpp common/qandroidvideooutput_p.h
        mediacapture/qandroidcamera.cpp mediacapture/qandroidcamera_p.h
        mediacapture/qandroidimagecapture.cpp mediacapture/qandroidimagecapture_p.h
        mediacapture/qandroidcamerasession.cpp mediacapture/qandroidcamerasession_p.h
        mediacapture/qandroidmediacapturesession.cpp mediacapture/qandroidmediacapturesession_p.h
        mediacapture/qandroidcapturesession.cpp mediacapture/qandroidcapturesession_p.h
        mediacapture/qandroidmediaencoder.cpp mediacapture/qandroidmediaencoder_p.h
        mediaplayer/qandroidmediaplayer.cpp mediaplayer/qandroidmediaplayer_p.h
        mediaplayer/qandroidmetadata.cpp mediaplayer/qandroidmetadata_p.h
        qandroidformatsinfo.cpp  qandroidformatsinfo_p.h
        qandroidintegration.cpp qandroidintegration_p.h
        wrappers/jni/androidcamera.cpp wrappers/jni/androidcamera_p.h
        wrappers/jni/androidmediametadataretriever.cpp wrappers/jni/androidmediametadataretriever_p.h
        wrappers/jni/androidmediaplayer.cpp wrappers/jni/androidmediaplayer_p.h
        wrappers/jni/androidmediarecorder.cpp wrappers/jni/androidmediarecorder_p.h
        wrappers/jni/androidmultimediautils.cpp wrappers/jni/androidmultimediautils_p.h
        wrappers/jni/androidsurfacetexture.cpp wrappers/jni/androidsurfacetexture_p.h
        wrappers/jni/androidsurfaceview.cpp wrappers/jni/androidsurfaceview_p.h
    NO_UNITY_BUILD_SOURCES
        # Resolves two problems:
        #   - Collision of `rwLock` with wrappers/jni/androidmediaplayer.cpp
        #   - and redefinition of `notifyFrameAvailable` with different signature
        #     with wrappers/jni/androidsurfacetexture.cpp
        wrappers/jni/androidcamera.cpp
    INCLUDE_DIRECTORIES
        audio
        common
        mediacapture
        mediaplayer
        wrappers/jni
        ../android
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        OpenSLES
        mediandk
)

set_property(TARGET QAndroidMediaPlugin APPEND PROPERTY QT_ANDROID_BUNDLED_JAR_DEPENDENCIES
    jar/Qt${QtMultimedia_VERSION_MAJOR}AndroidMultimedia.jar:org.qtproject.qt.android.multimedia.QtMultimediaUtils
)
set_property(TARGET QAndroidMediaPlugin APPEND PROPERTY QT_ANDROID_LIB_DEPENDENCIES
    ${INSTALL_PLUGINSDIR}/multimedia/libplugins_multimedia_androidmediaplugin.so
)
set_property(TARGET QAndroidMediaPlugin APPEND PROPERTY QT_ANDROID_PERMISSIONS
    android.permission.CAMERA android.permission.RECORD_AUDIO
    android.permission.BLUETOOTH
    android.permission.MODIFY_AUDIO_SETTINGS
)
