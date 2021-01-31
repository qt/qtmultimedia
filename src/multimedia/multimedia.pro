TARGET = QtMultimedia
QT = core-private network gui-private

MODULE_PLUGIN_TYPES = \
    video/gstvideorenderer \
    video/videonode \

QMAKE_DOCS = $$PWD/doc/qtmultimedia.qdocconf

INCLUDEPATH *= .

PRIVATE_HEADERS += \
    qtmultimediaglobal_p.h \
    qmediasource_p.h \
    qmediapluginloader_p.h \
    qmediaservice_p.h \
    qmediastoragelocation_p.h \
    qmultimediautils_p.h \

PUBLIC_HEADERS += \
    qtmultimediaglobal.h \
    qmediadevicemanager.h \
    qmediasink.h \
    qmediaenumdebug.h \
    qmediametadata.h \
    qmediasource.h \
    qmediaservice.h \
    qmediatimerange.h \
    qmultimedia.h \
    qmediaformat.h

SOURCES += \
    qmediasink.cpp \
    qmediadevicemanager.cpp \
    qmediametadata.cpp \
    qmediapluginloader.cpp \
    qmediaservice.cpp \
    qmediasource.cpp \
    qmediatimerange.cpp \
    qmediastoragelocation.cpp \
    qmultimedia.cpp \
    qmultimediautils.cpp \
    qmediaformat.cpp

CONFIG += simd optimize_full

include(audio/audio.pri)
include(camera/camera.pri)
include(controls/controls.pri)
include(playback/playback.pri)
include(recording/recording.pri)
include(video/video.pri)
include(platform/platform.pri)

ANDROID_BUNDLED_JAR_DEPENDENCIES = \
    jar/Qt$${QT_MAJOR_VERSION}AndroidMultimedia.jar:org.qtproject.qt.android.multimedia.QtMultimediaUtils
ANDROID_LIB_DEPENDENCIES = \
    plugins/mediaservice/libplugins_mediaservice_qtmedia_android.so \
    lib/libQt5MultimediaQuick.so:Qt5Quick
ANDROID_BUNDLED_FILES += \
    lib/libQt5MultimediaQuick.so
ANDROID_PERMISSIONS += \
    android.permission.CAMERA \
    android.permission.RECORD_AUDIO
ANDROID_FEATURES += \
    android.hardware.camera \
    android.hardware.camera.autofocus \
    android.hardware.microphone

MODULE_WINRT_CAPABILITIES_DEVICE += \
    microphone \
    webcam

win32: LIBS_PRIVATE += -luuid

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

load(qt_module)
