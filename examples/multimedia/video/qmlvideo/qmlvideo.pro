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
    images/folder.png \
    images/leaves.jpg \
    images/up.png \
    qml/frequencymonitor/FrequencyItem.qml \
    qml/performancemonitor/PerformanceItem.qml \
    qml/qmlvideo/Button.qml \
    qml/qmlvideo/CameraBasic.qml \
    qml/qmlvideo/CameraDrag.qml \
    qml/qmlvideo/CameraDummy.qml \
    qml/qmlvideo/CameraFullScreen.qml \
    qml/qmlvideo/CameraFullScreenInverted.qml \
    qml/qmlvideo/CameraItem.qml \
    qml/qmlvideo/CameraMove.qml \
    qml/qmlvideo/CameraOverlay.qml \
    qml/qmlvideo/CameraResize.qml \
    qml/qmlvideo/CameraRotate.qml \
    qml/qmlvideo/CameraSpin.qml \
    qml/qmlvideo/Content.qml \
    qml/qmlvideo/ErrorDialog.qml \
    qml/qmlvideo/Scene.qml \
    qml/qmlvideo/SceneBasic.qml \
    qml/qmlvideo/SceneDrag.qml \
    qml/qmlvideo/SceneFullScreen.qml \
    qml/qmlvideo/SceneFullScreenInverted.qml \
    qml/qmlvideo/SceneMove.qml \
    qml/qmlvideo/SceneMulti.qml \
    qml/qmlvideo/SceneOverlay.qml \
    qml/qmlvideo/SceneResize.qml \
    qml/qmlvideo/SceneRotate.qml \
    qml/qmlvideo/SceneSelectionPanel.qml \
    qml/qmlvideo/SceneSpin.qml \
    qml/qmlvideo/SeekControl.qml \
    qml/qmlvideo/VideoBasic.qml \
    qml/qmlvideo/VideoDrag.qml \
    qml/qmlvideo/VideoDummy.qml \
    qml/qmlvideo/VideoFillMode.qml \
    qml/qmlvideo/VideoFullScreen.qml \
    qml/qmlvideo/VideoFullScreenInverted.qml \
    qml/qmlvideo/VideoItem.qml \
    qml/qmlvideo/VideoMetadata.qml \
    qml/qmlvideo/VideoMove.qml \
    qml/qmlvideo/VideoOverlay.qml \
    qml/qmlvideo/VideoPlaybackRate.qml \
    qml/qmlvideo/VideoResize.qml \
    qml/qmlvideo/VideoRotate.qml \
    qml/qmlvideo/VideoSeek.qml \
    qml/qmlvideo/VideoSpin.qml \
    qml/qmlvideo/main.qml

resources.prefix = /

RESOURCES += resources

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideo
INSTALLS += target

EXAMPLE_FILES += \
    qmlvideo.png \
    qmlvideo.svg

