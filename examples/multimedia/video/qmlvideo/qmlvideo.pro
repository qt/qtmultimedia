TEMPLATE = app
TARGET = qmlvideo

QT += quick multimedia
CONFIG += add_ios_ffmpeg_libraries
android: qtHaveModule(androidextras) {
    QT += androidextras
    DEFINES += REQUEST_PERMISSIONS_ON_ANDROID
}

SOURCES += \
    frequencymonitor/frequencymonitor.cpp \
    main.cpp \
    qmlvideo/videosingleton.cpp

INCLUDEPATH += qmlvideo
INCLUDEPATH += frequencymonitor

DEFINES += QMLVIDEO_LIB

HEADERS += \
    frequencymonitor/frequencymonitor.h \
    trace.h \
    qmlvideo/videosingleton.h

resources.files = \
    frequencymonitor/FrequencyItem.qml \
    frequencymonitor/qmldir \
    frequencymonitor/PerformanceItem.qml \
    qmlvideo/CameraBasic.qml \
    qmlvideo/CameraDrag.qml \
    qmlvideo/CameraDummy.qml \
    qmlvideo/CameraFullScreen.qml \
    qmlvideo/CameraFullScreenInverted.qml \
    qmlvideo/CameraItem.qml \
    qmlvideo/CameraMove.qml \
    qmlvideo/CameraOverlay.qml \
    qmlvideo/CameraResize.qml \
    qmlvideo/CameraRotate.qml \
    qmlvideo/CameraSpin.qml \
    qmlvideo/Content.qml \
    qmlvideo/ErrorDialog.qml \
    qmlvideo/Main.qml \
    qmlvideo/Scene.qml \
    qmlvideo/SceneBasic.qml \
    qmlvideo/SceneDrag.qml \
    qmlvideo/SceneFullScreen.qml \
    qmlvideo/SceneFullScreenInverted.qml \
    qmlvideo/SceneMove.qml \
    qmlvideo/SceneMulti.qml \
    qmlvideo/SceneOverlay.qml \
    qmlvideo/SceneResize.qml \
    qmlvideo/SceneRotate.qml \
    qmlvideo/SceneSelectionPanel.qml \
    qmlvideo/SceneSpin.qml \
    qmlvideo/SeekControl.qml \
    qmlvideo/VideoBasic.qml \
    qmlvideo/VideoDrag.qml \
    qmlvideo/VideoDummy.qml \
    qmlvideo/VideoFillMode.qml \
    qmlvideo/VideoFullScreen.qml \
    qmlvideo/VideoFullScreenInverted.qml \
    qmlvideo/VideoItem.qml \
    qmlvideo/VideoMetadata.qml \
    qmlvideo/VideoMove.qml \
    qmlvideo/VideoOverlay.qml \
    qmlvideo/VideoPlaybackRate.qml \
    qmlvideo/VideoResize.qml \
    qmlvideo/VideoRotate.qml \
    qmlvideo/VideoSeek.qml \
    qmlvideo/VideoSpin.qml \
    qmlvideo/images/folder.png \
    qmlvideo/images/leaves.jpg \
    qmlvideo/images/up.png \
    qmlvideo/qmldir

resources.prefix = /qt/qml/

RESOURCES += resources

CONFIG += qmltypes
QML_IMPORT_NAME = qmlvideo
# QML_IMPORT_NAME = frequencymonitor
QML_IMPORT_MAJOR_VERSION = 1

QML_IMPORT_PATH = $$PWD/frequencymonitor

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideo
INSTALLS += target

EXAMPLE_FILES += \
    qmlvideo.png \
    qmlvideo.svg

