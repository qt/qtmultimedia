TEMPLATE = app
TARGET = qmlvideofx

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
    filereader.cpp \
    frequencymonitor.cpp \
    frequencymonitordeclarative.cpp \
    performancemonitor.cpp \
    performancemonitordeclarative.cpp

HEADERS += \
    filereader.h \
    frequencymonitor.h \
    performancemonitor.h \
    performancemonitordeclarative.h \
    trace.h

resources.files = \
    images/Dropdown_arrows.png \
    images/Slider_bar.png \
    images/Slider_handle.png \
    images/Triangle_Top.png \
    images/Triangle_bottom.png \
    images/icon_BackArrow.png \
    images/icon_Folder.png \
    images/icon_Menu.png \
    images/qt-logo.png \
    qml/frequencymonitor/FrequencyItem.qml \
    qml/performancemonitor/PerformanceItem.qml \
    qml/qmlvideofx/Button.qml \
    qml/qmlvideofx/Content.qml \
    qml/qmlvideofx/ContentCamera.qml \
    qml/qmlvideofx/ContentImage.qml \
    qml/qmlvideofx/ContentVideo.qml \
    qml/qmlvideofx/Curtain.qml \
    qml/qmlvideofx/Divider.qml \
    qml/qmlvideofx/Effect.qml \
    qml/qmlvideofx/EffectBillboard.qml \
    qml/qmlvideofx/EffectBlackAndWhite.qml \
    qml/qmlvideofx/EffectEmboss.qml \
    qml/qmlvideofx/EffectGaussianBlur.qml \
    qml/qmlvideofx/EffectGlow.qml \
    qml/qmlvideofx/EffectIsolate.qml \
    qml/qmlvideofx/EffectMagnify.qml \
    qml/qmlvideofx/EffectPageCurl.qml \
    qml/qmlvideofx/EffectPassThrough.qml \
    qml/qmlvideofx/EffectPixelate.qml \
    qml/qmlvideofx/EffectPosterize.qml \
    qml/qmlvideofx/EffectRipple.qml \
    qml/qmlvideofx/EffectSelectionList.qml \
    qml/qmlvideofx/EffectSepia.qml \
    qml/qmlvideofx/EffectSharpen.qml \
    qml/qmlvideofx/EffectShockwave.qml \
    qml/qmlvideofx/EffectSobelEdgeDetection1.qml \
    qml/qmlvideofx/EffectTiltShift.qml \
    qml/qmlvideofx/EffectToon.qml \
    qml/qmlvideofx/EffectVignette.qml \
    qml/qmlvideofx/EffectWarhol.qml \
    qml/qmlvideofx/EffectWobble.qml \
    qml/qmlvideofx/FileBrowser.qml \
    qml/qmlvideofx/FileOpen.qml \
    qml/qmlvideofx/HintedMouseArea.qml \
    qml/qmlvideofx/Main.qml \
    qml/qmlvideofx/ParameterPanel.qml \
    qml/qmlvideofx/Slider.qml \
    shaders/billboard.fsh \
    shaders/blackandwhite.fsh \
    shaders/emboss.fsh \
    shaders/gaussianblur_h.fsh \
    shaders/gaussianblur_v.fsh \
    shaders/glow.fsh \
    shaders/isolate.fsh \
    shaders/magnify.fsh \
    shaders/pagecurl.fsh \
    shaders/pixelate.fsh \
    shaders/posterize.fsh \
    shaders/ripple.fsh \
    shaders/selectionpanel.fsh \
    shaders/sepia.fsh \
    shaders/sharpen.fsh \
    shaders/shockwave.fsh \
    shaders/sobeledgedetection1.fsh \
    shaders/tiltshift.fsh \
    shaders/toon.fsh \
    shaders/vignette.fsh \
    shaders/warhol.fsh \
    shaders/wobble.fsh

resources.prefix = /

RESOURCES += resources

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideofx
INSTALLS += target

QMAKE_INFO_PLIST = Info.plist

EXAMPLE_FILES += \
    qmlvideofx.png \
    qmlvideofx.svg
