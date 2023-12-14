TEMPLATE = app
TARGET = qmlvideo

QT += quick multimedia
android: qtHaveModule(androidextras) {
    QT += androidextras
    DEFINES += REQUEST_PERMISSIONS_ON_ANDROID
}

DEFINES += \
    FREQUENCYMONITOR_SUPPORT \
    PERFORMANCEMONITOR_SUPPORT

SOURCES += \
    main.cpp \
    frequencymonitor.cpp \
    frequencymonitordeclarative.cpp \
    performancemonitor.cpp \
    performancemonitordeclarative.cpp

HEADERS += \
    trace.h \
    frequencymonitor.h \
    performancemonitor.h \
    performancemonitordeclarative.h

resources.files = \
    qmlvideo/images/folder.png \
    qmlvideo/images/leaves.jpg \
    qmlvideo/images/up.png \
    frequencymonitor/FrequencyItem.qml \
    frequencymonitor/qmldir \
    performancemonitor/PerformanceItem.qml \
    performancemonitor/qmldir \
    qmlvideo/Button.qml \
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
    qmlvideo/Main.qml \
    qmlvideo/qmldir

resources.prefix = /qt/qml/

RESOURCES += resources

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideo
INSTALLS += target

EXAMPLE_FILES += \
    qmlvideo.png \
    qmlvideo.svg

